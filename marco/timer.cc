#include "timer.h"

#include "util.h"

namespace marco {

// 定时器比较仿函数的实现
bool Timer::Comparator::operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const {
    // 对智能指针进行比较，首先处理为空的情况
    if (!lhs && !rhs) {
        return false;
    }
    if (!lhs) {
        return true;
    }
    if (!rhs) {
        return false;
    }
    // 比较定时器的执行时间
    if (lhs->m_next < rhs->m_next) {
        return true;
    } else if (rhs->m_next < lhs->m_next) {
        return false;
    } else {
        // 如果执行时间相同，比较定时器的地址
        return lhs.get() < rhs.get();
    }
}

// 构造函数：从间隔时间(ms)、回调函数(cb)、是否循环定时器(recurring)和定时器管理器(manager)构建定时器
Timer::Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager* manager)
    : m_recurring(recurring), m_ms(ms), m_cb(cb), m_manager(manager) {
    m_next = marco::GetCurrentMS() + m_ms;  // 计算下一次执行时间
}

// 构造函数：从下一个执行时间戳(next)构建定时器
Timer::Timer(uint64_t next) : m_next(next) {}

// 取消定时器：将定时器从定时器管理器中移除
bool Timer::cancel() {
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if (m_cb) {
        m_cb = nullptr;
        auto it = m_manager->m_timers.find(shared_from_this());
        if (it != m_manager->m_timers.end()) {
            m_manager->m_timers.erase(it);
            return true;
        }
    }
    return false;
}

// 刷新定时器：更新定时器的执行时间为当前时间加上间隔时间，并重新插入定时器管理器
bool Timer::refresh() {
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if (!m_cb) {
        return false;
    }
    auto it = m_manager->m_timers.find(shared_from_this());
    if (it == m_manager->m_timers.end()) {
        return false;
    }
    m_manager->m_timers.erase(it);
    // 更新执行时间，并重新插入定时器管理器
    m_next = marco::GetCurrentMS() + m_ms;
    m_manager->m_timers.insert(shared_from_this());
    return true;
}

// 重置定时器时间：修改定时器的间隔时间，并根据 from_now 参数重新计算下一次执行时间
bool Timer::reset(uint64_t ms, bool from_now) {
    if (ms == m_ms && !from_now) {
        return true;
    }
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if (!m_cb) {
        return false;
    }
    auto it = m_manager->m_timers.find(shared_from_this());
    if (it == m_manager->m_timers.end()) {
        return false;
    }
    m_manager->m_timers.erase(it);
    uint64_t start = 0;
    if (from_now) {
        start = marco::GetCurrentMS();
    } else {
        start = m_next - m_ms;
    }
    m_ms = ms;
    m_next = start + m_ms;
    m_manager->addTimer(shared_from_this(), lock);  // 重新插入定时器管理器
    return true;
}

// 构造函数：初始化定时器管理器，记录上次执行时间
TimerManager::TimerManager() {
    m_previouseTime = marco::GetCurrentMS();
}

// 析构函数：空实现
TimerManager::~TimerManager() {}

// 添加定时器：从指定的时间间隔(ms)、回调函数(cb)和是否循环(recurring)创建定时器，并添加到定时器管理器中
Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb, bool recurring) {
    Timer::ptr             timer(new Timer(ms, cb, recurring, this));
    RWMutexType::WriteLock lock(m_mutex);
    addTimer(timer, lock);
    return timer;
}

// 回调函数：在满足条件时执行回调函数，由条件定时器使用
static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb) {
    std::shared_ptr<void> tmp = weak_cond.lock();
    if (tmp) {
        cb();
    }
}

// 添加条件定时器：从指定的时间间隔(ms)、回调函数(cb)、条件(weak_cond)和是否循环(recurring)创建定时器，并添加到定时器管理器中
Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb,
                                           std::weak_ptr<void> weak_cond, bool recurring) {
    return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recurring);
}

// 获取下一个定时器执行的时间间隔(毫秒)
uint64_t TimerManager::getNextTimer() {
    RWMutexType::ReadLock lock(m_mutex);
    m_tickled = false;
    if (m_timers.empty()) {
        return ~0ull;  // 无定时器时返回最大值
    }
    const Timer::ptr& next = *m_timers.begin();  // 获取最近的定时器
    uint64_t          now_ms = marco::GetCurrentMS();
    if (now_ms >= next->m_next) {
        return 0;  // 当前时间已经超过定时器执行时间，返回0
    } else {
        return next->m_next - now_ms;  // 返回下一个定时器执行的时间间隔
    }
}

// 获取需要执行的定时器的回调函数列表
void TimerManager::listExpiredCb(std::vector<std::function<void()>>& cbs) {
    uint64_t                now_ms = marco::GetCurrentMS();
    std::vector<Timer::ptr> expired;
    {
        RWMutexType::ReadLock lock(m_mutex);
        if (m_timers.empty()) {
            return;
        }
    }
    RWMutexType::WriteLock lock(m_mutex);
    if (m_timers.empty()) {
        return;
    }
    bool rollover = detectClockRollover(now_ms);
    // 判断是否需要执行回调函数
    if (!rollover && ((*m_timers.begin())->m_next > now_ms)) {
        return;
    }
    Timer::ptr now_timer(new Timer(now_ms));
    auto       it = rollover ? m_timers.end() : m_timers.lower_bound(now_timer);
    while (it != m_timers.end() && (*it)->m_next == now_ms) {
        ++it;
    }
    expired.insert(expired.begin(), m_timers.begin(), it);
    m_timers.erase(m_timers.begin(), it);
    cbs.reserve(expired.size());

    // 将过期的定时器的回调函数添加到列表中
    for (auto& timer : expired) {
        cbs.push_back(timer->m_cb);
        if (timer->m_recurring) {
            timer->m_next = now_ms + timer->m_ms;  // 更新循环定时器的下一次执行时间
            m_timers.insert(timer);
        } else {
            timer->m_cb = nullptr;  // 非循环定时器回调函数置空
        }
    }
}

// 添加定时器到定时器管理器中
void TimerManager::addTimer(Timer::ptr val, RWMutexType::WriteLock& lock) {
    auto it = m_timers.insert(val).first;
    bool at_front = (it == m_timers.begin()) && !m_tickled;
    if (at_front) {
        m_tickled = true;
    }
    lock.unlock();

    if (at_front) {
        onTimerInsertedAtFront();
    }
}

// 检测系统时间是否发生了回卷
bool TimerManager::detectClockRollover(uint64_t now_ms) {
    bool rollover = false;
    // 如果当前时间小于上次执行时间且小于上次执行时间减去1小时，判断为回卷
    if (now_ms < m_previouseTime && now_ms < (m_previouseTime - 60 * 60 * 1000)) {
        rollover = true;
    }
    m_previouseTime = now_ms;
    return rollover;
}

// 判断是否有定时器
bool TimerManager::hasTimer() {
    RWMutexType::ReadLock lock(m_mutex);
    return !m_timers.empty();
}

}  // namespace marco
