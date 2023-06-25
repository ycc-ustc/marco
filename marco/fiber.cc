#include "fiber.h"

#include "config.h"
#include "macro.h"

namespace marco {

static std::atomic<uint64_t> s_fiber_id{0};
static std::atomic<uint64_t> s_fiber_count{0};

static thread_local Fiber*     t_fiber = nullptr;        // 当前协程
static thread_local Fiber::ptr t_threadFiber = nullptr;  // main协程

static ConfigVar<uint32_t>::ptr g_fiber_stack_size =
    Config::Lookup<uint32_t>("fiber.stack_size", 128 * 1024, "fiber stack size");  // 协程栈大小

class MallocStackAllocator {
public:
    static void* Alloc(size_t size) {
        return malloc(size);
    }

    static void Dealloc(void* vp, size_t size) {
        return free(vp);
    }
};

using StackAllocator = MallocStackAllocator;

Fiber::Fiber() {
    m_state = EXEC;
    setThis(this);

    if (getcontext(&m_ctx)) {
        MARCO_ASSERT2(false, "getcontext");
    }
    s_fiber_count++;
}

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

    makecontext(&m_ctx, &Fiber::MainFunction, 0);
}

Fiber::~Fiber() {
    --s_fiber_count;
    if (m_stack) {
        MARCO_ASSERT(m_state == TERM || m_state == EXCEPT || m_state == INIT);
        StackAllocator::Dealloc(m_stack, m_stacksize);
    } else { // 主协程
        MARCO_ASSERT(!m_cb);
        MARCO_ASSERT(m_state == EXEC);

        Fiber* cur = t_fiber;
        if (cur == this) {
            setThis(nullptr);
        }
    }
}

// 重置协程函数 并重置状态
void Fiber::reset(std::function<void()> cb) {}

void call() {}

void Fiber::swapIn() {}
void Fiber::swapOut() {}
void Fiber::setThis(Fiber* f) {
    t_fiber = f;
}
Fiber::ptr Fiber::GetThis() {}

void Fiber::YieldToReady() {}

void     Fiber::YieldToHold() {}
uint64_t Fiber::TotalFibers() {}
void     Fiber::MainFunction() {}
}  // namespace marco