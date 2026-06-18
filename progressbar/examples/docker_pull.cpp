#include <progressbar/progressbar.hpp>
#include <thread>
#include <vector>
#include <chrono>

int main() {
    pbar::MultiDisplay display({100, 30, '=', ' ', '>', true});
    display.start();

    auto layer1 = display.add_bar("layer1: Downloading", 1000);
    auto layer2 = display.add_bar("layer2: Downloading", 500);
    auto layer3 = display.add_bar("layer3: Waiting", 0);
    auto layer4 = display.add_bar("layer4: Waiting", 0);

    std::vector<std::thread> workers;

    workers.emplace_back([&layer1] {
        for (int i = 0; i <= 1000; ++i) {
            layer1->set_progress(i);
            layer1->set_status_text(
                std::to_string(i * 28 / 1000) + "MB/28MB");
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        layer1->set_label("layer1: Pull complete");
        layer1->set_status(pbar::BarStatus::Complete);
    });

    workers.emplace_back([&layer2] {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        for (int i = 0; i <= 500; ++i) {
            layer2->set_progress(i);
            layer2->set_status_text(
                std::to_string(i * 15 / 500) + "MB/15MB");
            std::this_thread::sleep_for(std::chrono::milliseconds(8));
        }
        layer2->set_label("layer2: Extracting");
        layer2->set_status(pbar::BarStatus::Complete);
    });

    workers.emplace_back([&layer3] {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        layer3->set_label("layer3: Downloading");
        layer3->set_total(200);
        for (int i = 0; i <= 200; ++i) {
            layer3->set_progress(i);
            layer3->set_status_text(
                std::to_string(i * 8 / 200) + "MB/8MB");
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
        }
        layer3->set_label("layer3: Pull complete");
        layer3->set_status(pbar::BarStatus::Complete);
    });

    workers.emplace_back([&layer4] {
        std::this_thread::sleep_for(std::chrono::seconds(4));
        layer4->set_label("layer4: Downloading");
        layer4->set_total(300);
        for (int i = 0; i <= 300; ++i) {
            layer4->set_progress(i);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        layer4->set_label("layer4: Pull complete");
        layer4->set_status(pbar::BarStatus::Complete);
    });

    for (auto& t : workers) t.join();
    display.stop();

    return 0;
}
