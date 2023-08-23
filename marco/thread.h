#ifndef __MARCO_THREAD_H__
#define __MARCO_THREAD_H__

#include "mutex.h"
#include "noncopyable.h"

namespace marco {

class Thread : Noncopyable {
public:
    using ptr = std::shared_ptr<Thread>;
    Thread(std::function<void()> cb, const std::string& name);
    ~Thread();

    pid_t getId() const {
        return m_id;
    }
    const std::string& getName() const {
        return m_name;
    }

    static void SetName(const std::string& name);

    void join();

    static Thread*            GetThis();
    static const std::string& GetName();

private:
    Thread(const Thread&) = delete;
    Thread(const Thread&&) = delete;
    Thread& operator=(const Thread&) = delete;

    static void* run(void* args);

private:
    pid_t                 m_id = -1;
    pthread_t             m_thread = 0;
    std::function<void()> m_cb;
    std::string           m_name;
    Semaphore             m_semaphore;
};
}  // namespace marco

#endif