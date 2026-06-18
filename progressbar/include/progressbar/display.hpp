#pragma once

#include <atomic>

namespace pbar {

struct DisplayOptions {
    unsigned int refresh_rate_ms = 100;
    unsigned int bar_width = 40;
    char fill_char = '=';
    char empty_char = ' ';
    char lead_char = '>';
    bool use_color = true;
};

class Display {
public:
    virtual ~Display() = default;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual bool is_running() const = 0;
};

} // namespace pbar
