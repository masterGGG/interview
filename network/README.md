[toc]
# 1. TCP/IP协议
## 1.1 7层协议各是那些？
## 1.2 TCP三次握手
### 1.2.1 为什么TCP连接的时候是3次？2次不可以吗？
### 1.2.2 半连接状态？半连接队列？
## 1.3 四次挥手过程？
### 1.3.1 为什么TCP连接的时候是3次，关闭的时候却是4次？
### 1.3.2 close_wait和time_wait的区别？TIME_WAIT 和CLOSE_WAIT状态socket过多
### 1.3.3 为什么TIME_WAIT状态需要保持2MSL这么长的时间
## 1.3 如果已经建立了连接，但是客户端突然出现故障了怎么办？
## 1.4 TCP与UDP的区别，以及使用场景。
## 1.5 滑动窗口
## 1.6 拥塞控制
## 1.7 Keppalive机制？
## 1.8 Nagle算法是什么？

# 2. HTTP协议
## 2.1 HTTP协议？特性？劣势？
## 2.2 HTTP2？ 优劣势？
## 2.3 HTTP3？ QUIC？

# 3. 网络IO/socket编程
## 3.1 5种IO模型？
### 3.1.1 同步异步阻塞非阻塞的区别？
## 3.2 select与epoll的区别？epoll为什么快？
## 3.3 epoll工作模式，et与lt的区别，以及使用场景？
## 3.3 什么是reactor模式？