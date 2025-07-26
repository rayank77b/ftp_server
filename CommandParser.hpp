#pragma once
#include <string>
#include <tuple>

class CommandParser {
public:
    // Returns (cmd, argument)
    static std::tuple<std::string, std::string> parse(const std::string& line);
};
