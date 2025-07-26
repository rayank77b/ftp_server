#pragma once
#include <string>
#include <fstream>
#include <mutex>

class Logger {
public:
    enum Level { INFO, WARNING, ERROR };

     // Initialize with optional log file name; call once at program start
    static void init(const std::string& filename = "");

    static void log(Level level, const std::string& msg);

    // Close the log file when done (optional, called by main)
    static void close();

private:
    static std::ofstream log_file_;
    static bool file_enabled_;
    static std::mutex log_mutex_;
};
