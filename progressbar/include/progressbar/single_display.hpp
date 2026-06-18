#pragma once

#include "display.hpp"
#include "bar.hpp"
#include "terminal.hpp"

#include <string>
#include <deque>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <chrono>
#include <atomic>

namespace pbar {

class SingleDisplay : public Display {
public:
    explicit SingleDisplay(DisplayOptions opts = {})
        : opts_(opts) {}

    ~SingleDisplay() override {
        if (running_.load(std::memory_order_relaxed))
            stop();
    }

    void set_progress(std::size_t current, std::size_t total) {
        std::lock_guard<std::mutex> lk(state_mu_);
        current_ = current;
        total_ = total;
    }

    void set_progress(std::size_t current) {
        std::lock_guard<std::mutex> lk(state_mu_);
        current_ = current;
    }

    void set_total(std::size_t total) {
        std::lock_guard<std::mutex> lk(state_mu_);
        total_ = total;
    }

    void log(const std::string& message) {
        std::lock_guard<std::mutex> lk(log_mu_);
        pending_logs_.push_back(message);
    }

    void set_label(const std::string& label) {
        std::lock_guard<std::mutex> lk(state_mu_);
        label_ = label;
    }

    void start() override {
        if (running_.load(std::memory_order_relaxed))
            return;
        running_.store(true, std::memory_order_relaxed);
        tty_ = term::is_tty();
        if (tty_)
            term::detail::install_cursor_guard();
        render_thread_ = std::thread(&SingleDisplay::render_loop, this);
    }

    void stop() override {
        if (!running_.load(std::memory_order_relaxed))
            return;
        running_.store(false, std::memory_order_relaxed);
        {
            std::lock_guard<std::mutex> lk(cv_mu_);
            cv_.notify_all();
        }
        if (render_thread_.joinable())
            render_thread_.join();

        render_frame();
        std::fputs("\n", stdout);
        if (tty_) {
            std::fputs(term::show_cursor().c_str(), stdout);
            std::fflush(stdout);
        }
    }

    bool is_running() const override {
        return running_.load(std::memory_order_relaxed);
    }

private:
    void render_loop() {
        using clock = std::chrono::steady_clock;
        auto interval = std::chrono::milliseconds(opts_.refresh_rate_ms);

        if (tty_)
            std::fputs(term::hide_cursor().c_str(), stdout);

        while (running_.load(std::memory_order_relaxed)) {
            auto frame_start = clock::now();
            render_frame();

            std::unique_lock<std::mutex> lk(cv_mu_);
            cv_.wait_until(lk, frame_start + interval, [this] {
                return !running_.load(std::memory_order_relaxed);
            });
        }
    }

    void render_frame() {
        if (!tty_) {
            render_plain();
            return;
        }

        std::string buf;

        {
            std::lock_guard<std::mutex> lk(log_mu_);
            if (!pending_logs_.empty()) {
                buf += term::carriage_return() + term::erase_line();
                for (auto& msg : pending_logs_) {
                    buf += msg + "\n";
                }
                pending_logs_.clear();
            }
        }

        std::size_t cur, tot;
        std::string label;
        {
            std::lock_guard<std::mutex> lk(state_mu_);
            cur = current_;
            tot = total_;
            label = label_;
        }

        buf += term::carriage_return() + term::erase_line();

        double pct = tot > 0 ? static_cast<double>(cur) / tot : 0.0;
        if (pct > 1.0) pct = 1.0;
        int filled = static_cast<int>(pct * opts_.bar_width);
        int pct_int = static_cast<int>(pct * 100);

        buf += label + ": [";
        for (unsigned int i = 0; i < opts_.bar_width; ++i) {
            if (static_cast<int>(i) < filled)
                buf += opts_.fill_char;
            else if (static_cast<int>(i) == filled && filled < static_cast<int>(opts_.bar_width))
                buf += opts_.lead_char;
            else
                buf += opts_.empty_char;
        }
        buf += "] " + std::to_string(pct_int) + "%";

        std::fwrite(buf.data(), 1, buf.size(), stdout);
        std::fflush(stdout);
    }

    void render_plain() {
        std::lock_guard<std::mutex> lk(log_mu_);
        for (auto& msg : pending_logs_) {
            std::fprintf(stdout, "%s\n", msg.c_str());
        }
        pending_logs_.clear();
    }

    DisplayOptions opts_;
    std::mutex state_mu_;
    std::size_t current_{0};
    std::size_t total_{100};
    std::string label_{"Progress"};

    std::mutex log_mu_;
    std::deque<std::string> pending_logs_;

    std::atomic<bool> running_{false};
    std::thread render_thread_;
    std::condition_variable cv_;
    std::mutex cv_mu_;
    bool tty_{true};
};

} // namespace pbar
