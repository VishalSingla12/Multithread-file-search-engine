#pragma once

#include <vector>
#include <thread>
#include <functional>
#include <atomic>
#include <cstddef>
#include "ThreadSafeQueue.h"

/**
 * A fixed-size thread pool that processes std::function<void()> tasks.
 *
 * Tasks are submitted via submit().  Workers block on the internal
 * ThreadSafeQueue until a task arrives or the pool is stopped.
 * Calling stop() (or the destructor) drains the queue and joins all threads.
 */
class ThreadPool {
public:
    using Task = std::function<void()>;

    /**
     * Construct a thread pool with the given number of worker threads.
     * @param threadCount  Number of workers; must be >= 1.
     */
    explicit ThreadPool(std::size_t threadCount);

    /**
     * Stop the pool (if still running) and join all worker threads.
     */
    ~ThreadPool();

    // Non-copyable, non-movable.
    ThreadPool(const ThreadPool&)            = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    /**
     * Submit a callable task to the pool.
     * @throws std::runtime_error if the pool has been stopped.
     */
    void submit(Task task);

    /**
     * Block until all submitted tasks have completed.
     * (Spin-wait with short yield – acceptable for benchmark use-cases.)
     */
    void waitForAll();

    /**
     * Signal workers to stop after draining remaining tasks, then join.
     * Safe to call multiple times.
     */
    void stop();

    /** @return Number of worker threads in this pool. */
    std::size_t threadCount() const { return workers_.size(); }

    /** @return Approximate number of tasks still pending. */
    std::size_t pendingTasks() const { return taskQueue_.size(); }

    /** @return Total number of tasks that have completed. */
    uint64_t completedTasks() const { return completedTasks_.load(); }

private:
    void workerLoop();

    ThreadSafeQueue<Task>    taskQueue_;
    std::vector<std::thread> workers_;
    std::atomic<bool>        stopped_{false};
    std::atomic<uint64_t>    submittedTasks_{0};
    std::atomic<uint64_t>    completedTasks_{0};
};
