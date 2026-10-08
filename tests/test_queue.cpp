#include <gtest/gtest.h>
#include "ThreadSafeQueue.h"
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>

TEST(ThreadSafeQueueTest, PushAndPop) {
    ThreadSafeQueue<int> q;
    q.push(1);
    q.push(2);
    q.push(3);
    EXPECT_EQ(q.size(), 3u);
    auto v = q.try_pop();
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, 1);
}

TEST(ThreadSafeQueueTest, EmptyQueue) {
    ThreadSafeQueue<int> q;
    EXPECT_TRUE(q.empty());
    auto v = q.try_pop();
    EXPECT_FALSE(v.has_value());
}

TEST(ThreadSafeQueueTest, ShutdownUnblocksWaiters) {
    ThreadSafeQueue<int> q;
    std::atomic<bool> unblocked{false};
    std::thread t([&]() {
        auto v = q.wait_and_pop(); // blocks
        unblocked.store(!v.has_value()); // nullopt on shutdown
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    q.shutdown();
    t.join();
    EXPECT_TRUE(unblocked.load());
}

TEST(ThreadSafeQueueTest, PushAfterShutdownThrows) {
    ThreadSafeQueue<int> q;
    q.shutdown();
    EXPECT_THROW(q.push(1), std::runtime_error);
}

TEST(ThreadSafeQueueTest, ConcurrentProducersConsumers) {
    ThreadSafeQueue<int> q;
    constexpr int kItems = 1000;
    std::atomic<int> consumed{0};

    // 4 producers
    std::vector<std::thread> producers;
    for (int i = 0; i < 4; ++i) {
        producers.emplace_back([&, i]() {
            for (int j = 0; j < kItems / 4; ++j) {
                q.push(i * 1000 + j);
            }
        });
    }

    // 4 consumers
    std::vector<std::thread> consumers;
    for (int i = 0; i < 4; ++i) {
        consumers.emplace_back([&]() {
            while (true) {
                auto v = q.try_pop();
                if (v.has_value()) {
                    consumed.fetch_add(1);
                } else if (q.is_shutdown()) {
                    break;
                }
                // small yield to avoid busy spin in test
                std::this_thread::yield();
            }
        });
    }

    for (auto& t : producers) t.join();

    // Give consumers time to drain
    while (!q.empty()) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    q.shutdown();
    for (auto& t : consumers) t.join();

    EXPECT_EQ(consumed.load(), kItems);
}

TEST(ThreadSafeQueueTest, WaitAndPopReceivesItems) {
    ThreadSafeQueue<int> q;
    std::vector<int> received;
    std::mutex mu;

    std::thread consumer([&]() {
        while (true) {
            auto v = q.wait_and_pop();
            if (!v.has_value()) break;
            std::lock_guard<std::mutex> lk(mu);
            received.push_back(*v);
        }
    });

    for (int i = 0; i < 10; ++i) q.push(i);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    q.shutdown();
    consumer.join();

    EXPECT_EQ(received.size(), 10u);
}

TEST(ThreadSafeQueueTest, IsShutdownReturnsFalseInitially) {
    ThreadSafeQueue<int> q;
    EXPECT_FALSE(q.is_shutdown());
    q.shutdown();
    EXPECT_TRUE(q.is_shutdown());
}
