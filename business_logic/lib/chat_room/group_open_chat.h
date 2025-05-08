#pragma once

#include "abstract_group_chat.h"


class OpenGroupChat : public AbstractGroupChat {
 public:
    OpenGroupChat(ChatRoomId id, const std::string& name, User::UserId admin_id);

    bool addParticipant(User::UserId user_add, User::UserId user_get_added) override;
};
