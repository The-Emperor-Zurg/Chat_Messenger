#pragma once

#include <chrono>


class AbstractTimeProvider {
 public:
    explicit AbstractTimeProvider(std::chrono::system_clock::time_point initial);

    [[nodiscard]] std::chrono::system_clock::time_point now() const;

    void advanceTime(std::chrono::seconds delta);

    virtual ~AbstractTimeProvider() = default;

 protected:
    std::chrono::system_clock::time_point currentTime_;
};
