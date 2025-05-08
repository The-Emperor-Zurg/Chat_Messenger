#pragma once

#include "abstract_chat.h"


class PersonalChat : public AbstractChat {
 public:
    PersonalChat(ChatRoomId id, const std::string& name);

    bool addParticipant(User::UserId user_add, User::UserId user_get_added) override;

    bool removeMessage(Message::MessageId message_id, User::UserId user_id,
        std::chrono::system_clock::time_point now) override;

    bool removeParticipant(User::UserId user_remove, User::UserId user_get_removed) override;

    bool canDeleteChat(User::UserId user_id) override;
};
