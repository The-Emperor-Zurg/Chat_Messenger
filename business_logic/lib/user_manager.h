#pragma once

#include <unordered_map>
#include <optional>
#include <shared_mutex>
#include <unordered_set>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/random_generator.hpp>

#include "user.h"

class UserManager {
public:
    using UserId = boost::uuids::uuid;

    UserManager() = default;
    UserId registerUser(const std::string& nickname = "Anonymous");
    bool renameUser(User::UserId id, const std::string& newName);
    std::optional<std::reference_wrapper<User>> getUser(const UserId& id);
    [[nodiscard]] bool userExists(const UserId& id) const;
    std::vector<std::pair<User::UserId, std::string>> getAllUsers() const;
    bool nameExists(const std::string& name) const;
    std::optional<UserId> findByName(const std::string& name) const;
    void setLoggedIn(UserId id, bool loggedIn);
    bool isLoggedIn(UserId id) const;

private:
    std::unordered_map<UserId, User> users_;
    boost::uuids::random_generator generator_;
    mutable std::shared_mutex mutex_;
    std::unordered_set<UserId> loggedIn_;
};
