#include <progressbar/progressbar.hpp>
#include <thread>
#include <vector>
#include <chrono>
#include <string>

int main() {
    pbar::SingleDisplay display({100, 50, '#', '.', '>', true});
    display.start();

    std::vector<std::string> steps = {
        "Reading package lists...",
        "Building dependency tree...",
        "Reading state information...",
        "The following packages will be upgraded:",
        "  libfoo libbar libbaz",
        "3 upgraded, 0 newly installed, 0 to remove.",
        "Unpacking libfoo (1.2.3-1) ...",
        "Setting up libfoo (1.2.3-1) ...",
        "Unpacking libbar (2.0.1-4) ...",
        "Setting up libbar (2.0.1-4) ...",
        "Unpacking libbaz (0.9.8-2) ...",
        "Setting up libbaz (0.9.8-2) ...",
        "Processing triggers for man-db ...",
    };

    std::size_t total = steps.size();
    display.set_total(total);

    for (std::size_t i = 0; i < total; ++i) {
        display.log(steps[i]);
        display.set_progress(i + 1, total);
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
    }

    display.stop();
    return 0;
}
