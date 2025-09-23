/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-09-17 09:54:43
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-09-23 09:44:53
 * @Description: 
 */
#ifndef BINDTHREAD_H     
#define BINDTHREAD_H  

#ifdef Q_OS_LINUX // 检查是否是 Linux 操作系统  
#include <sched.h>      // For sched_setaffinity, cpu_set_t
#include <unistd.h>     // For getpid (optional, useful for process affinity)
#include <sys/syscall.h> // For SYS_gettid (for thread affinity)
#include <QDebug>       // For Qt's debugging output
#include <QThread>      // If binding QThreads
#include <pthread.h>    // If binding std::thread or raw pthreads

bool bindThreadToCpu(int cpu_id)
{
    // 获取当前线程的 ID
    // 对于标准C++线程 (std::thread) 或 pthreads，使用 pthread_self()
    // 对于 Qt 线程 (QThread)，如果想绑定其内部的工作线程，可能需要更复杂的逻辑
    // 对于当前进程的主线程，可以直接使用 getpid() 或 syscall(SYS_gettid)
    pid_t tid;

#ifdef Q_OS_LINUX
    // 对于Linux，获取线程ID (tid)
    tid = syscall(SYS_gettid);
#else
    // 非Linux系统，此函数可能不适用或需要平台特定的实现
    Q_UNUSED(tid); // 避免编译警告
    qWarning() << "CPU affinity is Linux-specific or requires platform-specific implementation.";
    return false;
#endif

    cpu_set_t cpuset;       // 定义CPU集合
    CPU_ZERO(&cpuset);      // 清空CPU集合
    CPU_SET(cpu_id, &cpuset); // 将指定CPU核添加到集合中

    // 设置线程的CPU亲和性
    // 第一个参数是线程ID，0表示当前进程（主线程），或者特定的线程ID
    // 第二个参数是CPU集合的大小
    // 第三个参数是CPU集合
    if (sched_setaffinity(tid, sizeof(cpu_set_t), &cpuset) == -1)
    {
        qCritical() << "Failed to set CPU affinity for thread" << tid << "to CPU" << cpu_id << ":" << strerror(errno);
        return false;
    }

    qDebug() << "Thread" << tid << "successfully bound to CPU" << cpu_id;
    return true;
}

#endif
#endif // BINDTHREAD_H
