#include "chat_room/group_open_chat.h"


OpenGroupChat::OpenGroupChat(ChatRoomId id, const std::string& name, User::UserId admin_id)
    : AbstractGroupChat(id, name, admin_id)
{}

bool OpenGroupChat::addParticipant(User::UserId user_add, User::UserId user_get_added) {
    if (std::find(participants_.begin(), participants_.end(), user_add) == participants_.end()) {
        return false;
    }

    if (std::find(participants_.begin(), participants_.end(), user_get_added) != participants_.end()) {
        return false;
    }

    participants_.emplace_back(user_get_added);

    return true;
}
