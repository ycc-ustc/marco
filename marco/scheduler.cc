#include "scheduler.h"

#include "hook.h"
#include "log.h"
#include "macro.h"

namespace marco {

static marco::Logger::ptr g_logger = MARCO_LOG_NAME("system");  // 系统日志

static thread_local Scheduler* t_scheduler = nullptr;        // 当前线程的Scheduler
static thread_local Fiber*     t_scheduler_fiber = nullptr;  // 当前线程的主Fiber

// Scheduler构造函数
// threads:线程数量
// use_caller:是否使用调用者所在线程
// name:调度器名称
Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name) : m_name(name) {
    MARCO_ASSERT(threads > 0);

    if (use_caller) {
        // 获取当前线程的主Fiber
        marco::Fiber::GetThis();

        // 线程数减1,不再创建当前线程
        --threads;
        MARCO_ASSERT(GetThis() == nullptr);
        t_scheduler = this;

        // 主Fiber执行调度器的run函数
        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        marco::Thread::SetName(name);

        t_scheduler_fiber = m_rootFiber.get();
        m_rootThread = marco::GetThreadId();
        m_threadIds.push_back(m_rootThread);
    } else {
        m_rootThread = -1;
    }
    m_threadCount = threads;  // 线程数量
}

// 销毁时清理
Scheduler::~Scheduler() {
    MARCO_ASSERT(m_stopping);
    if (GetThis() == this) {
        t_scheduler = nullptr;
    }
}

// 获取当前线程的Scheduler
Scheduler* Scheduler::GetThis() {
    return t_scheduler;
}

// 获取当前线程的主Fiber
Fiber* Scheduler::GetMainFiber() {
    return t_scheduler_fiber;
}

// 启动所有IO线程
void Scheduler::start() {
    MutexType::Lock lock(m_mutex);
    if (!m_stopping) {
        return;
    }
    m_stopping = false;
    // 创建m_threadCount数量的线程
    m_threads.resize(m_threadCount);
    for (size_t i = 0; i < m_threadCount; i++) {
        m_threads[i].reset(
            new Thread(std::bind(&Scheduler::run, this), m_name + "_" + std::to_string(i)));
        m_threadIds.push_back(m_threads[i]->getId());
    }
    lock.unlock();
}

// 停止所有线程
void Scheduler::stop() {
    m_autoStop = true;
    // 等待任务执行完毕
    if (m_rootFiber && m_threadCount == 0 && m_rootFiber->getState() == Fiber::TERM) {
        m_stopping = true;

        if (stopping()) {
            return;
        }
    }

    // 设置停止状态
    m_stopping = true;

    // 通知所有线程
    for (size_t i = 0; i < m_threadCount; i++) {
        tickle();
    }

    // 如果有root fiber则调用让其结束
    if (m_rootFiber) {
        if (!stopping()) {
            m_rootFiber->call();
        }
    }

    // 等待所有线程结束
    std::vector<Thread::ptr> thrs;
    thrs.swap(m_threads);

    for (auto& i : thrs) {
        i->join();
    }
}

// 设置当前scheduler
void Scheduler::setThis() {
    t_scheduler = this;
}

// 调度器运行函数
void Scheduler::run() {
    set_hook_enable(true);
    setThis();

    // 当前线程管理的Fiber
    if (marco::GetThreadId() != m_rootThread) {
        t_scheduler_fiber = Fiber::GetThis().get();
    }

    // 空闲Fiber
    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));  // 没有线程执行时执行idle
    Fiber::ptr cb_fiber;

    // 存储要执行的任务
    FiberAndThread ft;

    while (true) {
        ft.reset();

        bool tickle_me = false;
        bool is_active = false;

        // 获取一个待执行的fiber
        {
            MutexType::Lock lock(m_mutex);
            auto            it = m_fibers.begin();
            while (it != m_fibers.end()) {
                // threadId != -1是use_caller 限制执行进程
                if (it->thread != -1 && it->thread != marco::GetThreadId()) {
                    ++it;
                    tickle_me = true;
                    continue;
                }

                // 要么执行fiber要么执行callback
                MARCO_ASSERT(it->fiber || it->cb);
                if (it->fiber && it->fiber->getState() == Fiber::EXEC) {
                    ++it;
                    continue;
                }
                ft = *it;
                m_fibers.erase(it++);
                ++m_activeThreadCount;
                is_active = true;
                break;
            }
            tickle_me |= it != m_fibers.end();
        }

        if (tickle_me) {
            tickle();
        }

        // 执行fiber
        if (ft.fiber && ft.fiber->getState() != Fiber::TERM &&
            ft.fiber->getState() != Fiber::EXCEPT) {
            ft.fiber->swapIn();
            --m_activeThreadCount;

            if (ft.fiber->getState() == Fiber::READY) {
                schedule(ft.fiber);
            } else if (ft.fiber->getState() != Fiber::TERM &&
                       ft.fiber->getState() != Fiber::EXCEPT) {
                ft.fiber->m_state = Fiber::HOLD;
            }

            ft.reset();
        } else if (ft.cb) {
            // 执行callback

            // cb_fiber在前面定义
            if (cb_fiber) {
                cb_fiber->reset(ft.cb);
            } else {
                cb_fiber.reset(new Fiber(ft.cb));
            }
            ft.reset();

            cb_fiber->swapIn();
            --m_activeThreadCount;

            if (cb_fiber->getState() == Fiber::READY) {
                schedule(cb_fiber);
                cb_fiber.reset();
            } else if (cb_fiber->getState() == Fiber::EXCEPT ||
                       cb_fiber->getState() == Fiber::TERM) {
                cb_fiber->reset(nullptr);
            } else {
                cb_fiber->m_state = Fiber::HOLD;
                cb_fiber.reset();
            }
        } else {
            if (is_active) {
                --m_activeThreadCount;
                continue;
            }

            // 执行空闲任务
            if (idle_fiber->getState() == Fiber::TERM) {
                MARCO_LOG_INFO(g_logger) << "idle fiber term";
                break;
            }

            ++m_idleThreadCount;
            idle_fiber->swapIn();
            --m_idleThreadCount;

            if (idle_fiber->getState() != Fiber::TERM && idle_fiber->getState() != Fiber::EXCEPT) {
                idle_fiber->m_state = Fiber::HOLD;
            }
        }
    }
}

// 判断是否所有任务都结束了
bool Scheduler::stopping() {
    MutexType::Lock lock(m_mutex);
    return m_autoStop && m_stopping && m_fibers.empty() && m_activeThreadCount == 0;
}

// 空闲时执行
void Scheduler::idle() {
    while (!stopping()) {
        marco::Fiber::YieldToHold();
    }
}

// 唤醒其他线程
void Scheduler::tickle() {}

}  // namespace marco
