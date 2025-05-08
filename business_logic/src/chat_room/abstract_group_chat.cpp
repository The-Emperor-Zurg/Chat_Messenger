#include "chat_room/abstract_group_chat.h"


AbstractGroupChat::AbstractGroupChat(ChatRoomId id, const std::string& name, User::UserId admin_id)
    : AbstractChat(id, name), adminId_(admin_id) {
    participants_.emplace_back(admin_id);
}

User::UserId AbstractGroupChat::getAdminId() const {
    return adminId_;
}

bool AbstractGroupChat::removeParticipant(User::UserId user_remove, User::UserId user_get_removed) {
    auto it = std::find(participants_.begin(), participants_.end(), user_get_removed);
    if (it == participants_.end()) {
        return false;
    }

    if (user_remove == user_get_removed) {
        participants_.erase(it);

        if (user_remove == adminId_ && !participants_.empty()) {
            adminId_ = participants_.front();
        }

        return true;
    }

    if (user_remove != adminId_) {
        return false;
    }

    participants_.erase(it);

    return true;
}

bool AbstractGroupChat::removeMessage(Message::MessageId message_id, User::UserId user_id,
    std::chrono::system_clock::time_point now) {

    auto it = std::find_if(messages_.begin(), messages_.end(),[message_id](const Message& m) {return m.getId() == message_id;});
    if (it == messages_.end()) {
        return false;
    }

    if (user_id == adminId_) {
        messages_.erase(it);
        return true;
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

bool AbstractGroupChat::canDeleteChat(User::UserId user_id) {
    return user_id == adminId_;
}
