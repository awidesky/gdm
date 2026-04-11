#include <glutil/logging.hpp>

namespace glutil {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::LogStream::LogStream(const std::string& level)
    : logLevel(level) {
}

Logger::LogStream::~LogStream() {
    std::string message = buffer.str();
    if (!message.empty()) {
        std::cerr << "[" << logLevel << "] " << message;
        if (message.back() != '\n') {
            std::cerr << std::endl;
        }
    }
}

Logger::LogStream Logger::warning() {
    return LogStream("WARNING");
}

Logger::LogStream Logger::error() {
    return LogStream("ERROR");
}

Logger::LogStream Logger::info() {
    return LogStream("INFO");
}

} // namespace glutil
