#include "user.h"

#include <utility>

User::User(const UserId& id, std::string nick_name)
    : id_(id), name_(std::move(nick_name)) {}

const User::UserId& User::getId() const {
    return id_;
}

const std::string& User::getName() const {
    return name_;
}

void User::setName(const std::string& nick_name) {
    name_ = nick_name;
}
