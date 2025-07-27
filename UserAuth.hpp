#pragma once
#include <string>
#include <unordered_map>

class UserAuth {
public:
    bool loadFromFile(const std::string& filename);
    bool checkPassword(const std::string& username, const std::string& password) const;

private:
    std::unordered_map<std::string, std::string> users_; // username -> md5(password)
    static std::string sha256(const std::string& input);
};
