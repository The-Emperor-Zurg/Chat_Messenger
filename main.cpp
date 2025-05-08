#include <iostream>
#include <csignal>
#include <memory>

#include "web/lib/server.h"

std::unique_ptr<Server> serverPtr;

void signalHandler(int signum) {
    std::cout << "\nReceived signal " << signum << ", shutting down server..." << std::endl;
    if (serverPtr) {
        serverPtr->stop();
    }
    std::_Exit(0);
}

int main() {
    const std::size_t threadCount = 2;
    const std::size_t port = 8080;

    std::cout << "Starting server on port " << port
              << " with " << threadCount << " threads..." << std::endl;

    serverPtr = std::make_unique<Server>(threadCount, port);

    {
        auto um = serverPtr->getUserManager();
        auto id1 = um->registerUser();
        um->renameUser(id1, "Maximus");
        auto id2 = um->registerUser();
        um->renameUser(id2, "Patroculus");
    }

    try {
        serverPtr->start();
    } catch (const std::exception& ex) {
        std::cerr << "Server error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
