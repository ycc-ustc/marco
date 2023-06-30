#ifndef __MARCO_SCHEDULER_H__
#define __MARCO_SCHEDULER_H__

#include <iostream>
#include <list>
#include <memory>
#include <vector>

#include "fiber.h"
#include "thread.h"

namespace marco {
class Scheduler {
public:
    using ptr = std::shared_ptr<Scheduler>;
    using MutexType = Mutex;
    Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name = "");
    virtual ~Scheduler();

    const std::string& getName() {
        return m_name;
    }

    static Scheduler* GetThis();
    static Fiber*     GetMainFiber();

    void start();
    void stop();

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
    virtual void tickle();
    void         run();
    virtual bool stopping();
    void         setThis();
    virtual void idle();

private:
    template <class FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc, int thread) {
        bool           need_tickle = m_fibers.empty();
        FiberAndThread ft(fc, thread);
        if (ft.fiber || ft.cb) {
            m_fibers.push_back(ft);
        }
        return need_tickle;
    }

private:
    struct FiberAndThread {
        Fiber::ptr            fiber;
        std::function<void()> cb;
        int                   thread;

        FiberAndThread(Fiber::ptr f, int thr) : fiber(f), thread(thr) {}

        FiberAndThread(Fiber::ptr* f, int thr) : thread(thr) {
            fiber.swap(*f);
        }

        FiberAndThread(std::function<void()> f, int thr) : cb(f), thread(thr) {
            cb.swap(f);
        }

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
    MutexType                 m_mutex;
    std::vector<Thread::ptr>  m_threads;
    std::list<FiberAndThread> m_fibers;
    std::string               m_name;
    Fiber::ptr                m_rootFiber;

protected:
    std::vector<int>    m_threadIds;
    size_t              m_threadCount = 0;
    std::atomic<size_t> m_activeThreadCount = {0};
    std::atomic<size_t> m_idleThreadCount = {0};
    bool                m_stopping = true;
    bool                m_autoStop = false;
    int                 m_rootThread = 0;
};
}  // namespace marco

#endif