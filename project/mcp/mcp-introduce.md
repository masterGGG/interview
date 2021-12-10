        MCP是一种纯异步网络事件通知，高性能，高可靠，可扩展的网络服务器框架，易于开发，通用性高。主要分为三类进程ccs（ccd）、smcp（mcd）和scc（dcc），他们各自的职能会在下面详述。或参考jeromechen的MCP++网络收发模块内部原理分享以及iandou的mcp++ 学习总结。

事件驱动
        mcp采用epoll事件驱动来达到高并发，数据接收，转发等网络通信功能。诸如服务fd，客户端接入fd，共享内存MQ对应的Fifo fd和后端服务fd，将一切可能阻塞的操作全部非阻塞化，通过epoll事件处理。epoll事件触发有ET和LT两种方式，在mcp框架里，主要采用的是ET触发的方式。

ccs
        ccs作为mcp框架的服务入口，主要有一下几个功能：

监听服务端口，监听客户端接入；
客户端请求数据验证；
合法请求路由至smcp；
监听管理服务端口，接收管理命令；

        ccs的启动流程图中，加载so文件是用来自行扩展的数据包验证以及smcp路由回调功能，不做详细分析。监听的端口包括配置文件中的服务端口和管理端口。



        请求的接收以及回应都是通过绑定的有名管道通知到对应的进程，ccs接收到数据之后，会验证数据有效性，对于合法请求，会打上自己的TMQHeadInfo头部，然后将数据写入与smcp通信的共享内存中，并通过有名管道通知smcp进程处理客户端请求。

        此外，css内部还会维护一个cs缓冲区和sc缓冲区，当css与smcp共享mq没有剩余空间时，会将新的请求放入到cs缓冲区，所以每次检测到client fd可读时，都会优先处理cs缓冲区中缓存的旧请求，以保证请求有序。

//接收对应socket的数据,return recv len
int CMainCtrl::RecvClientData(int iSocketSuffix) {
	//....//优先读取缓冲区中的旧请求
	int iOldDataSize = m_stBuffMngRecv.GetBufferSize(iSocketSuffix);
	//....
	//预留sizeof(TMQHeadInfo)长度用来构造shm头部，读取新请求，并将数据安放在旧请求之后，以保证数据有序
	int iRecvBytes = read(pSocketNode->m_iSocket, m_szBuffRecv+sizeof(TMQHeadInfo)+iOldDataSize, 
								sizeof(m_szBuffRecv)-sizeof(TMQHeadInfo)-iOldDataSize);
	//....
}
smcp
        smcp作为框架的业务逻辑处理模块，主要有一下几个功能：

监听有名管道，接收ccs发来的通知，从ccs到smcp的mq中获取请求；
业务逻辑处理；
对于非本机服务需要连接后端服务的请求，通知scc模块访问后端服务；
        smcp的内部结构与ccs类似。epoll 中监听ccs到smcp的MQ读端fd以及scc到smcp的MQ写端fd。业务逻辑处理模块以动态库的方式加载，需要继承SoWorker，并在OnMessage函数中实现业务逻辑。

scc
        scc是为了解决本地无法处理请求，需要访问后端服务的问题，主要有一下几个功能：

监听有名管道，接收smcp发来的通知；
维护到后端服务的连接，通信；
处理结果通知到smcp进行后续处理；
        scc的内部结构与ccs类似，启动流程也是类似。区别是scc不会有服务端口，仅通过mq接收smcp的请求；同时scc会根据mq消息的头部信息与对应的后端服务通信，并管理服务连接，以便复用，避免不必要的连接开销；

//用于从smcp到scc的MQ中获取业务请求
int CMainCtrl::CheckSvrMessage() {
	//....
                        //根据TMQHeadInfo中的ip和port，检查当前连接池是否已经包含对应服务的连接
			TSocketNode* pSocketNode = (TSocketNode*)m_stSocketNodeHash.GetObjectByKey((void *)szKey,iKeyLen,iNodeObjIdx);
			if (!pSocketNode)
			{
				//连接不存在
				int iSocket = CreateConnectSocket(pMQHeadInfo->m_unClientIP,pMQHeadInfo->m_usClientPort,
								m_stConfig.SOCKET_RCVBUF,m_stConfig.SOCKET_SNDBUF,0);
	                //....
                        }
	//....
}
应用模式
客户端模式
        客户端模式包含了smcp，scc两个模块，smcp负责业务逻辑处理，scc模块负责发起链接。

服务端模式
        服务端模式包含了ccs，smcp两个模块，ccs接收并检查服务请求，送往smcp进行业务逻辑的处理，smcp处理完之后将处理结果发回ccs。        

代理模式
        代理模式包含了ccs，smcp，scc三个模块，将服务请求转发至相应的后台服务。

       

