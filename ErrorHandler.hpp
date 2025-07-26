#pragma once
#include <string>

class ErrorHandler {
public:
    static void handleError(const std::string& msg, bool fatal = false);
};
