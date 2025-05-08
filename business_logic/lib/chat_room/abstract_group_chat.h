#pragma once

#include "abstract_chat.h"


class AbstractGroupChat : public AbstractChat {
 public:
    AbstractGroupChat(ChatRoomId id, const std::string& name, User::UserId admin_id);

    [[nodiscard]] User::UserId getAdminId() const;
    bool removeParticipant(User::UserId user_remove, User::UserId user_get_removed) override;

    bool removeMessage(Message::MessageId message_id, User::UserId user_id,
        std::chrono::system_clock::time_point now) override;

    bool canDeleteChat(User::UserId user_id) override;

    bool addParticipant(User::UserId user_add, User::UserId user_get_added) override = 0;

 protected:
    User::UserId adminId_;
};
