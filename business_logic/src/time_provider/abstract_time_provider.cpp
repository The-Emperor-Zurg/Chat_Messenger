#include "time_provider/abstract_time_provider.h"


AbstractTimeProvider::AbstractTimeProvider(std::chrono::system_clock::time_point initial)
    : currentTime_(initial)
{}

std::chrono::system_clock::time_point AbstractTimeProvider::now() const {
    return currentTime_;
}

void AbstractTimeProvider::advanceTime(std::chrono::seconds delta) {
    currentTime_ += delta;
}