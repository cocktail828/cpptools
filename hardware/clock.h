#pragma once

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/rtc.h>
#include <time.h>

#include <optional>
#include <iostream>
#include <cstdint>
#include <string>

#ifdef ENABLE_TSS2
#include <tss2/tss2_esys.h>
#endif

namespace hardware {

/**
 * @brief Read CMOS Clock
 *
 * @param dev CMOS Clock device path, default is "/dev/rtc0"
 * @return std::optional<rtc_time> If successful, returns the time structure; otherwise, returns empty
 */
std::optional<rtc_time> get_cmos_clock(const char* dev = "/dev/rtc0") {
    int fd = open(dev, O_RDWR);
    if (fd < 0) {
        return std::nullopt;
    }

    struct rtc_time rtc_t;
    if (ioctl(fd, RTC_RD_TIME, &rtc_t) < 0) {
        close(fd);
        return std::nullopt;
    }

    close(fd);
    return rtc_t;
}

std::optional<time_t> get_cmos_timestamp(const char* dev = "/dev/rtc0") {
    auto rtc_t = get_cmos_clock(dev);
    if (!rtc_t) {
        return std::nullopt;
    }

    struct tm tm_time {
        .tm_sec       = rtc_t->tm_sec,   // Seconds. [0-60] (1 leap second)
            .tm_min   = rtc_t->tm_min,   // Minutes. [0-59]
            .tm_hour  = rtc_t->tm_hour,  // Hours. [0-23]
            .tm_mday  = rtc_t->tm_mday,  // Day. [1-31]
            .tm_mon   = rtc_t->tm_mon,   // Month. [0-11]
            .tm_year  = rtc_t->tm_year,  // Year - 1900.
            .tm_isdst = -1,
    };

    time_t timestamp;
    timestamp = timegm(&tm_time);

    return timestamp;
}

#ifdef ENABLE_TSS2
struct tpm_clock {
    uint64_t clock; /* time in milliseconds during which the TPM has been powered. This structure element is used to
                       report on the TPMs Clock value. The value of Clock shall be recorded in nonvolatile memory no
                       less often than once per 69.9 minutes, 222 milliseconds of TPM operation. The reference for the
                       millisecond timer is the TPM oscillator. This value is reset to zero when the Storage Primary
                       Seed is changed TPM2_Clear. This value may be advanced by TPM2_AdvanceClock. */
    uint32_t resetCount;   /* number of occurrences of TPM Reset since the last TPM2_Clear */
    uint32_t restartCount; /* number of times that TPM2_Shutdown or _TPM_Hash_Start have occurred since the last TPM
                              Reset or TPM2_Clear. */
    uint8_t safe;
};

std::optional<tpm_clock> get_tpm2_clock() {
    ESYS_CONTEXT* ctx;
    TSS2_RC       rc;

    rc = Esys_Initialize(&ctx, nullptr, nullptr);
    if (rc != TSS2_RC_SUCCESS) {
        return std::nullopt;
    }

    TPMS_TIME_INFO* timeInfo = nullptr;
    rc                       = Esys_ReadClock(ctx, ESYS_TR_NONE, ESYS_TR_NONE, ESYS_TR_NONE, &timeInfo);

    if (rc != TSS2_RC_SUCCESS) {
        Esys_Finalize(&ctx);
        return std::nullopt;
    }

    auto c = tpm_clock{
        .clock        = timeInfo->clockInfo.clock,
        .resetCount   = timeInfo->clockInfo.resetCount,
        .restartCount = timeInfo->clockInfo.restartCount,
        .safe         = timeInfo->clockInfo.safe,
    };

    Esys_Free(timeInfo);
    Esys_Finalize(&ctx);
    return c;
}
#endif

};  // namespace hardware
