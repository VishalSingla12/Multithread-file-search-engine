#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <stdexcept>

/**
 * A thread-safe, generic queue that supports graceful shutdown.
 *
 * Producers call push(); consumers call wait_and_pop() (blocking)
 * or try_pop() (non-blocking).  When shutdown() is called all
 * blocked wait_and_pop() calls return std::nullopt so the threads
 * can exit cleanly.
 */
template <typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() = default;

    // Non-copyable, non-movable – queues are shared by reference.
    ThreadSafeQueue(const ThreadSafeQueue&)            = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    /**
     * Push a value onto the queue.
     * @throws std::runtime_error if the queue has been shut down.
     */
    void push(T value) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (shutdown_) {
                throw std::runtime_error("ThreadSafeQueue: push on shut-down queue");
            }
            queue_.push(std::move(value));
        }
        cv_.notify_one();
    }

    /**
     * Block until an item is available or the queue is shut down.
     * @return The next item, or std::nullopt if the queue was shut
     *         down and is empty.
     */
    std::optional<T> wait_and_pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !queue_.empty() || shutdown_; });

        if (queue_.empty()) {
            return std::nullopt;   // shut down + drained
        }
        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }

    /**
     * Non-blocking pop.
     * @return The next item or std::nullopt if the queue is empty.
     */
    std::optional<T> try_pop() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return std::nullopt;
        }
        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }

    /** @return true if the queue is currently empty. */
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    /** @return Number of items currently in the queue. */
    std::size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    /**
     * Signal all waiting consumers to unblock and return nullopt.
     * After calling shutdown(), push() will throw.
     */
    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            shutdown_ = true;
        }
        cv_.notify_all();
    }

    /** @return true if shutdown() has been called. */
    bool is_shutdown() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return shutdown_;
    }

private:
    mutable std::mutex      mutex_;
    std::condition_variable cv_;
    std::queue<T>           queue_;
    bool                    shutdown_{false};
};
