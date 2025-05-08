#include <utility>

#include "chat_room/abstract_chat.h"


AbstractChat::AbstractChat(ChatRoomId id, std::string  name)
    : id_(id), name_(std::move(name))
{}

AbstractChat::ChatRoomId AbstractChat::getId() const {
    return id_;
}

const std::string& AbstractChat::getName() const {
    return name_;
}

const std::vector<User::UserId>& AbstractChat::getParticipants() const {
    return participants_;
}

const std::vector<Message>& AbstractChat::getMessages() const {
    return messages_;
}

void AbstractChat::addMessage(const Message& message) {
    messages_.emplace_back(message);
}

bool AbstractChat::editMessage(User::UserId user, Message::MessageId message_id,
    const std::string& new_text,
    std::chrono::system_clock::time_point now) {

    auto it = std::find_if(messages_.begin(), messages_.end(),
        [message_id](const Message& m) {return m.getId() == message_id;});
    if (it == messages_.end()) {
        return false;
    }

    if (it->getAuthorId() != user) {
        return false;
    }

    auto diff = now - it->getTimestamp();
    if (std::chrono::duration_cast<std::chrono::minutes>(diff).count() > 60) {
        return false;
    }

    it->changeText(new_text);
    return true;
}

