#pragma once

#include <vector>
#include <string>

#include "message.h"
#include "user.h"


class AbstractChat {
 public:
    using ChatRoomId = boost::uuids::uuid;

    AbstractChat(ChatRoomId id, std::string  name);

    virtual ~AbstractChat() = default;

    [[nodiscard]] ChatRoomId getId() const;
    [[nodiscard]] const std::string& getName() const;
    [[nodiscard]] const std::vector<User::UserId>& getParticipants() const;
    [[nodiscard]] const std::vector<Message>& getMessages() const;

    void addMessage(const Message& message);

    bool editMessage(User::UserId user, Message::MessageId message_id,
        const std::string& new_text, std::chrono::system_clock::time_point now);

    virtual bool removeMessage(Message::MessageId message_id, User::UserId user_id,
        std::chrono::system_clock::time_point now) = 0;

    virtual bool addParticipant(User::UserId user_add, User::UserId user_get_added) = 0;
    virtual bool removeParticipant(User::UserId user_remove, User::UserId user_get_removed) = 0;
    virtual bool canDeleteChat(User::UserId user_id) = 0;

 protected:
    ChatRoomId                id_;
    std::string               name_;
    std::vector<User::UserId> participants_;
    std::vector<Message>      messages_;
};
