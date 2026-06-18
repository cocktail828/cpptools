#pragma once

#include "display.hpp"
#include "bar.hpp"
#include "terminal.hpp"

#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <algorithm>

namespace pbar {

class MultiDisplay : public Display {
public:
    explicit MultiDisplay(DisplayOptions opts = {})
        : opts_(opts) {}

    ~MultiDisplay() override {
        if (running_.load(std::memory_order_relaxed))
            stop();
    }

    std::shared_ptr<Bar> add_bar(const std::string& label, std::size_t total = 100) {
        std::lock_guard<std::mutex> lk(bars_mu_);
        auto bar = std::make_shared<Bar>(next_id_++, label, total);
        bars_.push_back(bar);
        return bar;
    }

    void remove_bar(Bar::Id id) {
        std::lock_guard<std::mutex> lk(bars_mu_);
        bars_.erase(
            std::remove_if(bars_.begin(), bars_.end(),
                [id](const std::shared_ptr<Bar>& b) { return b->id() == id; }),
            bars_.end());
    }

    void start() override {
        if (running_.load(std::memory_order_relaxed))
            return;
        running_.store(true, std::memory_order_relaxed);
        tty_ = term::is_tty();
        if (tty_)
            term::detail::install_cursor_guard();
        render_thread_ = std::thread(&MultiDisplay::render_loop, this);
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

        render_frame();

        if (tty_) {
            std::fputs(term::show_cursor().c_str(), stdout);
            std::fflush(stdout);
        }
    }

    void render_frame() {
        std::vector<Bar::Snapshot> snaps;
        {
            std::lock_guard<std::mutex> lk(bars_mu_);
            snaps.reserve(bars_.size());
            for (auto& b : bars_)
                snaps.push_back(b->snapshot());
        }

        if (snaps.empty())
            return;

        if (!tty_) {
            render_plain(snaps);
            return;
        }

        std::string buf;
        buf.reserve(snaps.size() * 128);

        if (lines_drawn_ > 0)
            buf += term::cursor_up(lines_drawn_);

        for (auto& snap : snaps) {
            buf += term::erase_line();
            buf += render_bar_line(snap);
            buf += "\n";
        }

        lines_drawn_ = static_cast<unsigned int>(snaps.size());

        std::fwrite(buf.data(), 1, buf.size(), stdout);
        std::fflush(stdout);
    }

    void render_plain(const std::vector<Bar::Snapshot>& snaps) {
        for (auto& snap : snaps) {
            if (snap.status == BarStatus::Complete) {
                std::fprintf(stdout, "%s [done]\n", snap.label.c_str());
            } else if (snap.total > 0) {
                int pct = static_cast<int>(100.0 * snap.current / snap.total);
                std::fprintf(stdout, "%s %d%%", snap.label.c_str(), pct);
                if (!snap.status_text.empty())
                    std::fprintf(stdout, " %s", snap.status_text.c_str());
                std::fprintf(stdout, "\n");
            }
        }
        std::fflush(stdout);
    }

    std::string render_bar_line(const Bar::Snapshot& snap) const {
        int term_width = term::get_width();
        std::string line;

        if (snap.status == BarStatus::Complete) {
            line = snap.label;
            if (opts_.use_color)
                line = term::green(line);
            return line;
        }

        if (snap.status == BarStatus::Error) {
            line = snap.label + " [error]";
            return line;
        }

        if (snap.status == BarStatus::Waiting) {
            line = snap.label;
            if (opts_.use_color)
                line = term::yellow(line);
            return line;
        }

        std::string label_part = snap.label + " ";
        std::string pct_str;
        int filled = 0;
        unsigned int bar_width = opts_.bar_width;

        if (snap.total > 0) {
            double ratio = static_cast<double>(snap.current) / snap.total;
            if (ratio > 1.0) ratio = 1.0;
            int pct = static_cast<int>(ratio * 100);
            pct_str = " " + std::to_string(pct) + "%";
            filled = static_cast<int>(ratio * bar_width);
        } else {
            static const char spin[] = "|/-\\";
            int idx = static_cast<int>(spin_counter_++ % 4);
            pct_str = std::string(" ") + spin[idx];
            filled = 0;
        }

        std::string status_part;
        if (!snap.status_text.empty())
            status_part = " " + snap.status_text;

        int overhead = static_cast<int>(label_part.size() + 2 + pct_str.size() + status_part.size());
        if (overhead + static_cast<int>(bar_width) > term_width && term_width > overhead + 10) {
            bar_width = term_width - overhead;
        }

        std::string bar = "[";
        for (unsigned int i = 0; i < bar_width; ++i) {
            if (static_cast<int>(i) < filled)
                bar += opts_.fill_char;
            else if (static_cast<int>(i) == filled && filled < static_cast<int>(bar_width))
                bar += opts_.lead_char;
            else
                bar += opts_.empty_char;
        }
        bar += "]";

        line = label_part + bar + pct_str + status_part;

        if (static_cast<int>(line.size()) > term_width)
            line.resize(term_width);

        return line;
    }

    DisplayOptions opts_;
    std::mutex bars_mu_;
    std::vector<std::shared_ptr<Bar>> bars_;
    std::atomic<bool> running_{false};
    std::thread render_thread_;
    std::condition_variable cv_;
    std::mutex cv_mu_;
    unsigned int lines_drawn_{0};
    Bar::Id next_id_{0};
    bool tty_{true};
    mutable std::size_t spin_counter_{0};
};

} // namespace pbar
