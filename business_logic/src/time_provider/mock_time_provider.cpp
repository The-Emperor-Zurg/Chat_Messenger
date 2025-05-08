#include "time_provider/mock_time_provider.h"


std::chrono::system_clock::time_point defaultMockTime() {
    std::tm tm = {};
    tm.tm_year = 2023 - 1900;
    tm.tm_mon = 0;
    tm.tm_mday = 1;
    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    std::time_t tt = std::mktime(&tm);
    return std::chrono::system_clock::from_time_t(tt);
}

MockTimeProvider::MockTimeProvider(std::chrono::system_clock::time_point initial)
    : AbstractTimeProvider(initial)
{}