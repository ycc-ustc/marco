#include "iomanager.h"

#include <fcntl.h>
#include <sys/epoll.h>

#include "macro.h"

namespace marco {

static marco::Logger::ptr g_logger = MARCO_LOG_NAME("system");

enum EpollCtlOp {};

static std::ostream& operator<<(std::ostream& os, const EpollCtlOp& op) {
    switch ((int)op) {
#define XX(ctl)                                                                                    \
    case ctl:                                                                                      \
        return os << #ctl;
        XX(EPOLL_CTL_ADD);
        XX(EPOLL_CTL_MOD);
        XX(EPOLL_CTL_DEL);
    default:
        return os << (int)op;
    }
#undef XX
}

static std::ostream& operator<<(std::ostream& os, EPOLL_EVENTS events) {
    if (!events) {
        return os << "0";
    }
    bool first = true;
#define XX(E)                                                                                      \
    if (events & E) {                                                                              \
        if (!first) {                                                                              \
            os << "|";                                                                             \
        }                                                                                          \
        os << #E;                                                                                  \
        first = false;                                                                             \
    }
    XX(EPOLLIN);
    XX(EPOLLPRI);
    XX(EPOLLOUT);
    XX(EPOLLRDNORM);
    XX(EPOLLRDBAND);
    XX(EPOLLWRNORM);
    XX(EPOLLWRBAND);
    XX(EPOLLMSG);
    XX(EPOLLERR);
    XX(EPOLLHUP);
    XX(EPOLLRDHUP);
    XX(EPOLLONESHOT);
    XX(EPOLLET);
#undef XX
    return os;
}

IOManager::FdContext::EventContext& IOManager::FdContext::getContext(IOManager::Event event) {
    switch (event) {
    case IOManager::READ:
        return read;
    case IOManager::WRITE:
        return write;
    default:
        MARCO_ASSERT2(false, "getContext");
    }
    throw std::invalid_argument("getContext invalid event");
}

IOManager::IOManager(size_t threads, bool use_caller, const std::string& name)
    : Scheduler(threads, use_caller, name) {
    m_epfd = epoll_create(100);
    MARCO_ASSERT(m_epfd > 0);

    int rt = pipe(m_tickleFds);
    MARCO_ASSERT(!rt);

    epoll_event event;
    memset(&event, 0, sizeof(epoll_event));
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = m_tickleFds[0];

    rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
    MARCO_ASSERT(!rt);

    rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
    MARCO_ASSERT(!rt);

    contextResize(32);

    start();
}

void IOManager::contextResize(size_t size) {
    m_fdContexts.resize(size);

    for (size_t i = 0; i < m_fdContexts.size(); ++i) {
        if (!m_fdContexts[i]) {
            m_fdContexts[i] = new FdContext;
            m_fdContexts[i]->fd = i;
        }
    }
}

IOManager::~IOManager() {
    stop();
    close(m_epfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);

    for (size_t i = 0; i < m_fdContexts.size(); i++) {
        if (m_fdContexts[i]) {
            delete m_fdContexts[i];
        }
    }
}

// 添加成功返回0,失败返回-1
int IOManager::addEvent(int fd, Event event, std::function<void()> cb) {
    FdContext*            fd_ctx = nullptr;
    RWMutexType::ReadLock lock(m_mutex);
    if (static_cast<int>(m_fdContexts.size()) > fd) {
        fd_ctx = m_fdContexts[fd];
        lock.unlock();
    } else {  // 需要扩容
        lock.unlock();
        RWMutexType::WriteLock lock2(m_mutex);
        contextResize(fd * 1.5);
        fd_ctx = m_fdContexts[fd];
    }

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if (MARCO_UNLIKELY(fd_ctx->events & event)) {  // 已有事件和新加的事件不一致
        MARCO_LOG_ERROR(g_logger) << "addEvent assert fd=" << fd << " event=" << (EPOLL_EVENTS)event
                                  << " fd_ctx.event=" << (EPOLL_EVENTS)fd_ctx->events;
        MARCO_ASSERT(!(fd_ctx->events & event));
    }

    int         op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event epevent;
    epevent.events = EPOLLET | fd_ctx->events | event;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        MARCO_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", " << (EpollCtlOp)op << ", " << fd
                                  << ", " << (EPOLL_EVENTS)epevent.events << "):" << rt << " ("
                                  << errno << ") (" << strerror(errno)
                                  << ") fd_ctx->events=" << (EPOLL_EVENTS)fd_ctx->events;
        return -1;
    }

    m_pendingEventCount++;
    fd_ctx->events = (Event)(fd_ctx->events | event);

    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    MARCO_ASSERT(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb);

    event_ctx.scheduler = Scheduler::GetThis();
    if (cb) {
        event_ctx.cb.swap(cb);
    } else {
        event_ctx.fiber = Fiber::GetThis();
        MARCO_ASSERT2(event_ctx.fiber->getState() == Fiber::EXEC,
                      "state=" + event_ctx.fiber->getState());
    }
    return 0;
}

void IOManager::FdContext::resetContext(EventContext& ctx) {
    ctx.scheduler = nullptr;
    ctx.fiber.reset();
    ctx.cb = nullptr;
}

bool IOManager::delEvent(int fd, Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    if (static_cast<int>(m_fdContexts.size()) <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if (MARCO_UNLIKELY(!(fd_ctx->events & event))) {
        return false;
    }

    auto        new_events = static_cast<Event>(fd_ctx->events & ~event);
    int         op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent{};
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        MARCO_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", " << static_cast<EpollCtlOp>(op)
                                  << ", " << fd << ", " << static_cast<EPOLL_EVENTS>(epevent.events)
                                  << "):" << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    --m_pendingEventCount;
    fd_ctx->events = new_events;
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    fd_ctx->resetContext(event_ctx);
    return true;
}

void IOManager::FdContext::triggerEvent(IOManager::Event event) {
    MARCO_ASSERT(events & event);
    events = static_cast<Event>(events & ~event);
    EventContext& ctx = getContext(event);
    if (ctx.cb) {
        ctx.scheduler->schedule(&ctx.cb);
    } else {
        ctx.scheduler->schedule(&ctx.fiber);
    }
    ctx.scheduler = nullptr;
    return;
}

bool IOManager::cancelEvent(int fd, Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    if ((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if (MARCO_UNLIKELY(!(fd_ctx->events & event))) {
        return false;
    }

    auto        new_events = static_cast<Event>(fd_ctx->events & ~event);
    int         op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent{};
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        MARCO_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", " << static_cast<EpollCtlOp>(op)
                                  << ", " << fd << ", " << static_cast<EPOLL_EVENTS>(epevent.events)
                                  << "):" << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    fd_ctx->triggerEvent(event);
    --m_pendingEventCount;
    return true;
}

bool IOManager::cancelAll(int fd) {
    RWMutexType::ReadLock lock(m_mutex);
    if (static_cast<int>(m_fdContexts.size()) <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if (!fd_ctx->events) {
        return false;
    }

    int         op = EPOLL_CTL_DEL;
    epoll_event epevent{};
    epevent.events = 0;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        MARCO_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", " << static_cast<EpollCtlOp>(op)
                                  << ", " << fd << ", " << static_cast<EPOLL_EVENTS>(epevent.events)
                                  << "):" << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    if (fd_ctx->events & READ) {
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
    }
    if (fd_ctx->events & WRITE) {
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
    }

    MARCO_ASSERT(fd_ctx->events == 0);
    return true;
}

IOManager* IOManager::GetThis() {
    return dynamic_cast<IOManager*>(Scheduler::GetThis());
}

void IOManager::tickle() {
    if (!hasIdleThreads()) {
        return;
    }
    int rt = write(m_tickleFds[1], "T", 1);
    MARCO_ASSERT(rt == 1);
}

bool IOManager::stopping(uint64_t& timeout) {
    timeout = getNextTimer();
    return timeout == ~0ull && m_pendingEventCount == 0 && Scheduler::stopping();
}

bool IOManager::stopping() {
    uint64_t timeout = 0;
    return stopping(timeout);
}

void IOManager::idle() {
    MARCO_LOG_DEBUG(g_logger) << "idle";
    const uint64_t MAX_EVNETS = 256;
    epoll_event*   events = new epoll_event[MAX_EVNETS]();
    // 变量不实际使用，作用是离开作用域可以释放events
    std::shared_ptr<epoll_event> shared_events(events, [](epoll_event* ptr) { delete[] ptr; });

    while (true) {
        uint64_t next_timeout = 0;
        if (MARCO_UNLIKELY(stopping(next_timeout))) {
            MARCO_LOG_INFO(g_logger) << "name=" << getName() << " idle stopping exit";
            break;
        }

        int rt = 0;
        do {
            static const int MAX_TIMEOUT = 3000;
            // 根据 next_timeout 更新等待时间，并确保不超过最大等待时间
            if (next_timeout != ~0ull) {
                next_timeout =
                    static_cast<int>(next_timeout) > MAX_TIMEOUT ? MAX_TIMEOUT : next_timeout;
            } else {
                next_timeout = MAX_TIMEOUT;
            }
            rt = epoll_wait(m_epfd, events, MAX_EVNETS, static_cast<int>(next_timeout));
            if (rt < 0 && errno == EINTR) {
            } else {
                break;
            }
        } while (true);

        std::vector<std::function<void()>> cbs;
        listExpiredCb(cbs);
        if (!cbs.empty()) {
            // MARCO_LOG_DEBUG(g_logger) << "on timer cbs.size=" << cbs.size();
            schedule(cbs.begin(), cbs.end());
            cbs.clear();
        }

        for (int i = 0; i < rt; ++i) {
            epoll_event& event = events[i];
            // 如果是触发通知的事件，读取数据并继续
            if (event.data.fd == m_tickleFds[0]) {
                uint8_t dummy[256];
                while (read(m_tickleFds[0], dummy, sizeof(dummy)) > 0) {
                }
                continue;
            }

            auto*                      fd_ctx = static_cast<FdContext*>(event.data.ptr);
            FdContext::MutexType::Lock lock(fd_ctx->mutex);
            if (event.events & (EPOLLERR | EPOLLHUP)) {
                event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events;
            }
            int real_events = NONE;
            if (event.events & EPOLLIN) {
                real_events |= READ;
            }
            if (event.events & EPOLLOUT) {
                real_events |= WRITE;
            }

            if ((fd_ctx->events & real_events) == NONE) {
                continue;
            }

            int left_events = (fd_ctx->events & ~real_events);
            int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            event.events = EPOLLET | left_events;

            int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
            if (rt2) {
                MARCO_LOG_ERROR(g_logger)
                    << "epoll_ctl(" << m_epfd << ", " << static_cast<EpollCtlOp>(op) << ", "
                    << fd_ctx->fd << ", " << static_cast<EPOLL_EVENTS>(event.events) << "):" << rt2
                    << " (" << errno << ") (" << strerror(errno) << ")";
                continue;
            }

            // MARCO_LOG_INFO(g_logger) << " fd=" << fd_ctx->fd << " events=" << fd_ctx->events
            //                         << " real_events=" << real_events;
            if (real_events & READ) {
                fd_ctx->triggerEvent(READ);
                --m_pendingEventCount;
            }
            if (real_events & WRITE) {
                fd_ctx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }
        }

        Fiber::ptr cur = Fiber::GetThis();
        auto       raw_ptr = cur.get();
        cur.reset();

        raw_ptr->swapOut();
    }
}

void IOManager::onTimerInsertedAtFront() {
    tickle();
}
}  // namespace marco