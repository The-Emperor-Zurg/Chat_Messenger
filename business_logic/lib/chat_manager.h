#pragma once

#include <shared_mutex>
#include <unordered_map>

#include "user.h"
#include "message.h"
#include "user_manager.h"
#include "chat_room/abstract_chat.h"
#include "time_provider/abstract_time_provider.h"


class ChatManager {
 public:
    using UserId = User::UserId;
    using ChatRoomId = boost::uuids::uuid;
    using MessageId = boost::uuids::uuid;
    using Clock = std::chrono::system_clock;

    explicit ChatManager(const AbstractTimeProvider& time_provider, UserManager& user_manager);

    ChatRoomId createPersonalChat(const std::string& name, UserId user1, UserId user2);
    ChatRoomId createOpenGroup(const std::string& name, UserId admin_id);
    ChatRoomId createCloseGroup(const std::string& name, UserId admin_id);
    bool deleteChat(ChatRoomId id, UserId user_id);

    bool addParticipant(ChatRoomId room_id, UserId user_add, UserId user_get_add);
    bool removeParticipant(ChatRoomId room_id, UserId user_remove, UserId user_get_remove);

    bool sendMessage(ChatRoomId room_id, UserId sender_id, const std::string& message);
    bool editMessage(ChatRoomId room_id, UserId user_edit, MessageId id, const std::string& new_text);
    bool removeMessage(ChatRoomId room_id, UserId user_remove, MessageId id);

    std::vector<Message> getHistory(ChatRoomId roomId) const;
    UserId getChatAdmin(ChatRoomId room_id) const;
    bool chatExists(ChatRoomId roomId) const;

    std::vector<ChatRoomId> getUserChats(UserId user_id) const;
    std::vector<UserId>     getChatParticipants(ChatRoomId room_id) const;

    Clock::time_point now() const;

 private:
    ChatRoomId generateChatRoomId();
    MessageId generateMessageId();
    bool validateUserExists(UserId user) const;

    std::unordered_map<ChatRoomId, std::unique_ptr<AbstractChat>> chatRooms_;
    UserManager& userManager_;

    const AbstractTimeProvider& timeProvider_;

    boost::uuids::random_generator chatRoomGenerator_;
    boost::uuids::random_generator messageGenerator_;

    mutable std::shared_mutex mutex_;
};
