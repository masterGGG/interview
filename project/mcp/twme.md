[toc]

[中间件、高性能服务器、分布式存储等(redis、memcache、pika、rocksdb、mongodb、wiredtiger、高性能代理中间件)二次开发、性能优化，逐步整理文档说明并配合demo指导](https://github.com/y123456yz/middleware_development_learning)

twemproxy是twitter开源的redis/memcached 代理，数据分片提供取模，一致性哈希等手段，维护和后端server的长连接，自动踢除server,恢复server，提供专门的状态监控端口供外部工具获取状态监控信息。

# 流程分析
twemproxy后端支持多个server pool，为每个server pool分配一个监听端口用于接收客户端的连接。客户端和proxy建立连接记作client_conn，发起请求，proxy读取数据，放入req_msg中，设置msg的owner为client_conn，proxy根据策略从server pool中选取一个server并且建立连接记作server_conn，然后转发req_forward，将req_msg指针放入client_conn的output队列中，同时放入server_conn的input队列，然后触发server_conn的写事件，server_conn的写回调函数会从input队列中取出req_msg发送给对应的后端server，发送完成后将req_msg放入server_conn的output队列，当req_msg的响应rsp_msg回来后，调用rsp_filter(用于判断消息是否为空，是否消息可以不用回复等)和rsp_forward，将req_msg从server_conn的output队列中取出，建立req_msg和rsp_msg的对应关系(通过msg的peer字段)，通过req_msg的owner找到client_conn，然后启动client_conn的写事件，client_conn的写回调函数从client_conn的output队列中取出req_msg，然后通过peer字段拿到对应的rsp_msg，将其发出去。至此，一次请求从被proxy接收到最后响应给client结束。
```c
/*  twmeproxy中关于请求指针流传阶段的举例。
 *             Client+             Proxy           Server+
 *                              (nutcracker)
 *                                   .
 *       msg_recv {read event}       .       msg_recv {read event}
 *         +                         .                         +
 *         |                         .                         |
 *         \                         .                         /
 *         req_recv_next             .             rsp_recv_next
 *           +                       .                       +
 *           |                       .                       |       Rsp
 *           req_recv_done           .           rsp_recv_done      <===
 *             +                     .                     +
 *             |                     .                     |
 *    Req      \                     .                     /
 *    ===>     req_filter*           .           *rsp_filter
 *               +                   .                   +
 *               |                   .                   |
 *               \                   .                   /
 *               req_forward-//  (a) . (c)  \\-rsp_forward
 *                                   .
 *                                   .
 *       msg_send {write event}      .      msg_send {write event}
 *         +                         .                         +
 *         |                         .                         |
 *    Rsp' \                         .                         /     Req'
 *   <===  rsp_send_next             .             req_send_next     ===>
 *           +                       .                       +
 *           |                       .                       |
 *           \                       .                       /
 *           rsp_send_done-//    (d) . (b)    //-req_send_done
 *
 *
 * (a) -> (b) -> (c) -> (d) is the normal flow of transaction consisting
 * of a single request response, where (a) and (b) handle request from
 * client, while (c) and (d) handle the corresponding response from the
 * server.
 **/
```
twmeproxy采用单线程epoll事件回调的方式，模拟责任链模式的机制，进行请求的相关处理。其中请求以msg结构封装，并在责任链上传递。责任链节点通过连接上读写事件回调的方式触发。
## 连接
twmeproxy上的socket对象分为3类：
**1. proxy** 
>  proxyConnn表示twmeproxy监听端口对应的socket连接，其上只注册了读事件，在client发起连接请求时触发。读事件回调会创建clientConn，并指定clientConn的一系列事件回调处理函数。
```c
conn->recv = msg_recv;// 从conn读数据
conn->recv_next = req_recv_next; // 在真正从conn读数据之前，需要分配一个req_msg，用于承载读进来的数据
conn->recv_done = req_recv_done; //每次读完一个完整的消req_msg被调用
conn->send = msg_send;// 将从server收到的响应rsp_msg发给客户端
conn->send_next = rsp_send_next;// 每次发送rsp_msg之前需要首先确定从哪个开始发
conn->send_done = rsp_send_done;// 每次发送完成一个rsp_msg给客户端，调一次
conn->enqueue_outq = req_client_enqueue_omsgq;// proxy每次接收到一个client发过来的req_msg，将req_msg入conn的output 队列
conn->dequeue_outq = req_client_dequeue_omsgq;// 给客户端发送完rsp_msg后将其对应的req_msg从conn的output队列中删除
```

<a id='clientConn'></a>
**2. client**
> clientConn表示client到Nutcracker的一条链路，注册了读写事件，读事件(conn_recv_t)用于接收client发送的request，写事件(conn_send_t)用于返回client request对应的response。

<a id='serverConn'></a>
**3. server**
> serverConn表示Nutcracker到后端cache的一条链路，注册读写事件，读事件(conn_recv_t)用于接收后端cache返回的应答数据。写事件(conn_send_t)用于将实际request发送给后端cache。

## 0拷贝设计
[client](#clientConn)和[server](#serverConn)包含两个requests的先进先出队列：
**imsg_q**： 记录收到但还未处理的请求。
**omsg_q**： 按请求的顺序缓存已经交由后续处理的请求。
twmeproxy通过将请求的指针在其中3个队列中流转（clientConn的imsg_q没有用到），实现0拷贝。请求流转包含3个阶段：
> proxyConn收到一个完整请求包进行路由转发`req_forward`的过程中，会将请求指针放入proxyConn的omsg_q（之后会遍历这个队列，按序返回就绪请求的应答）和serverConn的imsg_q，并添加serverConn的写事件到epoll中。

> 触发serverConn可写事件，遍历serverConn的imsg_q，将请求对了发送给后端cache，同时将请求指针从imsg_q移除，加入omsg_q队列。

> 后端响应请求，触发serverConn可读事件，检查无误之后将请求指针从omsg_q移除。调用req_done依次返回proxyConn omsg_q中以应答就绪的请求。

```c
/*  twmeproxy中关于请求指针流传阶段的举例。
 *      Clients                             Servers
 *                                    .
 *    in_q: <empty>                   .
 *    out_q: req11 -> req12           .   in_q:  req22
 *    (client1)                       .   out_q: req11 -> req21 -> req12
 *                                    .   (server1)
 *    in_q: <empty>                   .
 *    out_q: req21 -> req22 -> req23  .
 *    (client2)                       .
 *                                    .   in_q:  req23
 *                                    .   out_q: <empty>
 *                                    .   (server2)
 **/
```

# 代码结构
代码规模不大只有1w多行，而且功能划分的比较清晰，包括[1]：
## 事件处理
event/nc_epoll.c、event/nc_event.h、event/nc_evport.c、event/nc_kqueue.c
## 各种Hash函数
hashkit/nc_crc16.c、hashkit/nc_crc32.c、hashkit/nc_fnv.c、hashkit /nc_hashkit.h、hashkit/nc_hsieh.c、hashkit/nc_jenkins.c、hashkit /nc_ketama.c、hashkit/nc_md5.c、hashkit/nc_modula.c、hashkit/nc_murmur.c、 hashkit/nc_one_at_a_time.c、hashkit/nc_random.c
## 协议
proto/nc_memcache.c、proto/nc_proto.h、proto/nc_redis.c
## 自定义的数据类型
nc_array.c、nc_array.h、nc_string.c、nc_string.h
## 网络通信相关
nc_connection.c、nc_connection.h、nc_client.c、nc_client.h、nc_proxy.c、nc_proxy.h
## 信号处理
nc_signal.c、nc_signal.h
## 关键数据结构和算法
nc_rbtree.h、nc_rbtree.c、nc_queue.h、nc_request.c、nc_response.c、nc_mbuf.c、 nc_mbuf.h、nc_message.c、nc_message.h、nc_server.c、nc_server.h
## 统计、日志和工具
nc_stats.c、nc_stats.h、nc_log.c、nc_log.h、nc_util.c、nc_util.h
## 配置文件
nc_conf.c、nc_conf.h
## 主程序
nc.c、nc_core.c、nc_core.h