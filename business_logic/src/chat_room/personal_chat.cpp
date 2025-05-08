#include "chat_room/personal_chat.h"


PersonalChat::PersonalChat(ChatRoomId id, const std::string& name)
    : AbstractChat(id, name)
{}

bool PersonalChat::addParticipant(User::UserId user_add, User::UserId user_get_added) {
    if (participants_.size() > 1) {
        return false;
    }

    if (std::find(participants_.begin(), participants_.end(), user_get_added) != participants_.end()) {
        return false;
    }

    participants_.emplace_back(user_get_added);

    return true;
}

bool PersonalChat::removeMessage(Message::MessageId message_id, User::UserId user_id,
        std::chrono::system_clock::time_point now) {

    auto it = std::find_if(messages_.begin(), messages_.end(),[message_id](const Message& m) {return m.getId() == message_id;});
    if (it == messages_.end()) {
        return false;
    }

    if (it->getAuthorId() != user_id) {
        return false;
    }

    auto diff = now - it->getTimestamp();
    if (std::chrono::duration_cast<std::chrono::minutes>(diff).count() > 60) {
        return false;
    }

    messages_.erase(it);
    return true;
}

bool PersonalChat::removeParticipant(User::UserId user_remove, User::UserId user_get_removed) {
    return false;
}

bool PersonalChat::canDeleteChat(User::UserId user_id) {
    return std::find(participants_.begin(), participants_.end(), user_id) != participants_.end();
}
