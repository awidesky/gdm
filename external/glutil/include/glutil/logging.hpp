#ifndef GLUTIL_LOGGING_HPP
#define GLUTIL_LOGGING_HPP

#include <iostream>
#include <sstream>
#include <string>

namespace glutil {

class Logger {
public:
    static Logger& getInstance();

    class LogStream {
    public:
        LogStream(const std::string& level);
        ~LogStream();

        template<typename T>
        LogStream& operator<<(const T& value) {
            buffer << value;
            return *this;
        }

        // Specialization for std::endl
        LogStream& operator<<(std::ostream& (*manipulator)(std::ostream&)) {
            buffer << manipulator;
            return *this;
        }

    private:
        std::string logLevel;
        std::stringstream buffer;
    };

    LogStream warning();
    LogStream error();
    LogStream info();

private:
    Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
};

} // namespace glutil

// Macro definitions for convenient usage
#define LOG_WARNING() glutil::Logger::getInstance().warning()
#define LOG_ERROR() glutil::Logger::getInstance().error()
#define LOG_INFO() glutil::Logger::getInstance().info()

#endif // GLUTIL_LOGGING_HPP
