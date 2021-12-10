[toc]

# SHM区域简介
proxy服务是基于MCP框架开发的，pipeline功能主要是在CCS模块的SHM中添加一个序列号管理器，为每个请求加上序号，按序回包，这里SHM指的是CCS模块维护每个sock的信息创建的共享内存。
![enter image description here](https://km.woa.com/gkm/api/img/cos-file-url?url=https%3A%2F%2Fkm-pro-1258638997.cos.ap-guangzhou.myqcloud.com%2Ffiles%2Fphotos%2Fpictures%2F202009%2F1599208056_1_w984_h322.png&is_redirect=1)


上图为Shm的宏观划分，主要包含3部分：RecvShm（处理队列满时缓存接收到的请求包）、SendShm（系统缓冲区不足时缓存应答数据包）和SockNodeShm（记录每个sock的类型，ip，port，发送时间戳等信息）。
pipeline功能主要是SendShm的改造和扩展，这里详细讲解下SendShm的构造。如下图：

![enter image description here](https://km.woa.com/gkm/api/img/cos-file-url?url=https%3A%2F%2Fkm-pro-1258638997.cos.ap-guangzhou.myqcloud.com%2Ffiles%2Fphotos%2Fpictures%2F202009%2F1599208385_18_w1436_h988.png&is_redirect=1)

SendShm由Socket应答缓冲区管理器`CBuffMng`和数据块对象管理器`TIdxObjMng`构成。缓冲区管理器中的桶`TBuffItem`与SockNodeShm区的sock数组一一对应，负责管理每个sock上的应答队列，包含头数据块索引，缓存块大小，尾数据块索引等。数据块对象管理器包含索引数组`TIdx`和数据块数组`BufBlock`，每个TIdx会记录下一个数据块的索引,构成一个链表。初始状态下，只有一个空闲链表。

![enter image description here](https://km.woa.com/gkm/api/img/cos-file-url?url=https%3A%2F%2Fkm-pro-1258638997.cos.ap-guangzhou.myqcloud.com%2Ffiles%2Fphotos%2Fpictures%2F202009%2F1599208999_18_w1842_h636.png&is_redirect=1)

上图为应答包处理流程，这里解释下缓存应答包的释放逻辑，在发送完就绪应答包之后，根据发送大小和sock中记录的应答包存储位置`TBuffItem::m_iBuffIdx`，释放定长的应答包空间。

# pipeline设计
序列号管理器的共享内存结构如下：

![enter image description here](https://km.woa.com/gkm/api/img/cos-file-url?url=https%3A%2F%2Fkm-pro-1258638997.cos.ap-guangzhou.myqcloud.com%2Ffiles%2Fphotos%2Fpictures%2F202009%2F1599405405_85_w1652_h1050.png&is_redirect=1)

序列号管理器的内存结构与SendSHM类似，核心结构是上图的`TSockItem`（管理每个sock上的序列号列表）和`TSeqItem`（管理每个序列号的应答情况）。其中`TSockItem`与`TBuffItem`功能类似，记录每个sock的序列号列表，当前管理器的状态以及pipeline功能切换前应答缓冲区中的旧格式请求数据位置。`TSeqItem`用于记录每个序列号缓存应答包的位置，以便在队首应答包就绪时，能迅速遍历就绪序列号列表的应答包，快速应答客户端，同时更新缓冲区中应答包的发送状态为已发送。
与之前直接缓存应答包相比，**新的缓存逻辑**为每个应答包添加了一个应答包头部`TSeqBufHead`，记录应答包的长度以及发送状态，以便在释放应答包占用空间时能准确释放已经发送应答包，避免误删。
```cpp
typedef struct
{
    int m_iBufSize;     // 应答包的长度
    int m_bSent;        // 发送状态

    typedef enum
    {
        BUFFER_NEED_SEND = 0,
        BUFFER_SENT,
    } emSeqBuffStatusType;
} TSeqBufHead;
```

## 请求入队
在每个请求发往后端时，从序列号管理器获取一个空闲序列号加入到当前sock的序列号列表`TSockItem::m_iSeqTail`里即可。

## 应答出队

![enter image description here](https://km.woa.com/gkm/api/img/cos-file-url?url=https%3A%2F%2Fkm-pro-1258638997.cos.ap-guangzhou.myqcloud.com%2Ffiles%2Fphotos%2Fpictures%2F202009%2F1599415986_55_w1750_h980.png&is_redirect=1)
proxy接收到后端回应的应答包时，检测包内是否包含合理序列号，序列号是否超时。如果检查通过且序列号是最早的请求序列号，则直接将应答包回复给客户端，否则缓存在应答缓冲区，等待队首序列号应答包就绪。
