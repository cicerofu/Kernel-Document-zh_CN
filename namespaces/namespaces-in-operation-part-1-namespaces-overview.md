# 命名空间概述

***Copyright (C) 2014 Leslie Zhai <xiang.zhai@i-soft.com.cn>***

> http://lwn.net/Articles/531114/

Linux 3.8合并窗口看到了接受Eric Biederman的一大波[用户命名空间及其相关补丁](http://lwn.net/Articles/528078/)。
尽管还有一些细节需要完善，例如，一些Linux文件系统还不支持用户命名空间，目前已经
完成了用户命名空间的功能实现。

完成用户命名空间的工作是一个里程碑，原因如下。
第一，该工作代表迄今为止巨复杂的命名空间的实现已经完成，从Linux 2.6.23
用户命名空间迈出的第一步，已经过去了5年。
第二，命名空间的工作已经处于“稳定”，现有的命名空间大部分已实现。
但不意味着命名空间的工作已结束：未来可能会添加其他的命名空间，或对现有的命名空间进行扩展，比如[内核日志命名空间的隔离](http://lwn.net/Articles/527342/)。
第三，近期对用户命名空间实现的变更，意味着游戏规则已改变，如何使用命名空间：
从Linux 3.8开始，没有权限的进程可以创建用户命名空间，并在其中具有全部权限，
反之，在一个用户命名空间内允许创建任意其他类型的命名空间。

所以，现在趁热打铁看看命名空间的概述和API。
该系列文章首次概括描述现有命名空间；
接下来，会讲如何在程序中使用命名空间的API。

## 命名空间

目前，Linux实现了6种命名空间。
每个命名空间封装了一个抽象的全局系统资源，具有该命名空间的进程拥有隔离的全局资源实例。
命名空间还有一个目的是用来支持容器的实现，容器是轻量化虚拟（还有其他目的）工具，可以让一组进程产生错觉，错误地认为他们是系统中唯一的进程。

接下来，以命名空间被（完全）实现出来的先后顺序进行阐述。括号中罗列出来的
常量CLONE_NEW*用来区分命名空间，及其与之相关的API：接下来的几篇文章中将介绍
clone()、unshare()和setns()。

Mount namespaces (CLONE_NEWNS, Linux 2.4.19) isolate the set of filesystem mount points seen by a group of processes. Thus, processes in different mount namespaces can have different views of the filesystem hierarchy. With the addition of mount namespaces, the mount() and umount() system calls ceased operating on a global set of mount points visible to all processes on the system and instead performed operations that affected just the mount namespace associated with the calling process.

挂载（命名空间）可以创建类似chroot狱的环境。
但是，不同与chroot()系统调用，挂载（命名空间）更加安全和灵活。
还可以更加高大上地使用挂载（命名空间）。
例如，可以将挂载（命名空间）设置成主从关系，挂载信号可以从一个命名空间自动传递到另一个里；
例如，当磁盘设备挂载在一个命名空间下，就会自动出现在另一个里。

挂载（命名空间）是Linux上第一个实现出来的，出现在2002年。
“NEWNS”名字（“new namespaces”的简称）如此普适的原因：那个时候没人意识到，以后会需要其他类型的命名空间。

UTS命名空间（CLONE_NEWUTS，Linux 2.6.19）隔离两个系统ID：节点名和域名；
使用系统调用sethostname()和setdomainname()来设置。
在容器的上下文当中，UTS命名空间特性允许每个容器拥有自己的主机名和NIS域名。
对初始化和配置脚本非常有帮助，可以基于名称进行裁剪。
“UTS”该术语来源于传递给uname()系统调用的结构体的名字：struct ustname。
结构体的名字又是来源于“UNIX Time-sharing System”。

IPC命名空间（CLONE_NEWIPC，Linux 2.6.19）隔离某种进程通讯资源，即System V IPC对象
和（Linux 2.6.30之后）POSIX消息队列。
这些IPC机制的共性是：IPC对象不是由文件系统的路径名来区分。
每个IPC命名空间都有自己的System V IPC id，以及POSIX消息队列文件系统。

PID命名空间（CLONE_NEWPID，Linux 2.6.24）隔离进程ID数字空间。
换言之，在不同PID命名空间里的进程可以具有相同的PID。
PID命名空间的一个最大好处就是容器可以在主机之间迁移，保持容器内的进程具有相同的ID。
PID命名空间还允许每个容器拥有init（PID 1），“所有进程的祖先”管理这系统初始化任务，回收退出的子进程。

From the point of view of a particular PID namespace instance, a process has two PIDs: the PID inside the namespace, and the PID outside the namespace on the host system. PID namespaces can be nested: a process will have one PID for each of the layers of the hierarchy starting from the PID namespace in which it resides through to the root PID namespace. A process can see (e.g., view via /proc/PID and send signals with kill()) only processes contained in its own PID namespace and the namespaces nested below that PID namespace.

网络命名空间（CLONE_NEWNET，从Linux 2.4.19 2.6.24开始，大概到Linux 2.6.29大部分
已完成）提供网络系统资源的隔离。
所以，每个网络命名空间具有自己的网络设备、IP地址、IP路由表、/proc/net目录、
端口号等。

网络命名空间让容器在网络方面实用：每个容器具有自己的（虚拟）网络设备，自己的应用
绑定到命名空间相关的端口号空间；主机的路由规则可以向与容器相关联的网络设备上发送
网络数据包。
所以，例如，在同一台主机上可以有多个容器web服务，每个服务都绑定到它的（每个容器）
网络命名空间80端口。

~~User namespaces (CLONE_NEWUSER, started in Linux 2.6.23 and completed in Linux 3.8) isolate the user and group ID number spaces. In other words, a process's user and group IDs can be different inside and outside a user namespace. The most interesting case here is that a process can have a normal unprivileged user ID outside a user namespace while at the same time having a user ID of 0 inside the namespace. This means that the process has full root privileges for operations inside the user namespace, but is unprivileged for operations outside the namespace.

Starting in Linux 3.8, unprivileged processes can create user namespaces, which opens up a raft of interesting new possibilities for applications: since an otherwise unprivileged process can hold root privileges inside the user namespace, unprivileged applications now have access to functionality that was formerly limited to root. Eric Biederman has put a lot of effort into making the user namespaces implementation safe and correct. However, the changes wrought by this work are subtle and wide ranging. Thus, it may happen that user namespaces have some as-yet unknown security issues that remain to be found and fixed in the future.~~

Concluding remarks

It's now around a decade since the implementation of the first Linux namespace. Since that time, the namespace concept has expanded into a more general framework for isolating a range of global resources whose scope was formerly system-wide. As a result, namespaces now provide the basis for a complete lightweight virtualization system, in the form of containers. As the namespace concept has expanded, the associated API has grown—from a single system call (clone()) and one or two /proc files—to include a number of other system calls and many more files under /proc. The details of that API will form the subject of the follow-ups to this article.
