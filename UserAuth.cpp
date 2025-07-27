#include "UserAuth.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h> // Needs OpenSSL

bool UserAuth::loadFromFile(const std::string& filename) {
    users_.clear();
    std::ifstream file(filename);
    if (!file) return false;
    std::string line;
    while (std::getline(file, line)) {
        auto colon = line.find(':');
        if (colon == std::string::npos) continue;
        std::string user = line.substr(0, colon);
        std::string hash = line.substr(colon + 1);
        users_[user] = hash;
    }
    return true;
}

bool UserAuth::checkPassword(const std::string& username, const std::string& password) const {
    auto it = users_.find(username);
    if (it == users_.end()) return false;
    return sha256(password) == it->second;
}

// Uses OpenSSL SHA256
std::string UserAuth::sha256(const std::string& input) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.size(), digest);

    std::ostringstream oss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
    return oss.str();
}