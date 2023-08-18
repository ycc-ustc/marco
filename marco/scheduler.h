#ifndef __MARCO_SCHEDULER_H__
#define __MARCO_SCHEDULER_H__

#include <iostream>
#include <list>
#include <memory>
#include <vector>

#include "fiber.h"
#include "thread.h"

namespace marco {

// 协程调度器类
class Scheduler {
public:
    using ptr = std::shared_ptr<Scheduler>;
    using MutexType = Mutex;

    // 构造函数：创建协程调度器，可指定线程数量和是否使用调用者所在线程
    Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "");

    // 析构函数
    virtual ~Scheduler();

    // 获取调度器名称
    const std::string& getName() {
        return m_name;
    }

    // 获取当前协程调度器对象
    static Scheduler* GetThis();

    // 获取主协程对象
    static Fiber* GetMainFiber();

    // 启动调度器
    void start();

    // 停止调度器
    void stop();

    // 调度一个协程
    template <class FiberOrCb>
    void schedule(FiberOrCb fc, int thread = -1) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            need_tickle = scheduleNoLock(fc, thread);
        }
        if (need_tickle) {
            tickle();
        }
    }

    // 批量调度协程
    template <class InputIterator>
    void schedule(InputIterator begin, InputIterator end) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            while (begin != end) {
                need_tickle = scheduleNoLock(&*begin, -1) || need_tickle;
                ++begin;
            }
        }
        if (need_tickle) {
            tickle();
        }
    }

protected:
    // 触发线程执行协程
    virtual void tickle();

    // 协程执行函数
    void run();

    // 是否停止调度器
    virtual bool stopping();

    // 设置当前协程调度器对象
    void setThis();

    // 空闲状态处理函数
    virtual void idle();

    // 判断是否有空闲线程
    bool hasIdleThreads() {
        return m_idleThreadCount > 0;
    }

private:
    template <class FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc, int thread) {
        // 检查任务队列是否为空，以判断是否需要唤醒调度器
        bool need_tickle = m_fibers.empty();

        // 创建一个 FiberAndThread 结构体，用于存储任务和线程信息
        FiberAndThread ft(fc, thread);

        // 如果任务是协程或回调函数，则将其添加到任务队列中
        if (ft.fiber || ft.cb) {
            m_fibers.push_back(ft);
        }

        // 返回是否需要唤醒调度器的标志
        return need_tickle;
    }

private:
    // 内部结构：协程和线程的对应关系
    struct FiberAndThread {
        Fiber::ptr            fiber;   // 协程指针
        std::function<void()> cb;      // 回调函数
        int                   thread;  // 线程编号

        FiberAndThread(Fiber::ptr f, int thr) : fiber(f), thread(thr) {}

        FiberAndThread(Fiber::ptr* f, int thr) : thread(thr) {
            fiber.swap(*f);
        }

        FiberAndThread(std::function<void()> f, int thr) : cb(f), thread(thr) {}

        FiberAndThread(std::function<void()>* f, int thr) : thread(thr) {
            cb.swap(*f);
        }

        FiberAndThread() : thread(-1) {}

        void reset() {
            fiber = nullptr;
            cb = nullptr;
            thread = -1;
        }
    };

private:
    MutexType                 m_mutex;      // 互斥锁，保证线程安全
    std::vector<Thread::ptr>  m_threads;    // 线程数组
    std::list<FiberAndThread> m_fibers;     // 协程列表
    std::string               m_name;       // 调度器名称
    Fiber::ptr                m_rootFiber;  // 主协程

protected:
    std::vector<int>    m_threadIds;                // 线程编号数组
    size_t              m_threadCount = 0;          // 线程数量
    std::atomic<size_t> m_activeThreadCount = {0};  // 活跃线程数量
    std::atomic<size_t> m_idleThreadCount = {0};    // 空闲线程数量
    bool                m_stopping = true;          // 是否停止
    bool                m_autoStop = false;         // 是否自动停止
    int                 m_rootThread = 0;           // 主线程编号
};

}  // namespace marco

#endif
