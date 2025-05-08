#include "chat_manager.h"

#include <mutex>
#include <shared_mutex>

#include "chat_room/group_close_chat.h"
#include "chat_room/group_open_chat.h"
#include "chat_room/personal_chat.h"


ChatManager::ChatManager(const AbstractTimeProvider& time_provider, UserManager& user_manager)
    : timeProvider_(time_provider),
      userManager_(user_manager)
{}

ChatManager::ChatRoomId ChatManager::generateChatRoomId() {
    return chatRoomGenerator_();
}

ChatManager::MessageId ChatManager::generateMessageId() {
    return messageGenerator_();
}

bool ChatManager::validateUserExists(UserId user) const {
    return userManager_.userExists(user);
}

ChatManager::Clock::time_point ChatManager::now() const {
    return timeProvider_.now();
}

ChatManager::ChatRoomId ChatManager::createPersonalChat(const std::string& name, UserId user1, UserId user2) {
    std::unique_lock lock(mutex_);

    if (!validateUserExists(user1) || !validateUserExists(user2)) {
        return {};
    }

    auto personal_chat_id = generateChatRoomId();
    auto personal_chat = std::make_unique<PersonalChat>(personal_chat_id, name);

    personal_chat->addParticipant(user1, user1);
    personal_chat->addParticipant(user2, user2);

    chatRooms_.emplace(personal_chat_id, std::move(personal_chat));
    return personal_chat_id;
}

ChatManager::ChatRoomId ChatManager::createOpenGroup(const std::string& name, UserId admin_id) {
    std::unique_lock lock(mutex_);

    if (!validateUserExists(admin_id)) {
        return {};
    }

    auto open_group_id = generateChatRoomId();
    auto open_chat = std::make_unique<OpenGroupChat>(open_group_id, name, admin_id);

    chatRooms_.emplace(open_group_id, std::move(open_chat));
    return open_group_id;
}

ChatManager::ChatRoomId ChatManager::createCloseGroup(const std::string& name, UserId admin_id) {
    std::unique_lock lock(mutex_);

    if (!validateUserExists(admin_id)) {
        return {};
    }

    auto close_group_id = generateChatRoomId();
    auto close_chat = std::make_unique<CloseGroupChat>(close_group_id, name, admin_id);

    chatRooms_.emplace(close_group_id, std::move(close_chat));
    return close_group_id;
}

bool ChatManager::deleteChat(ChatRoomId id, UserId user_id) {
    std::unique_lock lock(mutex_);

    if (!validateUserExists(user_id)) {
        return false;
    }

    auto it = chatRooms_.find(id);
    if (it == chatRooms_.end()) return false;

    if (!it->second->canDeleteChat(user_id)) return false;

    chatRooms_.erase(it);
    return true;
}


bool ChatManager::addParticipant(ChatRoomId room_id, UserId user_add, UserId user_get_add) {
    std::unique_lock lock(mutex_);

    if (!validateUserExists(user_add) || !validateUserExists(user_get_add)) {
        return false;
    }

    auto it = chatRooms_.find(room_id);
    if (it == chatRooms_.end()) return false;

    return it->second->addParticipant(user_add, user_get_add);
}

bool ChatManager::removeParticipant(ChatRoomId room_id, UserId user_remove, UserId user_get_remove) {
    std::unique_lock lock(mutex_);

    if (!validateUserExists(user_remove) || !validateUserExists(user_get_remove)) {
        return false;
    }

    auto it = chatRooms_.find(room_id);
    if (it == chatRooms_.end()) return false;

    bool result = it->second->removeParticipant(user_remove, user_get_remove);

    if (result && dynamic_cast<AbstractGroupChat*>(it->second.get())->getParticipants().empty()) {
        chatRooms_.erase(it);
    }

    return result;
}

bool ChatManager::sendMessage(ChatRoomId room_id, UserId sender_id, const std::string& message) {
    std::unique_lock lock(mutex_);

    if (!validateUserExists(sender_id)) {
        return false;
    }

    auto it = chatRooms_.find(room_id);
    if (it == chatRooms_.end()) return false;

    const auto& participants = it->second->getParticipants();
    if (std::find(participants.begin(), participants.end(), sender_id) == participants.end()) {
        return false;
    }

    Message msg(generateMessageId(), sender_id, message, now());
    it->second->addMessage(msg);
    return true;
}

bool ChatManager::editMessage(ChatRoomId room_id, UserId user_edit, MessageId id, const std::string& new_text) {
    std::unique_lock lock(mutex_);

    if (!validateUserExists(user_edit)) {
        return false;
    }

    auto it = chatRooms_.find(room_id);
    if (it == chatRooms_.end()) return false;

    return it->second->editMessage(user_edit, id, new_text, now());
}

bool ChatManager::removeMessage(ChatRoomId room_id, UserId user_remove, MessageId id) {
    std::unique_lock lock(mutex_);

    if (!validateUserExists(user_remove)) {
        return false;
    }

    auto it = chatRooms_.find(room_id);
    if (it == chatRooms_.end()) return false;

    return it->second->removeMessage(id, user_remove, now());
}


std::vector<Message> ChatManager::getHistory(ChatRoomId roomId) const {
    std::shared_lock lock(mutex_);

    auto it = chatRooms_.find(roomId);
    return it != chatRooms_.end() ? it->second->getMessages() : std::vector<Message>{};
}

User::UserId ChatManager::getChatAdmin(ChatRoomId room_id) const {
    std::shared_lock lock(mutex_);

    auto it = chatRooms_.find(room_id);
    if (it == chatRooms_.end()) return {};

    auto* group_chat = dynamic_cast<const AbstractGroupChat*>(it->second.get());
    return group_chat ? group_chat->getAdminId() : UserId{};
}


bool ChatManager::chatExists(ChatRoomId roomId) const {
    std::shared_lock lock(mutex_);
    return chatRooms_.contains(roomId);
}

std::vector<ChatManager::ChatRoomId> ChatManager::getUserChats(UserId user_id) const {
    std::shared_lock lock(mutex_);
    std::vector<ChatRoomId> out;
    for (auto const& [roomId, roomPtr] : chatRooms_) {
        auto participants = roomPtr->getParticipants();
        if (std::find(participants.begin(), participants.end(), user_id) != participants.end()) {
            out.push_back(roomId);
        }
    }
    return out;
}

std::vector<ChatManager::UserId> ChatManager::getChatParticipants(ChatRoomId room_id) const {
    std::shared_lock lock(mutex_);
    auto it = chatRooms_.find(room_id);
    if (it == chatRooms_.end()) return {};
    return it->second->getParticipants();
}
