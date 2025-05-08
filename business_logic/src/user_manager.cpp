#include "user_manager.h"

UserManager::UserId UserManager::registerUser(const std::string& nickname) {
    std::unique_lock lock(mutex_);

    UserId id = generator_();
    users_.emplace(id, User(id, nickname));
    return id;
}

bool UserManager::renameUser(User::UserId id, const std::string& newName) {
    std::unique_lock lock(mutex_);

    auto it = users_.find(id);
    if (it == users_.end()) {
        return false;
    }

    it->second.setName(newName);
    return true;
}

std::optional<std::reference_wrapper<User>> UserManager::getUser(const UserId& id) {
    std::shared_lock lock(mutex_);

    auto it = users_.find(id);
    if (it != users_.end()) {
        return it->second;
    }

    return std::nullopt;
}

bool UserManager::userExists(const UserId& id) const {
    std::shared_lock lock(mutex_);

    return users_.find(id) != users_.end();
}

std::vector<std::pair<User::UserId, std::string>> UserManager::getAllUsers() const {
    std::vector<std::pair<User::UserId, std::string>> out;
    for (auto const& [id, user] : users_) {
        out.emplace_back(id, user.getName());
    }

    return out;
}

bool UserManager::nameExists(const std::string& name) const {
    std::shared_lock lock(mutex_);
    for (auto const& [id, user] : users_) {
        if (user.getName() == name) return true;
    }
    return false;
}

std::optional<UserManager::UserId> UserManager::findByName(const std::string& name) const {
    std::shared_lock lock(mutex_);
    for (auto const& [id, user] : users_) {
        if (user.getName() == name) return id;
    }
    return std::nullopt;
}

void UserManager::setLoggedIn(UserId id, bool loggedIn) {
    std::unique_lock lock(mutex_);
    if (loggedIn) {
        loggedIn_.insert(id);
    } else {
        loggedIn_.erase(id);
    }
}

bool UserManager::isLoggedIn(UserId id) const {
    std::shared_lock lock(mutex_);
    return loggedIn_.count(id) != 0;
}