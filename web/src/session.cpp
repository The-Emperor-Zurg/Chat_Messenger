#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/beast/core.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "lib/session.h"

#include "lib/commands.h"


Session::Session(
    std::shared_ptr<websocket::stream<beast::tcp_stream>> websocket,
    std::shared_ptr<ChatManager> chat_manager,
    std::shared_ptr<UserManager> user_manager,
    User::UserId user_id)
    : ws_(std::move(websocket))
    , chatManager_(std::move(chat_manager))
    , userManager_(std::move(user_manager))
    , userId_(user_id)
{}

void Session::start() {
    ws_->async_accept([
        self = shared_from_this()
        ](boost::system::error_code ec) {
            if (ec) {
                std::cerr << "WebSocket handshake error: " << ec.message() << "\n";
                return;
            }
            std::string payload = to_string(self->userId_);
            self->writeAsync(std::to_string(int(OutCommand::USER_CREATED)) + " " + payload);

            self->doRead();
        });
}

void Session::doRead() {
    ws_->async_read(buffer_, [
        self = shared_from_this()
        ](boost::system::error_code ec, std::size_t) {
            if (ec) {
                if (ec == websocket::error::closed) {
                    std::cout << "Connection closed\n";
                } else {
                    std::cerr << "Read error: " << ec.message() << "\n";
                }
                self->userManager_->setLoggedIn(self->userId_, false);
                return;
            }

            const std::string line = beast::buffers_to_string(self->buffer_.cdata());
            self->buffer_.consume(self->buffer_.size());

            std::string reply = self->dispatchCommand(line);
            if (!reply.empty()) {
                self->writeAsync(reply);
            }

            self->doRead();
        });
}

void Session::writeAsync(const std::string& message) {
    std::lock_guard lock(messageQueueMutex_);
    messageQueue_.emplace_back(message);

    if (messageQueue_.size() == 1) {
        doWrite();
    }
}

void Session::doWrite() {
    if (messageQueue_.empty()) {
        return;
    }

    ws_->text(ws_->got_text());
    ws_->async_write(asio::buffer(messageQueue_.front()),
        [self = shared_from_this()](boost::system::error_code ec, std::size_t ...) {
        if (ec) {
            std::cerr << "Write error: " << ec.message() << "\n";
            return;
        }

        {
            std::lock_guard lock(self->messageQueueMutex_);
            self->messageQueue_.pop_front();
        }

        if (!self->messageQueue_.empty()) {
            self->doWrite();
        }
    });
}

std::string Session::dispatchCommand(const std::string& line) {
    std::stringstream ss;
    std::vector<std::string> tokens;
    split(tokens, line, boost::is_any_of(" "));

    if (tokens.empty()) {
        return std::to_string(int(OutCommand::ERRORR)) + ' ' +
            std::to_string(int(ErrorCode::INCORRECT_FORMAT));
    }

    int cmdInt = 0;
    try { cmdInt = std::stoi(tokens[0]); }
    catch (...) { return std::to_string(int(OutCommand::ERRORR)) + ' ' +
            std::to_string(int(ErrorCode::INCORRECT_FORMAT)); }


    const auto cmd = static_cast<InCommand>(cmdInt);
    try {
        switch (cmd) {
            case InCommand::RENAME_USER: {
                if (tokens.size() < 2) {
                    ss << static_cast<int>(OutCommand::ERRORR) << ' ' << static_cast<int>(ErrorCode::INCORRECT_FORMAT);
                    break;
                }

                const std::string& newName = tokens[1];
                bool ok = userManager_->renameUser(userId_, newName);
                if (ok) {
                    ss << static_cast<int>(OutCommand::USER_RENAMED);
                } else {
                    ss << static_cast<int>(OutCommand::ERRORR) << ' ' << static_cast<int>(ErrorCode::ERROR_USER_RENAME);
                }

                break;
            }

            case InCommand::CREATE_PERSONAL_CHAT: {
                if (tokens.size() < 3) {
                    ss << int(OutCommand::ERRORR) << ' ' << int(ErrorCode::INCORRECT_FORMAT);
                    break;
                }

                boost::uuids::uuid other = boost::uuids::string_generator()(tokens[1]);
                const std::string& name = tokens[2];
                auto chatId = chatManager_->createPersonalChat(name, userId_, other);
                if (chatId.is_nil()) {
                    ss << int(OutCommand::ERRORR) << ' ' << int(ErrorCode::ERROR_CHAT_CREATE);
                } else {
                    ss << int(OutCommand::CHAT_CREATED) << ' ' << to_string(chatId);
                }

                break;
            }

            case InCommand::CREATE_OPEN_GROUP: {
                if (tokens.size() < 2) {
                    ss << int(OutCommand::ERRORR) << ' ' << int(ErrorCode::INCORRECT_FORMAT);
                    break;
                }

                const std::string& name = tokens[1];
                auto chatId = chatManager_->createOpenGroup(name, userId_);

                if (chatId.is_nil()) {
                    ss << int(OutCommand::ERRORR) << ' ' << int(ErrorCode::ERROR_CHAT_CREATE);
                } else {
                    ss << int(OutCommand::CHAT_CREATED) << ' ' << to_string(chatId);
                }

                break;
            }

            case InCommand::CREATE_CLOSE_GROUP: {
                if (tokens.size() < 2) {
                    ss << int(OutCommand::ERRORR) << ' ' << int(ErrorCode::INCORRECT_FORMAT);
                    break;
                }

                const std::string& name = tokens[1];
                auto chatId = chatManager_->createCloseGroup(name, userId_);

                if (chatId.is_nil()) {
                    ss << int(OutCommand::ERRORR) << ' ' << int(ErrorCode::ERROR_CHAT_CREATE);
                } else {
                    ss << int(OutCommand::CHAT_CREATED) << ' ' << to_string(chatId);
                }

                break;
            }

            case InCommand::DELETE_CHAT: {
                if (tokens.size() < 2) {
                    ss << int(OutCommand::ERRORR) << ' ' << int(ErrorCode::INCORRECT_FORMAT);
                    break;
                }

                boost::uuids::uuid chatId = boost::uuids::string_generator()(tokens[1]);
                bool ok = chatManager_->deleteChat(chatId, userId_);

                if (ok) {
                    ss << int(OutCommand::CHAT_DELETED);
                } else {
                    ss << int(OutCommand::ERRORR) << ' ' << int(ErrorCode::ERROR_CHAT_DELETE);
                }

                break;
            }

            case InCommand::ADD_PARTICIPANT: {
                if (tokens.size() < 3) {
                    ss << int(OutCommand::ERRORR) << ' ' << int(ErrorCode::INCORRECT_FORMAT);
                    break;
                }

                boost::uuids::uuid chatId = boost::uuids::string_generator()(tokens[1]);
                boost::uuids::uuid toAdd  = boost::uuids::string_generator()(tokens[2]);
                bool ok = chatManager_->addParticipant(chatId, userId_, toAdd);

                if (ok) {
                    ss << int(OutCommand::PARTICIPANT_ADDED);
                } else {
                    ss << int(OutCommand::ERRORR) << ' ' << int(ErrorCode::ERROR_PARTICIPANT_ADD);
                }

                break;
            }

            case InCommand::REMOVE_PARTICIPANT: {
                if (tokens.size() < 3) {
                    ss << int(OutCommand::ERRORR) << ' ' << int(ErrorCode::INCORRECT_FORMAT);
                    break;
                }

                boost::uuids::uuid chatId    = boost::uuids::string_generator()(tokens[1]);
                boost::uuids::uuid toRemove  = boost::uuids::string_generator()(tokens[2]);
                bool ok = chatManager_->removeParticipant(chatId, userId_, toRemove);

                if (ok) {
                    ss << int(OutCommand::PARTICIPANT_REMOVED);
                } else {
                    ss << int(OutCommand::ERRORR) << ' ' << int(ErrorCode::ERROR_PARTICIPANT_REMOVE);
                }

                break;
            }

            case InCommand::SEND_MESSAGE: {
                if (tokens.size() < 3) {
                    ss << int(OutCommand::ERRORR) << ' ' << int(ErrorCode::INCORRECT_FORMAT);
                    break;
                }

                boost::uuids::uuid chatId = boost::uuids::string_generator()(tokens[1]);
                std::string text = line.substr(line.find(tokens[2]));
                bool ok = chatManager_->sendMessage(chatId, userId_, text);

                if (ok) {
                    ss << int(OutCommand::MESSAGE_SENT);
                } else {
                    ss << int(OutCommand::ERRORR) << ' ' << int(ErrorCode::ERROR_SEND_MESSAGE);
                }

                break;
            }

            case InCommand::EDIT_MESSAGE: {
                if (tokens.size() < 4) {
                    ss << int(OutCommand::ERRORR) << ' ' << int(ErrorCode::INCORRECT_FORMAT);
                    break;
                }

                boost::uuids::uuid chatId = boost::uuids::string_generator()(tokens[1]);
                boost::uuids::uuid msgId  = boost::uuids::string_generator()(tokens[2]);
                std::string newText = line.substr(
                    line.find(tokens[2]) + tokens[2].length() + 1
                );
                bool ok = chatManager_->editMessage(chatId, userId_, msgId, newText);

                if (ok) {
                    ss << int(OutCommand::MESSAGE_EDITED);
                } else {
                    ss << int(OutCommand::ERRORR) << ' ' << int(ErrorCode::ERROR_EDIT_MESSAGE);
                }

                break;
            }

            case InCommand::REMOVE_MESSAGE: {
                if (tokens.size() < 3) {
                    ss << int(OutCommand::ERRORR) << ' ' << int(ErrorCode::INCORRECT_FORMAT);
                    break;
                }

                boost::uuids::uuid chatId = boost::uuids::string_generator()(tokens[1]);
                boost::uuids::uuid msgId  = boost::uuids::string_generator()(tokens[2]);
                bool ok = chatManager_->removeMessage(chatId, userId_, msgId);

                if (ok) {
                    ss << int(OutCommand::MESSAGE_REMOVED);
                } else {
                    ss << int(OutCommand::ERRORR) << ' ' << int(ErrorCode::ERROR_REMOVE_MESSAGE);
                }

                break;
            }

            case InCommand::GET_HISTORY: {
                if (tokens.size() < 2) {
                    ss << int(OutCommand::ERRORR) << ' ' << int(ErrorCode::INCORRECT_FORMAT);
                    break;
                }

                boost::uuids::uuid chatId = boost::uuids::string_generator()(tokens[1]);
                auto history = chatManager_->getHistory(chatId);
                ss << int(OutCommand::HISTORY);

                if (!history.empty()) {
                    ss << ' ';
                    bool first = true;
                    for (const auto& m : history) {
                        if (!first) ss << '|';
                        ss << to_string(m.getId()) << ';'
                           << m.getAuthorId() << ';'
                           << m.getText();
                        first = false;
                    }
                }

                break;
            }

            case InCommand::LIST_USERS: {
                auto users = userManager_->getAllUsers();
                ss << static_cast<int>(OutCommand::USERS_LIST);
                if (!users.empty()) {
                    ss << ' ';
                    bool first = true;
                    for (auto& [id, name] : users) {
                        if (!first) ss << '|';
                        first = false;
                        ss << to_string(id) << ';' << name;
                    }
                }
                break;
            }

            case InCommand::SIGN_UP: {
                if (tokens.size() < 2) {
                    ss << int(OutCommand::SIGN_UP_FAIL);
                    break;
                }
                const std::string name = tokens[1];
                if (userManager_->nameExists(name)) {
                    ss << int(OutCommand::SIGN_UP_FAIL);
                } else {
                    auto newId = userManager_->registerUser();
                    userManager_->renameUser(newId, name);
                    userManager_->setLoggedIn(newId, true);
                    userId_ = newId;
                    ss << int(OutCommand::SIGN_UP_SUCCESS) << ' ' << to_string(newId);
                }
                break;
            }

            case InCommand::SIGN_IN: {
                if (tokens.size() < 2) {
                    ss << int(OutCommand::SIGN_IN_FAIL);
                    break;
                }
                const std::string name = tokens[1];
                auto optId = userManager_->findByName(name);
                if (!optId || userManager_->isLoggedIn(*optId)) {
                    ss << int(OutCommand::SIGN_IN_FAIL);
                } else {
                    userManager_->setLoggedIn(*optId, true);
                    userId_ = *optId;
                    ss << int(OutCommand::SIGN_IN_SUCCESS) << ' ' << to_string(*optId);
                }
                break;
            }

            case InCommand::SIGN_OUT: {
                userManager_->setLoggedIn(userId_, false);
                ss << int(OutCommand::SIGN_OUT_SUCCESS);
                break;
            }

            case InCommand::LIST_CHATS: {
                auto chats = chatManager_->getUserChats(userId_);
                ss << int(OutCommand::CHATS_LIST);
                if (!chats.empty()) {
                    ss << ' ';
                    bool first = true;
                    for (auto& cid : chats) {
                        if (!first) ss << '|';
                        first = false;
                        ss << to_string(cid);
                    }
                }
                break;
            }

            case InCommand::LIST_PARTICIPANTS: {
                if (tokens.size() < 2) {
                    ss << int(OutCommand::ERRORR) << ' ' << int(ErrorCode::INCORRECT_FORMAT);
                    break;
                }
                boost::uuids::uuid cid = boost::uuids::string_generator()(tokens[1]);
                auto parts = chatManager_->getChatParticipants(cid);
                ss << int(OutCommand::PARTICIPANTS_LIST);
                if (!parts.empty()) {
                    ss << ' ';
                    bool first = true;
                    for (auto& uid : parts) {
                        if (!first) ss << '|';
                        first = false;
                        ss << to_string(uid);
                    }
                }
                break;
            }

            default:
                ss << int(OutCommand::ERRORR) << ' ' << int(ErrorCode::UNKNOWN_COMMAND);

                break;
        }
    }
    catch (...) {
        ss << int(OutCommand::ERRORR) << ' ' << int(ErrorCode::INCORRECT_FORMAT);
    }

    return ss.str();
}
