#include "Logger.hpp"
#include <iostream>
#include <ctime>


std::ofstream Logger::log_file_;
bool Logger::file_enabled_ = false;
std::mutex Logger::log_mutex_;

void Logger::init(const std::string& filename) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    if (!filename.empty()) {
        log_file_.open(filename, std::ios::app);
        if (log_file_) {
            file_enabled_ = true;
        } else {
            std::cerr << "[Logger] Failed to open log file: " << filename << std::endl;
            file_enabled_ = false;
        }
    }
}

void Logger::close() {
    std::lock_guard<std::mutex> lock(log_mutex_);
    if (file_enabled_) {
        log_file_.close();
        file_enabled_ = false;
    }
}

void Logger::log(Level level, const std::string& msg) {
    std::lock_guard<std::mutex> lock(log_mutex_);

    // Get timestamp
    std::time_t now = std::time(nullptr);
    char tbuf[32];
    std::strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));


    const char* level_str = "";
    switch (level) {
        case INFO:      level_str = "[INFO] "; break;
        case WARNING:   level_str = "[WARN] "; break;
        case ERROR:     level_str = "[ERROR] "; break;
    }
    std::string out = std::string("[") + tbuf + "] " + level_str + msg;

    // Log to console
    std::cout << out << std::endl;

    // Log to file (if enabled)
    if (file_enabled_) {
        log_file_ << out << std::endl;
    }
}
