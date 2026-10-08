#include "Logger.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

Logger::Logger() = default;

Logger::~Logger() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (logFile_.is_open()) {
        logFile_.close();
    }
}

void Logger::setLogFile(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (logFile_.is_open()) logFile_.close();
    logFile_.open(filepath, std::ios::app);
}

void Logger::setMinLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    minLevel_ = level;
}

void Logger::enableConsole(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    consoleEnabled_ = enabled;
}

void Logger::log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (level < minLevel_) return;

    std::string line = "[" + currentTimestamp() + "] [" + levelToString(level) + "] " + message;

    if (consoleEnabled_) {
        if (level == LogLevel::ERR) {
            std::cerr << line << '\n';
        } else {
            std::cout << line << '\n';
        }
    }
    if (logFile_.is_open()) {
        logFile_ << line << '\n';
        logFile_.flush();
    }
}

void Logger::info(const std::string& message)    { log(LogLevel::INFO,    message); }
void Logger::warning(const std::string& message) { log(LogLevel::WARNING, message); }
void Logger::error(const std::string& message)   { log(LogLevel::ERR,     message); }

std::string Logger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARN";
        case LogLevel::ERR:     return "ERROR";
    }
    return "UNKNOWN";
}

std::string Logger::currentTimestamp() const {
    using namespace std::chrono;
    auto now   = system_clock::now();
    auto time  = system_clock::to_time_t(now);
    auto ms    = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    std::ostringstream oss;
    struct tm tmBuf{};
#ifdef _WIN32
    localtime_s(&tmBuf, &time);
#else
    localtime_r(&time, &tmBuf);
#endif
    oss << std::put_time(&tmBuf, "%H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}
