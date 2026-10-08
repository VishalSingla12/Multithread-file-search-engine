#pragma once

#include <string>
#include <mutex>
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

enum class LogLevel {
    INFO,
    WARNING,
    ERR
};

class Logger {
public:
    static Logger& instance();

    void setLogFile(const std::string& filepath);
    void setMinLevel(LogLevel level);
    void enableConsole(bool enabled);

    void log(LogLevel level, const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

private:
    Logger();
    ~Logger();

    std::string levelToString(LogLevel level) const;
    std::string currentTimestamp() const;

    mutable std::mutex mutex_;
    std::ofstream logFile_;
    LogLevel minLevel_{LogLevel::INFO};
    bool consoleEnabled_{true};
};

// Convenience macros
#define LOG_INFO(msg)    Logger::instance().info(msg)
#define LOG_WARNING(msg) Logger::instance().warning(msg)
#define LOG_ERROR(msg)   Logger::instance().error(msg)
