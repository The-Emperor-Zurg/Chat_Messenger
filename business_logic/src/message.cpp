#include "message.h"

#include <utility>


Message::Message(MessageId id, User::UserId author_id,std::string text,TimePoint timestamp)
    : id_(id),
      authorId_(author_id),
      text_(std::move(text)),
      timestamp_(timestamp),
      isEdited_(false)
{}

Message::MessageId Message::getId() const {
    return id_;
}

User::UserId Message::getAuthorId() const {
    return authorId_;
}

const std::string& Message::getText() const {
    return text_;
}

Message::TimePoint Message::getTimestamp() const {
    return timestamp_;
}

bool Message::isEdited() const {
    return isEdited_;
}

void Message::changeText(const std::string& text) {
    text_ = text;
    isEdited_ = true;
}