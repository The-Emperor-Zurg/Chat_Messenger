#pragma once

#include "chat_manager.h"
#include "session.h"
#include "time_provider/mock_time_provider.h"

namespace ip = asio::ip;
namespace websocket = beast::websocket;


class Server {
public:
    Server(std::size_t threadCount,
           std::size_t port,
           std::shared_ptr<AbstractTimeProvider> timeProvider = std::make_shared<MockTimeProvider>());

    void start();

    void stop();

    std::shared_ptr<UserManager> getUserManager() const;

private:
    void onAcceptAsync();

    asio::io_context  ioc_;
    asio::thread_pool pool_;
    ip::tcp::acceptor acceptor_;

    std::size_t threadCount_;
    std::size_t port_;

    std::shared_ptr<AbstractTimeProvider> timeProvider_;
    std::shared_ptr<UserManager> userManager_;
    std::shared_ptr<ChatManager> chatManager_;
};
