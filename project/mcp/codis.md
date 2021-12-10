[toc]
# 0. 请求时序
* Proxy每接到一个客户端连接，就创建一个独立的session进行处理
* codis将请求与结果关联起来的方式，就是把结果当做request的一个属性
* Session核心就是创建loopReader和loopWriter。loopReader负责读取和分发请求到后端，loopWriter负责合并请求结果，然后返回给客户端。
* forwardSync将指定的slot、request、键的哈希值，经过process得到实际处理请求的BackendConn，然后把请求放入BackendConn的chan *Request中，等待处理
* backendConn负责实际对redis请求进行处理，loopWriter负责从backendConn.input中取出请求并发送，loopReader负责遍历所有请求，从redis.Conn中解码得到resp并设置为相关的请求的属性
# 1. 目录简介
`proxy`目录是codis代理层的具体实现。[请求接收第一步](#step_1)。
## 1.1 redis
`redis`目录主要是proxy与后端redis交互的实现。包含以下几个文件：
### 1.1.1 conn.go
提供与后端redis交互的API接口。内部整合了请求的编码发送以及应答的解码接收。
### 1.1.2 decoder.go 
实现redis应答数据的接收以及解析。
<a id='step_6_2'></a>
#### 1.1.2.1 decodeResp
调用`utils.bufio2.ReadByte`获取后端返回的应答数据。[应答接收下一步](#step_6_3)
### 1.1.3 encoder.go
实现了请求数据编码成RESP协议格式。<a id='step_3'></a>[返回上一步](#step_2_3)
<a id='step_5_2'></a>
#### 1.1.3.1 EncodeMultiBulk
主要是将就绪的数据以RESP协议格式写入bio的数据缓冲区，等待Flush[请求转发下一步](#step_5_3)调用。
<a id='step_5_3'></a>
#### 1.1.3.2 Flush
将缓存的数据发送给后端cache做实际处理。[应答接收第一步](#step_6_1)
<a id='step_7_3'></a>
#### 1.1.3.3 Encode
调用`bufio2.WriteByte`将应答数据以RESP协议的格式写入bufio2的缓冲区中，然后调用[Flush 应答回复下一步](#step_7_4)
<a id='step_7_4'></a>
#### 1.1.3.4 Flush
调用`io.Writer`将缓冲的应答数据回复给client。至此一个请求的处理应答流程结束。
### 1.1.4 resp.go 
## 1.2 proxy.go
<a id='step_1'></a>
### 1.2.1 New
该函数启动一个proxy实例，并启动两个goroutine（[serveAdmin](#serveAdmin)和[serveProxy 请求接收下一步](#step_1_1)）
<a id='serveAdmin'></a>
### 1.2.2 serveAdmin
该函数对发送到11080端口的请求做处理。
<a id='step_1_1'></a>
### 1.2.3 serveProxy
内部启动一个新的goroutine调用`acceptConn`处理发送到19000端口的redis请求。
当客户端发起连接请求后，创建一个[session 请求接收下一步](#step_2_1)，并调用Start方法进行该连接上的数据收发。
## 1.3 session.go
每接到一个redis客户端的连接请求，就创建一个独立的session进行处理（默认的每个session的tcp连接过期时间为75秒，也就是每个请求最多处理75秒）。
<a id='step_2_1'></a>
### 1.3.1 NewSession
对当前建立的连接进行必要的初始化工作。这里的第一个参数是net.Conn，Conn是一个通用的面向流的网络连接，多个goroutines可以同时调用Conn的方法。这里的net.Conn就是我们之前Proxy的lproxy这个Listener监听到的19000请求到来的时候返回的net.Conn。[Start 请求接收下一步](#step_2_2)
<a id='step_2_2'></a>
### 1.3.2 Start
方法首先检查总的session数量是否超过上限（默认为1000），以及Router是否在线。如果不符合就返回错误。方法是被Session中的Once.Do包起来的，即使有多次调用，也只会执行一次，避免浪费不必要的性能。
之后，会创建一个请求缓存channel`tasks`，以及两个goroutine`loopWriter`（client请求的应答就绪触发回写）和[loopReader 请求接收下一步](#step_2_3)（读取client请求并进行处理）。
<a id='step_2_3'></a>
### 1.3.3 loopReader
* session只要没有退出，就一直从conn中取请求，直到连接断开或者异常然后会关闭tasks这个requestChan。
* 通过[s.Conn.DecodeMultiBulk](#step_3)从client连接中取出请求参数。
* 根据不同的命令做不同的操作
* * 如果需要转发至对应redis处理
* * * 调用[Router.dispatch 请求映射第一步](#step_4_1)进行转发处理<a id='dispatch_bak1'></a>。
* * * 调用<a id='step_4_3'></a>[PushBack 请求映射下一步](#step_4_4)加入请求缓存channel`tasks`，如果收到请求应答，会触发`loopWriter`回复client。
<a id='step_7_1'></a>
### 1.3.4 loopWriter
前面我们在`tasks.PushBack(r)`中，将请求放入了data []*Request切片，现在就是取出最早的请求及其处理结果。如果当前session的requestChan为空，就调用cond.wait让goroutine等待，直到调用pushback又放入请求为止。
会依次阻塞等待请求队列中请求的应答就绪，如果就绪就调用handleResponse进行回复处理。[应答回复下一步](#step_7_2)
<a id='step_7_2'></a>
### 1.3.5 handleResponse
goroutine都会一直阻塞到所有请求处理完之后，前面我们已经看到，向RequestChan中添加一个请求，backend.go里将请求加入到input队列时，request.Batch会加一，后面BackendConn调用setResponse，也就是完成处理的时候，又会调用Done方法减一。[应答回复下一步](#step_7_3)
## 1.4 router.go
每个codis集群会维护1024个槽位，每个槽位对应一个`group`（codis_server，redis服务）。具体分配策略这里不做阐述。
### 1.4.1 fillSlot
重新分配slot，记录slot对应的后端cache，以及相关转发的方法`forwardMethod`。之后通过`Retain`从`Router`的连接池`sharedBackendConnPool`中获取一个共享连接`sharedBackendConn`（参考**backend.go**中的Retain方法）。
<a id='step_4_1'></a>
### 1.4.2 dispatch
[返回上一步](#dispatch_bak1)
根据key进行hash计算，找到对应的存储slot，`slot.forward`将请求分发，[Forward 请求映射下一步](#step_4_2)
## 1.5 forward.go
该文件包含了具体的转发逻辑。
<a id='step_4_2'></a>
### 1.5.1 forwardSync.Forward
主要包含两个功能
* 通过`process`方法获取slot对应的后端连接`BackendConn`，具体细节参考[forward2](#step_4_2_1)方法。
* 通过[bc.PushBack 请求映射下一步](#step_4_3)方法将请求加入`BackendConn`的请求channel`input`。
<a id='step_4_2_1'></a>
### 1.5.2 forward2
获取真正处理redis请求的BackendConn。主要有两种选择方式：
* 如果是读请求，就调用replicaGroup来处理。
* 从sharedBackendConn中取出一个BackendConn（sharedBackendConn中储存了BackendConn组成的二维切片）。[返回上一步](#step_4_2)
## 1.6 jodis.go
## 1.7 mapper.go
mapKey2Slot()处理HashTag，并使用CRC32计算哈希值。
## 1.8 metrics.go
## 1.9 backend.go
基于redis目录封装与后端redis通信的结构`BackendConn`。
<a id='step_4_4'></a>
### 1.9.1 PushBack
将请求放入BackendConn等待处理。如果request的sync.WaitGroup不为空，就加一，然后判断加一之后的值，如果加一之后couter为0，那么所有阻塞在counter上的goroutine都会得到释放。将请求直接存入到BackendConn的chan *Request中，等待后续被取出并进行处理。
* r.Batch记录当前连请求拆分的数量，但命令请求即为1.
* 之后将请求丢入input chan。[请求转发第一步](#step_5_1)
### 1.9.2 NewBackendConn
### 1.9.3 newBackendReader
<a id='step_5_1'></a>
### 1.9.4 loopWriter
1. 内部会调用`newBackendReader`启动了loopReader。
2. 从bc对应的input chan中取出就绪的request请求进行实际转发。[请求转发下一步](#step_5_2)。
3. 请求发往后端cache后，把请求写入tasks这个channel，触发loopReader接收应答数据。
<a id='step_6_1'></a>
### 1.9.5 loopReader
1. 遍历tasks里的请求
2. 获取请求的应答数据[应答接收下一步](#step_6_2)
<a id='step_6_3'></a>
### 1.9.6 setResponse
将应答数据赋值给对应的请求Request。同时将Reques上的Batch数减一，触发请求对应的session的handleResponse方法。[应答回复第一步](#step_7_1)
## 1.10 proxy_api.go
## 1.11 request.go
## 1.12 delay.go
## 1.13 config.go
## 1.14 slots.go
## 1.15 stats.go
