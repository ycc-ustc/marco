#ifndef __MARCO_IOMANAGER_H__
#define __MARCO_IOMANAGER_H__

#include "scheduler.h"
#include "timer.h"

namespace marco {
class IOManager : public Scheduler, public TimerManager {
public:
    using ptr = std::shared_ptr<IOManager>;
    using RWMutexType = RWMutex;

    enum Event { NONE = 0x0, READ = 0x1, WRITE = 0x4 };

private:
    struct FdContext {
        using MutexType = Mutex;
        struct EventContext {
            Scheduler*            scheduler = nullptr;  // 事件执行的调度器
            Fiber::ptr            fiber;      // 事件协程
            std::function<void()> cb;         // 事件的回调函数
        };

        EventContext& getContext(Event event);

        void resetContext(EventContext& ctx);
        void triggerEvent(Event event);

        int          fd;             // 事件关联的句柄
        EventContext read;           // 读事件
        EventContext write;          // 写事件
        Event        events = NONE;  // 当前的事件
        MutexType    mutex;
    };

public:
    IOManager(size_t threads = 1, bool use_caller = true, const std::string& name = "");
    ~IOManager();

    // 1success 0 retry -1 error
    int  addEvent(int fd, Event event, std::function<void()> cb = nullptr);
    bool delEvent(int fd, Event event);
    /**
     * @brief 取消事件
     * @param[in] fd socket句柄
     * @param[in] event 事件类型
     * @attention 如果事件存在则触发事件
     */
    bool cancelEvent(int fd, Event event);

    bool              cancelAll(int fd);
    static IOManager* GetThis();
    void              onTimerInsertedAtFront();

protected:
    void tickle() override;
    bool stopping() override;
    void idle() override;
    void contextResize(size_t size);
    bool stopping(uint64_t& timeout);

private:
    /// epoll 文件句柄
    int m_epfd = 0;
    /// pipe 文件句柄
    int m_tickleFds[2];
    /// 当前等待执行的事件数量
    std::atomic<size_t> m_pendingEventCount = {0};
    /// IOManager的Mutex
    RWMutexType m_mutex;
    /// socket事件上下文的容器
    std::vector<FdContext*> m_fdContexts;
};
}  // namespace marco

#endif