#include "fiber.h"

#include "config.h"
#include "log.h"
#include "macro.h"
#include "scheduler.h"

namespace marco {

static Logger::ptr g_logger = MARCO_LOG_NAME("system");

// 静态计数器，用于生成唯一的协程ID和计数协程数量
static std::atomic<uint64_t> s_fiber_id{0};
static std::atomic<uint64_t> s_fiber_count{0};

// thread_local变量，存储当前协程和主协程的指针
static thread_local Fiber*     t_fiber = nullptr;        // 当前协程
static thread_local Fiber::ptr t_threadFiber = nullptr;  // main协程

// 获取配置项，协程栈大小，默认为128KB
static ConfigVar<uint32_t>::ptr g_fiber_stack_size =
    Config::Lookup<uint32_t>("fiber.stack_size", 128 * 1024, "fiber stack size");

// 自定义的协程栈分配器，使用malloc和free进行分配和释放
class MallocStackAllocator {
public:
    static void* Alloc(size_t size) {
        return malloc(size);
    }

    static void Dealloc(void* vp, size_t size) {
        return free(vp);
    }
};

// 使用MallocStackAllocator作为协程栈分配器
using StackAllocator = MallocStackAllocator;

// 协程类的构造函数，默认构造主协程
Fiber::Fiber() {
    m_state = EXEC;
    SetThis(this);

    if (getcontext(&m_ctx)) {
        MARCO_ASSERT2(false, "getcontext");
    }
    s_fiber_count++;
    MARCO_LOG_DEBUG(g_logger) << "Fiber::Fiber main";
}

// 协程类的构造函数，传入协程函数、栈大小和是否使用调用者栈
Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller)
    : m_id(++s_fiber_id), m_cb(cb) {
    ++s_fiber_count;
    m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();

    m_stack = StackAllocator::Alloc(m_stacksize);
    if (getcontext(&m_ctx)) {
        MARCO_ASSERT2(false, "getcontext");
    }

    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    if (!use_caller) {
        makecontext(&m_ctx, &Fiber::MainFunction, 0);
    } else {
        makecontext(&m_ctx, &Fiber::CallerMainFunction, 0);
    }
    MARCO_LOG_DEBUG(g_logger) << "Fiber::Fiber id=" << m_id;
}

// 协程类的析构函数
Fiber::~Fiber() {
    --s_fiber_count;
    if (m_stack) {
        MARCO_ASSERT(m_state == TERM || m_state == EXCEPT || m_state == INIT);
        StackAllocator::Dealloc(m_stack, m_stacksize);
    } else {  // 当前协程
        MARCO_ASSERT(!m_cb);
        MARCO_ASSERT(m_state == EXEC);

        Fiber* cur = t_fiber;
        if (cur == this) {
            SetThis(nullptr);
        }
    }
    MARCO_LOG_DEBUG(g_logger) << "Fiber::~Fiber id=" << m_id << " total=" << s_fiber_count;
}

// 重置协程的函数和状态
void Fiber::reset(std::function<void()> cb) {
    MARCO_ASSERT(m_stack);
    MARCO_ASSERT(m_state == TERM || m_state == EXCEPT || m_state == INIT);

    m_cb = cb;
    if (getcontext(&m_ctx)) {
        MARCO_ASSERT2(false, "getcontext");
    }

    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    makecontext(&m_ctx, &Fiber::MainFunction, 0);
    m_state = INIT;
}

// 切换到当前协程的执行
void Fiber::call() {
    SetThis(this);
    m_state = EXEC;
    if (swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
        MARCO_ASSERT2(false, "swapcontext");
    }
}

// 切换到当前协程的执行，供调度器使用
void Fiber::swapIn() {
    SetThis(this);
    MARCO_ASSERT(m_state != EXEC)
    m_state = EXEC;

    if (swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx)) {
        MARCO_ASSERT2(false, "swapcontext");
    }
}

// 切出当前协程的执行，返回到调度器
void Fiber::swapOut() {
    SetThis(Scheduler::GetMainFiber());

    if (swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) {
        MARCO_ASSERT2(false, "swapcontext");
    }
}

// 切换到主协程执行，让出当前协程的执行
void Fiber::back() {
    SetThis(t_threadFiber.get());
    if (swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
        MARCO_ASSERT2(false, "swapcontext");
    }
}

// 设置当前线程的当前协程指针
void Fiber::SetThis(Fiber* f) {
    t_fiber = f;
}

// 获取当前线程的当前协程指针
Fiber::ptr Fiber::GetThis() {
    if (t_fiber) {
        return t_fiber->shared_from_this();
    }
    Fiber::ptr main_fiber(new Fiber);
    MARCO_ASSERT(t_fiber == main_fiber.get());
    t_threadFiber = main_fiber;
    return t_fiber->shared_from_this();
}

// 将当前协程切换到READY状态，以便其他协程可以执行
void Fiber::YieldToReady() {
    Fiber::ptr cur = GetThis();
    MARCO_ASSERT(cur->m_state == EXEC);
    cur->m_state = READY;
    cur->swapOut();
}

// 将当前协程切换到HOLD状态，以便其他协程可以执行
void Fiber::YieldToHold() {
    Fiber::ptr cur = GetThis();
    MARCO_ASSERT(cur->m_state == EXEC);
    cur->m_state = HOLD;
    cur->swapOut();
}

// 获取总协程数量
uint64_t Fiber::TotalFibers() {
    return s_fiber_count;
}

// 获取当前协程的ID
uint64_t Fiber::GetFiberId() {
    if (t_fiber) {
        return t_fiber->getId();
    }
    return 0;
}

// 协程主函数，执行协程函数并处理异常
void Fiber::MainFunction() {
    Fiber::ptr cur = GetThis();
    MARCO_ASSERT(cur);
    try {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    } catch (std::exception& ex) {
        cur->m_state = EXCEPT;
        MARCO_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what() << " fiber_id=" << cur->getId()
                                  << std::endl
                                  << marco::BacktraceToString();
    } catch (...) {
        cur->m_state = EXCEPT;
        MARCO_LOG_ERROR(g_logger) << "Fiber Except"
                                  << " fiber_id=" << cur->getId() << std::endl
                                  << marco::BacktraceToString();
    }

    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->swapOut();

    // swapout后直接析构了 下面的语句永远不会执行
    MARCO_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}

// 调用者主函数，与MainFunction类似
void Fiber::CallerMainFunction() {
    Fiber::ptr cur = GetThis();
    MARCO_ASSERT(cur);
    try {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    } catch (std::exception& ex) {
        cur->m_state = EXCEPT;
        MARCO_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what() << " fiber_id=" << cur->getId()
                                  << std::endl
                                  << marco::BacktraceToString();
    } catch (...) {
        cur->m_state = EXCEPT;
        MARCO_LOG_ERROR(g_logger) << "Fiber Except"
                                  << " fiber_id=" << cur->getId() << std::endl
                                  << marco::BacktraceToString();
    }

    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->back();
    MARCO_ASSERT2(false, "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}

}  // namespace marco
