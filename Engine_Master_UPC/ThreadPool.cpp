#include "ThreadPool.h"

ThreadPool::ThreadPool()
{
    size_t numThreads = std::thread::hardware_concurrency();
    m_workers.reserve(numThreads);

    for (unsigned int i = 0; i < numThreads; ++i)
    {
        m_workers.emplace_back(std::bind(&ThreadPool::WorkerFunc, this, i));
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::lock_guard<std::mutex> lock(m_condMutex);
        m_finish = true;
    }

    m_cond.notify_all();

    for (std::thread& th : m_workers)
    {
        if (th.joinable())
        {
            th.join();
        }
    }

    m_idleCond.notify_all();
}

void ThreadPool::WorkerFunc(unsigned int id)
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(m_condMutex);
        m_cond.wait(lock);

        if (m_finish)
            return;

        lock.unlock();

        std::packaged_task<void()> task;

        while (popTask(task))
        {
            {
                std::lock_guard<std::mutex> lock(m_queueMutex);
                ++m_activeTasks;
            }

            task();

            {
                std::lock_guard<std::mutex> lock(m_queueMutex);
                --m_activeTasks;

                if (m_tasks.empty() && m_activeTasks == 0)
                {
                    m_idleCond.notify_all();
                }
            }
        }
    }
}

std::future<void> ThreadPool::submitTask(const std::function<void()>& function)
{
    std::packaged_task<void()> task(function);
    std::future<void> result = task.get_future();

    pushTask(std::move(task));

    m_cond.notify_one();

    return result;
}

void ThreadPool::pushTask(std::packaged_task<void()>&& task)
{
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_tasks.push(std::move(task));
}

bool ThreadPool::popTask(std::packaged_task<void()>& task)
{
    std::lock_guard<std::mutex> lock(m_queueMutex);

    if (m_tasks.empty())
    {
        return false;
    }

    task = std::move(m_tasks.front());
    m_tasks.pop();

    return true;
}

void ThreadPool::waitIdle()
{
    std::unique_lock<std::mutex> lock(m_queueMutex);
    m_idleCond.wait(lock, [this]() { return m_tasks.empty() && m_activeTasks == 0; });
}
