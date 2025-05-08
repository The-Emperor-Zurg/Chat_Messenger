#pragma once

#include <deque>
#include <mutex>
#include <memory>
#include <string>

#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>

#include "chat_manager.h"


namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace asio = boost::asio;
namespace ip = asio::ip;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(
        std::shared_ptr<websocket::stream<beast::tcp_stream>> ws,
        std::shared_ptr<ChatManager> chat_manager,
        std::shared_ptr<UserManager> user_manager,
        User::UserId user_id
    );

    void start();

private:

    void doRead();
    void doWrite();
    void writeAsync(const std::string& message);

    std::string dispatchCommand(const std::string& line);

    std::deque<std::string> messageQueue_;
    std::mutex messageQueueMutex_;

    std::shared_ptr<websocket::stream<beast::tcp_stream>> ws_;
    beast::flat_buffer buffer_;

    std::shared_ptr<ChatManager>  chatManager_;
    std::shared_ptr<UserManager>  userManager_;
    User::UserId                  userId_;
};
