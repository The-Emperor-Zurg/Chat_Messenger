#pragma once

#include <chrono>

#include <boost/uuid/uuid.hpp>

#include "user.h"


class Message {
 public:
    using TimePoint = std::chrono::system_clock::time_point;
    using MessageId = boost::uuids::uuid;

    Message(MessageId id, User::UserId author_id, std::string text, TimePoint timestamp);

    [[nodiscard]] MessageId getId() const;
    [[nodiscard]] User::UserId getAuthorId() const;
    [[nodiscard]] const std::string& getText() const;
    [[nodiscard]] TimePoint getTimestamp() const;
    [[nodiscard]] bool isEdited() const;

    void changeText(const std::string& text);

 private:
    MessageId    id_;
    User::UserId authorId_;
    std::string  text_;
    TimePoint    timestamp_;
    bool         isEdited_;
};
