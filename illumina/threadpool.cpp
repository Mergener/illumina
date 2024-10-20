#include "threadpool.h"

namespace illumina {

ThreadPool::ThreadPool(size_t n_threads) {
    create_helpers(n_threads);
}

void ThreadPool::create_helpers(size_t n_threads) {
    m_stop = false;
    for (size_t i = 0; i < n_threads; ++i) {
        m_workers.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(m_queue_mutex);
                    m_condition.wait(lock, [this] { return m_stop || !m_tasks.empty(); });

                    if (m_stop && m_tasks.empty()) {
                        return;
                    }

                    task = std::move(m_tasks.front());
                    m_tasks.pop();
                }
                task();
            }
        });
    }
}

void ThreadPool::kill_all_helpers() {
    if (m_workers.empty()) {
        m_stop = true;
        return;
    }

    {
        std::unique_lock<std::mutex> lock(m_queue_mutex);
        m_stop = true;
    }
    m_condition.notify_all();

    for (std::thread& worker: m_workers) {
        worker.join();
    }

    m_workers.clear();
}

void ThreadPool::resize(size_t n_threads) {
    if (n_threads == m_workers.size()) {
        return;
    }

    kill_all_helpers();
    create_helpers(n_threads);
}

ThreadPool::~ThreadPool() {
    kill_all_helpers();
}

}