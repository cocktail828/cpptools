#pragma once

#include <cstdio>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>
#include <csignal>
#include <cstdlib>

namespace pbar {
namespace term {

inline int get_width() {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0)
        return w.ws_col;
    return 80;
}

inline bool is_tty() {
    return isatty(STDOUT_FILENO) != 0;
}

inline std::string cursor_up(int n)   { return "\033[" + std::to_string(n) + "A"; }
inline std::string cursor_down(int n) { return "\033[" + std::to_string(n) + "B"; }
inline std::string erase_line()       { return "\033[2K"; }
inline std::string carriage_return()  { return "\r"; }
inline std::string hide_cursor()      { return "\033[?25l"; }
inline std::string show_cursor()      { return "\033[?25h"; }

inline std::string green(const std::string& s)  { return "\033[32m" + s + "\033[0m"; }
inline std::string yellow(const std::string& s) { return "\033[33m" + s + "\033[0m"; }
inline std::string cyan(const std::string& s)   { return "\033[36m" + s + "\033[0m"; }
inline std::string bold(const std::string& s)   { return "\033[1m" + s + "\033[0m"; }

namespace detail {

inline void restore_cursor_handler(int) {
    std::fputs("\033[?25h", stdout);
    std::fflush(stdout);
    std::_Exit(1);
}

inline void install_cursor_guard() {
    static bool installed = false;
    if (!installed) {
        installed = true;
        std::atexit([] { std::fputs("\033[?25h", stdout); std::fflush(stdout); });
        std::signal(SIGINT, restore_cursor_handler);
        std::signal(SIGTERM, restore_cursor_handler);
    }
}

} // namespace detail
} // namespace term
} // namespace pbar
