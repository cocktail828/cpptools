#pragma once

#include <string>
#include <cstddef>
#include <mutex>
#include <atomic>

namespace pbar {

enum class BarStatus {
    Waiting,
    InProgress,
    Complete,
    Error
};

class Bar {
public:
    using Id = std::size_t;

    struct Snapshot {
        Id id;
        std::string label;
        std::size_t current;
        std::size_t total;
        BarStatus status;
        std::string status_text;
    };

    Bar(Id id, std::string label, std::size_t total = 100)
        : id_(id), label_(std::move(label)), total_(total) {}

    void set_progress(std::size_t current) {
        std::lock_guard<std::mutex> lk(mu_);
        current_ = current;
        if (status_ == BarStatus::Waiting)
            status_ = BarStatus::InProgress;
    }

    void set_total(std::size_t total) {
        std::lock_guard<std::mutex> lk(mu_);
        total_ = total;
    }

    void set_status(BarStatus status) {
        std::lock_guard<std::mutex> lk(mu_);
        status_ = status;
    }

    void set_status_text(const std::string& text) {
        std::lock_guard<std::mutex> lk(mu_);
        status_text_ = text;
    }

    void set_label(const std::string& label) {
        std::lock_guard<std::mutex> lk(mu_);
        label_ = label;
    }

    void tick(std::size_t amount = 1) {
        std::lock_guard<std::mutex> lk(mu_);
        current_ += amount;
        if (status_ == BarStatus::Waiting)
            status_ = BarStatus::InProgress;
    }

    Snapshot snapshot() const {
        std::lock_guard<std::mutex> lk(mu_);
        return {id_, label_, current_, total_, status_, status_text_};
    }

    Id id() const { return id_; }

private:
    const Id id_;
    mutable std::mutex mu_;
    std::string label_;
    std::size_t current_{0};
    std::size_t total_;
    BarStatus status_{BarStatus::Waiting};
    std::string status_text_;
};

} // namespace pbar
