#include <string>

#include <gtest/gtest.h>

#include "scope_guard.h"

TEST(ScopeGuardTest, RAII) {
    std::string str = "1";
    {
        ON_SCOPE_EXIT([&str]() noexcept { str += "2"; });
    }
    EXPECT_EQ(str, "12");
}

TEST(ScopeGuardTest, Dismiss) {
    std::string str = "1";
    {
        auto v = ON_SCOPE_EXIT_RAW([&str]() noexcept { str += "2"; });
        v.dismiss();
    }
    EXPECT_EQ(str, "1");
}