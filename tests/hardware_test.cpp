#include <string>

#include <gtest/gtest.h>

#include "clock.h"

TEST(CMOSTest, ReadClock) {
    auto rtc_t = hardware::get_cmos_clock();
    if (!rtc_t) {
        FAIL() << "读取 CMOS 时钟失败";
    }

    printf("CMOS 时间: %04d-%02d-%02d %02d:%02d:%02d\n", rtc_t->tm_year + 1900, rtc_t->tm_mon + 1, rtc_t->tm_mday,
           rtc_t->tm_hour, rtc_t->tm_min, rtc_t->tm_sec);

    auto timestamp = hardware::get_cmos_timestamp();
    if (!timestamp) {
        FAIL() << "读取 CMOS 时钟失败";
    }

    printf("CMOS 时间戳: %lu\n", timestamp.value());
}

#ifdef ENABLE_TSS2
TEST(TPMTest, ReadClock) {
    auto clock = hardware::get_tpm2_clock();
    if (!clock) {
        FAIL() << "读取 TPM 时钟失败";
    }

    printf("TPM 时钟 (ms): %lu\n", clock.value().clock);
    printf("Reset Count: %u\n", clock.value().resetCount);
    printf("Restart Count: %u\n", clock.value().restartCount);
    printf("Safe: %d\n", clock.value().safe);
}
#endif
