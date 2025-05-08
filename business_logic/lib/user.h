#pragma once

#include <string>

#include <boost/uuid/uuid.hpp>


class User {
 public:
    using UserId = boost::uuids::uuid;

    User(const UserId& id, std::string nick_name);

    [[nodiscard]] const UserId& getId() const;
    [[nodiscard]] const std::string& getName() const;

    void setName(const std::string& newName);

 private:
    UserId      id_;
    std::string name_;
};
