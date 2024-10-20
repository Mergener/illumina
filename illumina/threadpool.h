#ifndef ILLUMINA_THREAD_POOL_H
#define ILLUMINA_THREAD_POOL_H

#include <future>
#include <iostream>
#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <type_traits>

#include "debug.h"

namespace illumina {

class ThreadPool {
public:
    ThreadPool(size_t n_threads = 0);

    ~ThreadPool();

    template<class F, class... Args>
    std::future<std::invoke_result_t<F, Args&&...>> submit(F&& f, Args&&... args);

    size_t size() const;
    void resize(size_t n_threads);

private:
    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_queue_mutex;
    std::condition_variable m_condition;
    bool m_stop;

    void kill_all_helpers();
    void create_helpers(size_t n_threads);
};

template<typename F, typename... Args>
std::future<std::invoke_result_t<F, Args&&...>> ThreadPool::submit(F&& f, Args&&... args) {
    ILLUMINA_ASSERT(size() > 0);

    using TRet = decltype(f(args...));
    auto task = std::make_shared<std::packaged_task<TRet()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    std::future<TRet> result = task->get_future();
    {
        std::unique_lock<std::mutex> lock(m_queue_mutex);
        m_tasks.emplace([task]() { (*task)(); });
    }
    m_condition.notify_one();

    return result;
}

inline size_t ThreadPool::size() const {
    return m_workers.size();
}

}

#endif  // ILLUMINA_THREAD_POOL_H