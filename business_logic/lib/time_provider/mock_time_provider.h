#pragma once

#include "abstract_time_provider.h"


std::chrono::system_clock::time_point defaultMockTime();

class MockTimeProvider : public AbstractTimeProvider {
 public:
    explicit MockTimeProvider(std::chrono::system_clock::time_point initial = defaultMockTime());
};