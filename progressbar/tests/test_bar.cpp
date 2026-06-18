#include <progressbar/progressbar.hpp>
#include <cassert>
#include <iostream>
#include <thread>
#include <vector>

void test_bar_basic() {
    pbar::Bar bar(0, "test", 100);
    auto snap = bar.snapshot();
    assert(snap.id == 0);
    assert(snap.label == "test");
    assert(snap.current == 0);
    assert(snap.total == 100);
    assert(snap.status == pbar::BarStatus::Waiting);

    bar.set_progress(50);
    snap = bar.snapshot();
    assert(snap.current == 50);
    assert(snap.status == pbar::BarStatus::InProgress);

    bar.tick(10);
    snap = bar.snapshot();
    assert(snap.current == 60);

    bar.set_status(pbar::BarStatus::Complete);
    snap = bar.snapshot();
    assert(snap.status == pbar::BarStatus::Complete);

    std::cout << "test_bar_basic PASSED\n";
}

void test_bar_thread_safety() {
    pbar::Bar bar(1, "threaded", 10000);
    std::vector<std::thread> threads;

    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&bar] {
            for (int i = 0; i < 2500; ++i) {
                bar.tick(1);
            }
        });
    }

    for (auto& th : threads) th.join();

    auto snap = bar.snapshot();
    assert(snap.current == 10000);

    std::cout << "test_bar_thread_safety PASSED\n";
}

void test_multi_display_add_remove() {
    pbar::MultiDisplay display;
    auto b1 = display.add_bar("one", 100);
    auto b2 = display.add_bar("two", 200);

    assert(b1->id() == 0);
    assert(b2->id() == 1);

    display.remove_bar(b1->id());
    b2->set_progress(50);
    auto snap = b2->snapshot();
    assert(snap.current == 50);

    std::cout << "test_multi_display_add_remove PASSED\n";
}

int main() {
    test_bar_basic();
    test_bar_thread_safety();
    test_multi_display_add_remove();

    std::cout << "\nAll tests passed.\n";
    return 0;
}
