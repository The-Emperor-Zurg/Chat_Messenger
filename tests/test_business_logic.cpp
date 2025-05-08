#include "gtest/gtest.h"

#include "../business_logic/lib/chat_manager.h"
#include "../business_logic/lib/time_provider/mock_time_provider.h"

class ChatTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        userManager_ = std::make_unique<UserManager>();
        chatManager_ = std::make_unique<ChatManager>(mockTimeProvider_, *userManager_);
    }

    User::UserId registerUser(const std::string& name) {
        return userManager_->registerUser(name);
    }

    MockTimeProvider mockTimeProvider_;
    std::unique_ptr<UserManager> userManager_;
    std::unique_ptr<ChatManager> chatManager_;
};

TEST_F(ChatTestFixture, RegisterUser) {
    auto userId = registerUser("Maximus");
    auto userOpt = userManager_->getUser(userId);
    ASSERT_TRUE(userOpt.has_value());
    EXPECT_EQ(userOpt->get().getName(), "Maximus");
}

TEST_F(ChatTestFixture, CreatePersonalChatAndSendMessages) {
    auto user1 = registerUser("Bormoley");
    auto user2 = registerUser("Achilles");
    auto chatId = chatManager_->createPersonalChat("Chat", user1, user2);

    auto history = chatManager_->getHistory(chatId);
    EXPECT_TRUE(history.empty());

    bool sentByUser1 = chatManager_->sendMessage(chatId, user1, "Hello!");
    bool sentByUser2 = chatManager_->sendMessage(chatId, user2, "Hi!");
    EXPECT_TRUE(sentByUser1);
    EXPECT_TRUE(sentByUser2);

    history = chatManager_->getHistory(chatId);
    EXPECT_EQ(history.size(), 2);
}

TEST_F(ChatTestFixture, UsersCannotLeavePersonalChat) {
    auto user1 = registerUser("Bormoley");
    auto user2 = registerUser("Achilles");
    auto chatId = chatManager_->createPersonalChat("Chat", user1, user2);

    EXPECT_FALSE(chatManager_->removeParticipant(chatId, user1, user1));
}

TEST_F(ChatTestFixture, NotAuthorCannotDeleteMessageInPersonalChat) {
    auto user1 = registerUser("Bormoley");
    auto user2 = registerUser("Achilles");
    auto chatId = chatManager_->createPersonalChat("Chat", user1, user2);

    chatManager_->sendMessage(chatId, user1, "Hello!");
    auto history = chatManager_->getHistory(chatId);
    ASSERT_FALSE(history.empty());

    auto messageId = history.back().getId();
    EXPECT_FALSE(chatManager_->removeMessage(chatId, user2, messageId));
}

TEST_F(ChatTestFixture, CannotAddThirdUserToPersonalChat) {
    auto user1 = registerUser("Bormoley");
    auto user2 = registerUser("Achilles");
    auto user3 = registerUser("Patroclus");
    auto chatId = chatManager_->createPersonalChat("Chat", user1, user2);

    EXPECT_FALSE(chatManager_->addParticipant(chatId, user1, user3));
}

TEST_F(ChatTestFixture, EditMessageImmediatelySuccess) {
    auto user1 = registerUser("Bormoley");
    auto user2 = registerUser("Achilles");
    auto chatId = chatManager_->createPersonalChat("Chat", user1, user2);

    chatManager_->sendMessage(chatId, user1, "Hello!");
    auto history = chatManager_->getHistory(chatId);
    auto messageId = history.back().getId();

    EXPECT_TRUE(chatManager_->editMessage(chatId, user1, messageId, "Ave!"));
    history = chatManager_->getHistory(chatId);
    EXPECT_EQ(history.back().getText(), "Ave!");
}

TEST_F(ChatTestFixture, EditMessageAfter1HourFailure) {
    auto user1 = registerUser("Bormoley");
    auto user2 = registerUser("Achilles");
    auto chatId = chatManager_->createPersonalChat("Chat", user1, user2);

    chatManager_->sendMessage(chatId, user1, "Hello!");
    auto history = chatManager_->getHistory(chatId);
    auto messageId = history.back().getId();

    mockTimeProvider_.advanceTime(std::chrono::hours(1) + std::chrono::minutes(1));
    EXPECT_FALSE(chatManager_->editMessage(chatId, user1, messageId, "Ave!"));
}

TEST_F(ChatTestFixture, DeleteMessageImmediatelySuccess) {
    auto user1 = registerUser("Bormoley");
    auto user2 = registerUser("Achilles");
    auto chatId = chatManager_->createPersonalChat("Chat", user1, user2);

    chatManager_->sendMessage(chatId, user1, "Message to delete");
    auto history = chatManager_->getHistory(chatId);
    auto messageId = history.back().getId();

    EXPECT_TRUE(chatManager_->removeMessage(chatId, user1, messageId));
    EXPECT_TRUE(chatManager_->getHistory(chatId).empty());
}

TEST_F(ChatTestFixture, DeleteMessageAfter1HourFailure) {
    auto user1 = registerUser("Bormoley");
    auto user2 = registerUser("Achilles");
    auto chatId = chatManager_->createPersonalChat("Chat", user1, user2);

    chatManager_->sendMessage(chatId, user1, "Message to delete");
    auto history = chatManager_->getHistory(chatId);
    auto messageId = history.back().getId();

    mockTimeProvider_.advanceTime(std::chrono::hours(1) + std::chrono::minutes(1));
    EXPECT_FALSE(chatManager_->removeMessage(chatId, user1, messageId));
}

TEST_F(ChatTestFixture, DeletePersonalChatByParticipantAndOthers) {
    auto user1 = registerUser("Bormoley");
    auto user2 = registerUser("Achilles");
    auto user3 = registerUser("Patroclus");
    auto chatId = chatManager_->createPersonalChat("Chat", user1, user2);

    EXPECT_FALSE(chatManager_->deleteChat(chatId, user3));
    EXPECT_TRUE(chatManager_->deleteChat(chatId, user1));
    EXPECT_FALSE(chatManager_->sendMessage(chatId, user1, "Test"));
}

TEST_F(ChatTestFixture, CreateOpenGroupSuccess) {
    auto admin = registerUser("Bormoley");
    auto groupId = chatManager_->createOpenGroup("Open Group", admin);

    EXPECT_TRUE(chatManager_->chatExists(groupId));
    EXPECT_TRUE(chatManager_->getHistory(groupId).empty());
}

TEST_F(ChatTestFixture, CreateClosedGroupSuccess) {
    auto admin = registerUser("Bormoley");
    auto groupId = chatManager_->createCloseGroup("Closed Group", admin);

    EXPECT_TRUE(chatManager_->chatExists(groupId));
}

TEST_F(ChatTestFixture, AddMultipleParticipantsInOpenGroup) {
    auto admin = registerUser("Bormoley");
    auto user1 = registerUser("Achilles");
    auto user2 = registerUser("Patroclus");
    auto groupId = chatManager_->createOpenGroup("Open Group", admin);

    EXPECT_TRUE(chatManager_->addParticipant(groupId, admin, user1));
    EXPECT_TRUE(chatManager_->addParticipant(groupId, user1, user2));
}

TEST_F(ChatTestFixture, AdminAndNonAdminAddParticipantToClosedGroup) {
    auto admin = registerUser("Bormoley");
    auto user1 = registerUser("Achilles");
    auto user2 = registerUser("Patroclus");
    auto groupId = chatManager_->createCloseGroup("Closed Group", admin);

    EXPECT_TRUE(chatManager_->addParticipant(groupId, admin, user1));
    EXPECT_FALSE(chatManager_->addParticipant(groupId, user1, user2));
}

TEST_F(ChatTestFixture, AdminCanRemoveParticipantInOpenGroup) {
    auto admin = registerUser("Bormoley");
    auto user = registerUser("Achilles");
    auto groupId = chatManager_->createOpenGroup("Open Group", admin);

    chatManager_->addParticipant(groupId, admin, user);
    EXPECT_TRUE(chatManager_->removeParticipant(groupId, admin, user));
}

TEST_F(ChatTestFixture, AdminCannotEditAndDeleteMessages) {
    auto admin = registerUser("Bormoley");
    auto user = registerUser("Achilles");
    auto groupId = chatManager_->createOpenGroup("Open Group", admin);

    chatManager_->addParticipant(groupId, admin, user);
    chatManager_->sendMessage(groupId, user, "Ciao!");
    auto history = chatManager_->getHistory(groupId);
    auto messageId = history.back().getId();

    EXPECT_FALSE(chatManager_->editMessage(groupId, admin, messageId, "Edited"));
    EXPECT_TRUE(chatManager_->removeMessage(groupId, admin, messageId));
    history = chatManager_->getHistory(groupId);
    EXPECT_TRUE(history.empty());
}

TEST_F(ChatTestFixture, UserEditAndDeleteOwnMessages) {
    auto admin = registerUser("Bormoley");
    auto user1 = registerUser("Achilles");
    auto user2 = registerUser("Patroclus");
    auto groupId = chatManager_->createOpenGroup("Open Group", admin);

    chatManager_->addParticipant(groupId, admin, user1);
    chatManager_->addParticipant(groupId, admin, user2);
    chatManager_->sendMessage(groupId, user1, "Original");

    auto history = chatManager_->getHistory(groupId);
    auto messageId = history.back().getId();

    EXPECT_TRUE(chatManager_->editMessage(groupId, user1, messageId, "Edited"));
    EXPECT_FALSE(chatManager_->editMessage(groupId, user2, messageId, "Hacked"));

    EXPECT_TRUE(chatManager_->removeMessage(groupId, user1, messageId));
    EXPECT_TRUE(chatManager_->getHistory(groupId).empty());

    chatManager_->sendMessage(groupId, user1, "New Message");
    history = chatManager_->getHistory(groupId);
    messageId = history.back().getId();

    mockTimeProvider_.advanceTime(std::chrono::hours(2));

    EXPECT_FALSE(chatManager_->editMessage(groupId, user1, messageId, "Expired Edit"));
    EXPECT_FALSE(chatManager_->removeMessage(groupId, user1, messageId));
}

TEST_F(ChatTestFixture, DeleteOpenGroupByAdminAndOthers) {
    auto admin = registerUser("Bormoley");
    auto user = registerUser("Achilles");
    auto other_user = registerUser("Patroclus");
    auto groupId = chatManager_->createOpenGroup("Open Group", admin);

    chatManager_->addParticipant(groupId, admin, user);

    EXPECT_FALSE(chatManager_->deleteChat(groupId, user));
    EXPECT_FALSE(chatManager_->deleteChat(groupId, other_user));
    EXPECT_TRUE(chatManager_->deleteChat(groupId, admin));
    EXPECT_FALSE(chatManager_->chatExists(groupId));
}

TEST_F(ChatTestFixture, AdminLeavesNewAdminAssigned) {
    auto admin = registerUser("Bormoley");
    auto user = registerUser("Achilles");
    auto groupId = chatManager_->createOpenGroup("Group", admin);

    chatManager_->addParticipant(groupId, admin, user);
    chatManager_->removeParticipant(groupId, admin, admin);

    EXPECT_EQ(chatManager_->getChatAdmin(groupId), user);
}

TEST_F(ChatTestFixture, GroupDeletionWhenEmpty) {
    auto admin = registerUser("Bormoley");
    auto user = registerUser("Achilles");
    auto groupId = chatManager_->createOpenGroup("Group", admin);

    chatManager_->addParticipant(groupId, admin, user);
    chatManager_->removeParticipant(groupId, admin, admin);
    chatManager_->removeParticipant(groupId, user, user);

    EXPECT_FALSE(chatManager_->chatExists(groupId));
}
