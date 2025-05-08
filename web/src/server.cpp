#include <iostream>

#include "lib/server.h"


Server::Server(std::size_t threadCount,
               std::size_t port,
               std::shared_ptr<AbstractTimeProvider> timeProvider)
    : pool_(threadCount),
      acceptor_(ioc_),
      threadCount_(threadCount),
      port_(port),
      timeProvider_(std::move(timeProvider)),
      userManager_(std::make_shared<UserManager>()),
      chatManager_(std::make_shared<ChatManager>(*timeProvider_, *userManager_))
{}


void Server::start() {
    for (std::size_t i = 0; i < threadCount_; ++i) {
        post(pool_, [this] {
            auto workGuard = make_work_guard(ioc_);
            ioc_.run();
        });
    }

    ip::tcp::endpoint endpoint(ip::tcp::v4(), port_);
    boost::system::error_code ec;

    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
        std::cerr << "Error opening acceptor: " << ec.message() << "\n";
        return;
    }

    acceptor_.set_option(ip::tcp::acceptor::reuse_address(true), ec);
    if (ec) {
        std::cerr << "Error setting reuse address: " << ec.message() << "\n";
        return;
    }

    acceptor_.bind(endpoint, ec);
    if (ec) {
        std::cerr << "Error binding: " << ec.message() << "\n";
        return;
    }

    acceptor_.listen(ip::tcp::acceptor::max_listen_connections, ec);
    if (ec) {
        std::cerr << "Error listening: " << ec.message() << "\n";
        return;
    }

    onAcceptAsync();
    pool_.join();
}

void Server::stop() {
    boost::system::error_code ec;
    acceptor_.close(ec);
    if (ec) {
        std::cerr << "Error closing acceptor: " << ec.message() << "\n";
    }

    ioc_.stop();
}

std::shared_ptr<UserManager> Server::getUserManager() const {
    return userManager_;
}

void Server::onAcceptAsync() {
    auto ws = std::make_shared<websocket::stream<beast::tcp_stream>>(ioc_);
    ws->set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));

    acceptor_.async_accept(get_lowest_layer(*ws).socket(), [this, ws](boost::system::error_code ec){
        if (ec == asio::error::operation_aborted) {
            return;
        }

        if (ec) {
            std::cerr << "Accept error: " << ec.message() << "\n";
            return;
        }

        auto userId = userManager_->registerUser();
        auto session = std::make_shared<Session>(ws, chatManager_, userManager_, userId);

        session->start();
        onAcceptAsync();
    });
}
