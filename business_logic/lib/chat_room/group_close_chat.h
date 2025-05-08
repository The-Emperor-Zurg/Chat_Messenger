#pragma once

#include "abstract_group_chat.h"


class CloseGroupChat : public AbstractGroupChat {
 public:
    CloseGroupChat(ChatRoomId id, const std::string& name, User::UserId admid_id);

    bool addParticipant(User::UserId user_add, User::UserId user_get_add) override;
};
