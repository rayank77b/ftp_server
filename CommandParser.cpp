#include "CommandParser.hpp"
#include <algorithm>

std::tuple<std::string, std::string> CommandParser::parse(const std::string& line) {
    std::string trimmed = line;
    // Remove trailing \r\n or \n
    while (!trimmed.empty() && (trimmed.back() == '\r' || trimmed.back() == '\n')) {
        trimmed.pop_back();
    }
    // Split command and argument
    auto space = trimmed.find(' ');
    if (space == std::string::npos) {
        std::string cmd = trimmed;
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
        return {cmd, ""};
    }
    std::string cmd = trimmed.substr(0, space);
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
    std::string arg = trimmed.substr(space + 1);
    return {cmd, arg};
}
