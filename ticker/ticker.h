#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

namespace timeutil {
class Ticker {
  public:
    using clock_type = std::chrono::steady_clock;
    using duration = clock_type::duration;
    using time_point = clock_type::time_point;

    explicit Ticker(duration interval, std::function<void()> task, bool lazy_start = true)
        : interval_(interval), task_(std::move(task)), stopped_(false) {
        if (!lazy_start)
            start();
    }

    ~Ticker() {
        stop();
        join();
    }

    void start() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (worker_.joinable())
            return; // already started
        stopped_ = false;
        worker_ = std::thread(&Ticker::run, this);
        worker_.detach();
    }

    void stop() noexcept {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stopped_)
                return;
            stopped_ = true;
        }
        cv_.notify_all();
    }

    void join() {
        if (worker_.joinable())
            worker_.join();
    }

    Ticker(const Ticker&) = delete;
    Ticker& operator=(const Ticker&) = delete;

  private:
    void run() {
        time_point tp = clock_type::now();
        while (true) {
            tp += interval_;
            std::unique_lock<std::mutex> lock(mutex_);
            if (cv_.wait_until(lock, tp, [this] { return stopped_; }))
                return;

            lock.unlock();
            task_();
        }
    }

    const duration              interval_;
    const std::function<void()> task_;
    std::thread                 worker_;
    mutable std::mutex          mutex_;
    std::condition_variable     cv_;
    bool                        stopped_;
};

} // namespace timeutil
