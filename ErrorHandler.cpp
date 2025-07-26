#include "ErrorHandler.hpp"
#include "Logger.hpp"
#include <cstdlib>

void ErrorHandler::handleError(const std::string& msg, bool fatal) {
    Logger::log(Logger::ERROR, msg);
    if (fatal) {
        std::exit(EXIT_FAILURE);
    }
}
