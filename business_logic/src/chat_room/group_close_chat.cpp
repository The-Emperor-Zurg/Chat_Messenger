#include "chat_room/group_close_chat.h"


CloseGroupChat::CloseGroupChat(ChatRoomId id, const std::string& name, User::UserId admid_id)
    : AbstractGroupChat(id, name, admid_id)
{}

bool CloseGroupChat::addParticipant(User::UserId user_add, User::UserId user_get_add) {
    if (user_add != adminId_) {
        return false;
    }

    if (std::find(participants_.begin(), participants_.end(), user_get_add) != participants_.end()) {
        return false;
    }

    participants_.emplace_back(user_get_add);
    return true;
}