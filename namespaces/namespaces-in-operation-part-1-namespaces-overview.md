# 命名空间概述

***Copyright (C) 2014 Leslie Zhai <xiang.zhai@i-soft.com.cn>***

> http://lwn.net/Articles/531114/

Linux 3.8合并窗口看到了接受Eric Biederman的一大波[用户命名空间及其相关补丁](http://lwn.net/Articles/528078/)。
尽管还有一些细节需要完善，例如，一些Linux文件系统还不支持用户命名空间，目前已经
完成了用户命名空间的功能实现。

完成用户命名空间的工作是一个里程碑，原因如下。第一，该工作代表了迄今为止巨复杂的
命名空间的实现已经完成，从Linux 2.6.23用户命名空间迈出的第一步，已经过去了5年。
第二，命名空间的工作已经处于“稳定”，现有的命名空间大部分已经实现。但不意味着命名
空间的工作已经结束：未来可能会添加其他的命名空间，或对现有的命名空间进行扩展，
比如[内核日志命名空间的隔离](http://lwn.net/Articles/527342/)。
第三，近期对用户命名空间实现的变更，意味着游戏规则改变，如何使用命名空间：
从Linux 3.8开始，没有权限的进程可以创建用户命名空间，并在其中具有全部权限，
反过来理解就是，在一个用户命名空间内允许创建任意其他类型的命名空间。

所以，现在趁热打铁看看命名空间的概述和API。该系列文章首次概括描述现有命名空间；
接下来，会讲如何在程序中使用命名空间的API。

## 命名空间

目前，Linux实现了6种命名空间。每个命名空间封装了一个抽象的全局系统资源，具有该
命名空间的进程拥有隔离的全局资源实例。命名空间的还有一个目的是用来支持容器的实现，
容器是轻量化虚拟（还有其他目的）工具，可以让一组进程产生错觉，错误地认为他们是
系统中唯一的进程。

In the discussion below, we present the namespaces in the order that they were implemented (or at least, the order in which the implementations were completed). The CLONE_NEW* identifiers listed in parentheses are the names of the constants used to identify namespace types when employing the namespace-related APIs (clone(), unshare(), and setns()) that we will describe in our follow-on articles.

Mount namespaces (CLONE_NEWNS, Linux 2.4.19) isolate the set of filesystem mount points seen by a group of processes. Thus, processes in different mount namespaces can have different views of the filesystem hierarchy. With the addition of mount namespaces, the mount() and umount() system calls ceased operating on a global set of mount points visible to all processes on the system and instead performed operations that affected just the mount namespace associated with the calling process.

One use of mount namespaces is to create environments that are similar to chroot jails. However, by contrast with the use of the chroot() system call, mount namespaces are a more secure and flexible tool for this task. Other more sophisticated uses of mount namespaces are also possible. For example, separate mount namespaces can be set up in a master-slave relationship, so that the mount events are automatically propagated from one namespace to another; this allows, for example, an optical disk device that is mounted in one namespace to automatically appear in other namespaces.

Mount namespaces were the first type of namespace to be implemented on Linux, appearing in 2002. This fact accounts for the rather generic "NEWNS" moniker (short for "new namespace"): at that time no one seems to have been thinking that other, different types of namespace might be needed in the future.

UTS namespaces (CLONE_NEWUTS, Linux 2.6.19) isolate two system identifiers—nodename and domainname—returned by the uname() system call; the names are set using the sethostname() and setdomainname() system calls. In the context of containers, the UTS namespaces feature allows each container to have its own hostname and NIS domain name. This can be useful for initialization and configuration scripts that tailor their actions based on these names. The term "UTS" derives from the name of the structure passed to the uname() system call: struct utsname. The name of that structure in turn derives from "UNIX Time-sharing System".

IPC namespaces (CLONE_NEWIPC, Linux 2.6.19) isolate certain interprocess communication (IPC) resources, namely, System V IPC objects and (since Linux 2.6.30) POSIX message queues. The common characteristic of these IPC mechanisms is that IPC objects are identified by mechanisms other than filesystem pathnames. Each IPC namespace has its own set of System V IPC identifiers and its own POSIX message queue filesystem.

PID namespaces (CLONE_NEWPID, Linux 2.6.24) isolate the process ID number space. In other words, processes in different PID namespaces can have the same PID. One of the main benefits of PID namespaces is that containers can be migrated between hosts while keeping the same process IDs for the processes inside the container. PID namespaces also allow each container to have its own init (PID 1), the "ancestor of all processes" that manages various system initialization tasks and reaps orphaned child processes when they terminate.

From the point of view of a particular PID namespace instance, a process has two PIDs: the PID inside the namespace, and the PID outside the namespace on the host system. PID namespaces can be nested: a process will have one PID for each of the layers of the hierarchy starting from the PID namespace in which it resides through to the root PID namespace. A process can see (e.g., view via /proc/PID and send signals with kill()) only processes contained in its own PID namespace and the namespaces nested below that PID namespace.

Network namespaces (CLONE_NEWNET, started in Linux 2.4.19 2.6.24 and largely completed by about Linux 2.6.29) provide isolation of the system resources associated with networking. Thus, each network namespace has its own network devices, IP addresses, IP routing tables, /proc/net directory, port numbers, and so on.

Network namespaces make containers useful from a networking perspective: each container can have its own (virtual) network device and its own applications that bind to the per-namespace port number space; suitable routing rules in the host system can direct network packets to the network device associated with a specific container. Thus, for example, it is possible to have multiple containerized web servers on the same host system, with each server bound to port 80 in its (per-container) network namespace.

User namespaces (CLONE_NEWUSER, started in Linux 2.6.23 and completed in Linux 3.8) isolate the user and group ID number spaces. In other words, a process's user and group IDs can be different inside and outside a user namespace. The most interesting case here is that a process can have a normal unprivileged user ID outside a user namespace while at the same time having a user ID of 0 inside the namespace. This means that the process has full root privileges for operations inside the user namespace, but is unprivileged for operations outside the namespace.

Starting in Linux 3.8, unprivileged processes can create user namespaces, which opens up a raft of interesting new possibilities for applications: since an otherwise unprivileged process can hold root privileges inside the user namespace, unprivileged applications now have access to functionality that was formerly limited to root. Eric Biederman has put a lot of effort into making the user namespaces implementation safe and correct. However, the changes wrought by this work are subtle and wide ranging. Thus, it may happen that user namespaces have some as-yet unknown security issues that remain to be found and fixed in the future.

Concluding remarks

It's now around a decade since the implementation of the first Linux namespace. Since that time, the namespace concept has expanded into a more general framework for isolating a range of global resources whose scope was formerly system-wide. As a result, namespaces now provide the basis for a complete lightweight virtualization system, in the form of containers. As the namespace concept has expanded, the associated API has grown—from a single system call (clone()) and one or two /proc files—to include a number of other system calls and many more files under /proc. The details of that API will form the subject of the follow-ups to this article.
