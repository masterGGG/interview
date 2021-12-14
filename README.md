[toc]

常见招聘要求
```
1.基础扎实，熟练掌握C/C++编程，3年以上Linux环境开发经验；
2.了解常用数据结构和算法和设计模式，并能灵活运用；
3.熟悉分布式系统的设计和应用，有海量服务相关经验者优先；

熟悉Unix/Linux操作系统原理及常用工具，
熟悉Unix/Linux下C++, golang开发和程序性能调优，
熟悉TCP/IP协议、进程间通讯编程，
熟悉Unix/Linux下常用架构设计方法；
熟悉后台分布式的主要架构，
熟悉Mysql等数据库，
熟悉NoSQL存储
熟悉面向对象设计。

	
1、掌握Linux平台下的C/C++/Go一种以上语言并有实际工程经验，熟悉网络编程，掌握常用的通信协议；
2、熟悉分布式系统相关理论和工程实践，掌握Mysql、Redis等组件的原理和使用；
3、熟悉多进程/协程编程，熟悉Unix/Linux下常用后台服务架构设计方法；
4、熟悉K8S, Sevice Mesh微服务架构、消息推送、网络加速等技术者优先考虑；
5、有高性能后台服务开发经验者优先考虑;

	
1. 本科及以上学历，计算机相关专业，三年以上软件开发工作经验；
2. 对后台开发有浓厚的激情，热爱编程，具有后台Server架构设计、开发以及性能调优能力；
3. 熟悉Unix/Linux操作系统，精通C++语言，熟悉面向对象开发方法；
4. 熟悉TCP/IP协议，熟悉网络编程，熟悉常用的数据结构与算法，具备良好的分析解决问题能力；
5. 熟悉MYSQL/NoSQL的开发，Shell/Python等脚本语言；

	
1. 计算机相关专业本科及以上学历；
2. 3年以上服务器端开发工作经验；
3. 熟悉Unix/Linux操作系统、面向对象编程技术、TCP/IP协议相关知识；
4. 熟练掌握golang/C++编程，能独立进行后台开发和性能调优；
5. 熟悉mysql、es、redis、mongodb等常用存储系统，熟悉MySQL等数据库的使用以及SQL调优;
```

本仓库主要是记录专业技能要点，用于应聘c++后台开发岗位。涉及到`操作系统`、`网络编程`、`TCP/IP`、`多线程编程`、`算法`、`数据结构`等多个方面。

# 操作系统 os
os为操作系统目录，涉及到`进程`、`线程`、`内存管理`、`CPU调度算法`等方面。

# 网络编程 network
network是网络编程相关知识点的整理目录，涉及到`TCP/IP`、`IO多路复用`、`IO模型`等方面。

# 语言相关 c
c是c/c++语言相关知识点的整理目录，涉及到`面对对象设计`、`c++11特性`、`stl`等方面。。

# 设计模式 designmode
designmode是设计模式相关的目录，涉及到21种设计模式、6个设计原则等。

# 数据结构与算法

# linux环境开发

# 自我介绍：
  1.名字  学校，第一份从校招进入，，，，，主要负责什么工作，什么时间加入....，20197月加入腾信 pcg 应用架构，负责pcg存储平台相关开发与维护工作
# 进程与线程的区别
定义：进程是操作系统资源分配的基本单位，线程是操作调度的基本单位。
进程对于操作系统来说，操作系统会把进程当作分配资源的--->,这些资源包括.....-->，线程是衍生与进程的，它共享进程的......，自己独有....，
在linux平台下，进程是.....，线程是....，它是....线程模型，KSE是`操作系统内核调度器调度的对象实体`，某一些地方也把他称作内核级线程，这就是我们常说的`线程是操作系统调度和分派的最小单位`。与KSE相对的是`用户态线程`，用户态线程运行在用户态，切换和调度由应用程序封装的线程库控制，对于操作系统不可见。
常用线程模型.....,linux使用这么线程模型优势....，劣势
多进程环境：
  优势
  劣势
多线程环境
  优势
  劣势
协程：
  1.定义：
  2.演进过程，线程模型--->协程-->应用场景
  3.实例：有栈、无栈，你看过的，优势在哪里、劣势，golang、libco，ucontext、异步io_using