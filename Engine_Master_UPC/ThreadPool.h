#pragma once

#include <vector>
#include <thread>
#include <queue>
#include <future>
#include <functional>
#include <mutex>
#include <condition_variable>

class ThreadPool
{
private:
    std::vector<std::thread> m_workers;
    std::queue<std::packaged_task<void()>> m_tasks;

    std::mutex m_queueMutex;
    std::mutex m_condMutex;
    std::condition_variable m_cond;
    std::condition_variable m_idleCond;

    bool m_finish = false;
    size_t m_activeTasks = 0;

public:
    ThreadPool();
    ~ThreadPool();

    std::future<void> submitTask(const std::function<void()>& function);
    void waitIdle();

    size_t getNumThreads() { return m_numThreads; }

private:
    void WorkerFunc(unsigned int id);

    void pushTask(std::packaged_task<void()>&& task);
    bool popTask(std::packaged_task<void()>& task);

    size_t m_numThreads;
};