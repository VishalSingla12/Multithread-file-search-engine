#include "ThreadPool.h"
#include "Logger.h"
#include <stdexcept>
#include <thread>

ThreadPool::ThreadPool(std::size_t threadCount) {
    if (threadCount == 0) {
        throw std::invalid_argument("ThreadPool: threadCount must be >= 1");
    }
    workers_.reserve(threadCount);
    for (std::size_t i = 0; i < threadCount; ++i) {
        workers_.emplace_back(&ThreadPool::workerLoop, this);
    }
    LOG_INFO("ThreadPool started with " + std::to_string(threadCount) + " threads");
}

ThreadPool::~ThreadPool() {
    stop();
}

void ThreadPool::submit(Task task) {
    if (stopped_.load(std::memory_order_relaxed)) {
        throw std::runtime_error("ThreadPool: submit on stopped pool");
    }
    submittedTasks_.fetch_add(1, std::memory_order_relaxed);
    taskQueue_.push(std::move(task));
}

void ThreadPool::waitForAll() {
    while (completedTasks_.load(std::memory_order_relaxed) <
           submittedTasks_.load(std::memory_order_relaxed)) {
        std::this_thread::yield();
    }
}

void ThreadPool::stop() {
    bool expected = false;
    if (!stopped_.compare_exchange_strong(expected, true)) return;
    taskQueue_.shutdown();
    for (auto& t : workers_) {
        if (t.joinable()) t.join();
    }
    LOG_INFO("ThreadPool stopped");
}

void ThreadPool::workerLoop() {
    while (true) {
        auto task = taskQueue_.wait_and_pop();
        if (!task.has_value()) break;
        try {
            (*task)();
        } catch (const std::exception& ex) {
            LOG_ERROR(std::string("ThreadPool worker exception: ") + ex.what());
        } catch (...) {
            LOG_ERROR("ThreadPool worker: unknown exception");
        }
        completedTasks_.fetch_add(1, std::memory_order_relaxed);
    }
}
