#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

namespace cpptools {
namespace slog {

enum class LogSeverity {
    DEBUG = spdlog::level::debug,
    INFO = spdlog::level::info,
    WARNING = spdlog::level::warn,
    ERROR = spdlog::level::err,
    FATAL = spdlog::level::critical
};

struct LogConfig {
    std::string progname = "app";
    LogSeverity log_level = LogSeverity::INFO;  // --loglevel
    bool log_to_stderr = true;                  // --logtostderr
    std::string log_file = "./logs/app.log";    // --logfile
    size_t max_file_size = 50 * 1024 * 1024;    // rolling size
    size_t max_files = 10;                      // rolling count
};

struct LogWrapper {
    LogConfig cfg_;
    std::shared_ptr<spdlog::logger> logger_;

    using iterator = std::vector<spdlog::sink_ptr>::iterator;
    LogWrapper(const LogConfig& cfg, spdlog::sink_ptr sink)
        : cfg_(cfg), logger_(std::make_shared<spdlog::logger>(cfg.progname, sink)) {}
    LogWrapper(const LogConfig& cfg, iterator begin, iterator end)
        : cfg_(cfg), logger_(std::make_shared<spdlog::logger>(cfg.progname, begin, end)) {}
};

inline std::shared_ptr<LogWrapper>& GetLogWrapper() {
    static std::shared_ptr<LogWrapper> wrapper = [] {
        LogConfig cfg;
        cfg.progname = "default";
        cfg.log_to_stderr = true;
        cfg.log_file.clear();

        auto sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
        sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

        auto w = std::make_shared<LogWrapper>(cfg, sink);
        w->logger_->set_level(spdlog::level::info);
        w->logger_->flush_on(spdlog::level::err);
        return w;
    }();

    return wrapper;
}

// -lspdlog -lfmt
inline void InitLoggingCompat(const LogConfig& cfg) {
    static std::once_flag once;
    std::call_once(once, [&] {
        std::vector<spdlog::sink_ptr> sinks;
        if (cfg.log_to_stderr) {
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
            sinks.push_back(console_sink);
        }

        if (!cfg.log_file.empty()) {
            auto file_sink =
                std::make_shared<spdlog::sinks::rotating_file_sink_mt>(cfg.log_file, cfg.max_file_size, cfg.max_files);
            file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
            sinks.push_back(file_sink);
        }

        auto lw = std::make_shared<LogWrapper>(cfg, sinks.begin(), sinks.end());
        lw->logger_->set_level(static_cast<spdlog::level::level_enum>(cfg.log_level));
        lw->logger_->flush_on(spdlog::level::err);

        spdlog::register_logger(lw->logger_);
        GetLogWrapper().swap(lw);
    });
}

inline void ShutdownLoggingCompat() {
    static std::once_flag once;
    auto& lw = GetLogWrapper();
    if (lw) {
        std::call_once(once, [&] {
            lw->logger_->flush();
            spdlog::drop(lw->logger_->name());
            lw.reset();
        });
    }
}

class LogStream {
   public:
    LogStream(LogSeverity severity, const char* file, int line) : loglevel_(severity), file_(file), line_(line) {
        stream_ << "[" << file_ << ":" << line_ << "] ";
    }

    ~LogStream() {
        auto& lw = GetLogWrapper();
        if (!lw) return;

        // fast return
        if (loglevel_ < lw->cfg_.log_level) return;

        lw->logger_->log(static_cast<spdlog::level::level_enum>(loglevel_), stream_.str());
        if (loglevel_ == LogSeverity::FATAL) {
            lw->logger_->flush();
            std::abort();
        }
    }

    template <typename T>
    LogStream& operator<<(const T& value) {
        stream_ << value;
        return *this;
    }

   private:
    int line_;
    const char* file_;
    std::ostringstream stream_;
    LogSeverity loglevel_;
};

};  // namespace slog
};  // namespace cpptools

#define LOG(sev) ::cpptools::slog::LogStream(::cpptools::slog::LogSeverity::sev, __FILE__, __LINE__)
#define CHECK(cond)                                                                               \
    do {                                                                                          \
        if (!(cond))                                                                              \
            ::cpptools::slog::LogStream(::cpptools::slog::LogSeverity::FATAL, __FILE__, __LINE__) \
                << "Check failed: " #cond " ";                                                    \
    } while (0)
