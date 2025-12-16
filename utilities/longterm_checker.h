#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

namespace cpptools {
namespace utilities {

class longterm_checker {
   public:
    using clock_type = std::chrono::steady_clock;
    using duration = clock_type::duration;
    using time_point = clock_type::time_point;

    longterm_checker(duration interval, std::function<void()> task)
        : interval_(interval), task_(std::move(task)), stopped_(true) {}

    ~longterm_checker() {
        stop();
        if (worker_.joinable()) {
            worker_.join();
        }
    }

    void start() {
        if (worker_.joinable()) return;
        stopped_.store(false);

        next_deadline_.store(clock_type::now() + interval_);
        worker_ = std::thread(&longterm_checker::run, this);
    }

    void stop() {
        if (!stopped_.exchange(true)) {
            cv_.notify_all();
        }
    }

    void join() {
        if (worker_.joinable()) {
            worker_.join();
        }
    }

    void check() {
        next_deadline_.store(clock_type::now() + interval_, std::memory_order_relaxed);
        cv_.notify_all();  // stop current wait, let worker check next deadline
    }

   private:
    void run() {
        while (!stopped_.load()) {
            time_point deadline = next_deadline_.load(std::memory_order_relaxed);

            std::unique_lock<std::mutex> lock(mutex_);
            if (!cv_.wait_until(lock, deadline, [this] { return stopped_.load(); })) {
                if (clock_type::now() >= next_deadline_.load()) {
                    lock.unlock();

                    try {
                        task_();
                    } catch (...) {
                        // handle exception
                    }

                    next_deadline_.store(clock_type::now() + interval_);
                    continue;
                }

                // deadline postponed by check()
                continue;
            }

            // stopped
            return;
        }
    }

    const duration interval_;
    const std::function<void()> task_;
    std::thread worker_;

    std::mutex mutex_;
    std::condition_variable cv_;

    std::atomic<bool> stopped_;
    std::atomic<time_point> next_deadline_;
};

class recorder {
   public:
    using clock_type = std::chrono::steady_clock;
    using duration = clock_type::duration;
    using time_point = clock_type::time_point;

    recorder() : start_at_(clock_type::now()), last_at_(start_at_) {}

    ~recorder() = default;

    // Total elapsed time since start
    duration elapsed() const noexcept { return clock_type::now() - start_at_; }

    // Time elapsed since last call, and reset last_at_
    duration elapsed_since_last() noexcept {
        time_point now = clock_type::now();
        duration diff = now - last_at_;
        last_at_ = now;
        return diff;
    }

   private:
    const time_point start_at_;
    time_point last_at_;
};

};  // namespace utilities
}  // namespace cpptools
