#include "time_provider/time_provider.h"


TimeProvider::TimeProvider()
    : AbstractTimeProvider(std::chrono::system_clock::now())
{}