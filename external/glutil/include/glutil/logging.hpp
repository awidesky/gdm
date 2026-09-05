#ifndef GLUTIL_LOGGING_HPP
#define GLUTIL_LOGGING_HPP

#include <iostream>
#include <utility>
#include <sstream>

namespace glutil {

/**
 * Logger utility providing stream-style logging with severity levels.
 *
 * Supports separate stdout/stderr log sinks and RAII-based message flushing.
 */
class Logger {
public:
    inline static Logger& stdoutLogger() {
        static Logger instance(&std::cout);
        return instance;
    }
    inline static Logger& stderrLogger() {
        static Logger instance(&std::cerr);
        return instance;
    }

    /**
     * RAII log stream that buffers output and flushes on destruction.
     * Prefixes messages with a log level (INFO/WARNING/ERROR).
     */
    class LogStream {
    public:
        LogStream(std::ostream* out, const char* level, bool enabled, bool prefixEnabled)
         : out(out), level(level), enabled(enabled), prefixEnabled(prefixEnabled) {}

        /**
         * Flushes buffered log message on destruction.
         * Output format: [LEVEL] message
         */
        ~LogStream() {
            if (!enabled || !out) return;

            auto msg = buffer.str();
            if (msg.empty()) return;

            if (prefixEnabled) {
                (*out) << "[" << level << "] " << msg;
            } else (*out) << msg;

            if (msg.back() != '\n') (*out) << '\n';
        }

        LogStream& operator<<(std::ostream& (*m)(std::ostream&)) {
            if (enabled) buffer << m;
            return *this;
        }
        LogStream& operator<<(std::ios& (*m)(std::ios&)) {
            if (enabled) m(buffer);
            return *this;
        }
        LogStream& operator<<(std::ios_base& (*m)(std::ios_base&)) {
            if (enabled) m(buffer);
            return *this;
        }

        template <typename T> LogStream& operator<<(T&& value) {
            if (enabled) buffer << std::forward<T>(value);
            return *this;
        }

    private:
        std::ostream* out = nullptr; // Output stream target
        const char* level = "";      // Log level label
        bool enabled , prefixEnabled;// Whether logging is enabled and prefix printing is enabled
        std::ostringstream buffer;   // Internal message buffer
    };

    LogStream warning() const { return LogStream(stream, "WARNING", enabled, prefixEnabled); }
    LogStream error() const { return LogStream(stream, "ERROR", enabled, prefixEnabled); }
    LogStream info() const { return LogStream(stream, "INFO", enabled, prefixEnabled); }

    void setOutput(std::ostream* os) { stream = os; }
    std::ostream* getOutput() { return stream; }
    void enable(bool v) { enabled = v; }
    void printPrefix(bool v) { prefixEnabled = v; }

private:
    Logger(std::ostream* os) : stream(os) {}

    std::ostream* stream;
    bool enabled = true;
    bool prefixEnabled = true;
};

/**
 * Enables or disables all global loggers (stdout and stderr).
 */
inline void enableAllLoggers(bool enable) {
    Logger::stdoutLogger().enable(enable);
    Logger::stderrLogger().enable(enable);
}
/**
 * Enables or disables prefix printing for all global loggers (stdout and stderr).
 */
inline void enablePrefixAllLoggers(bool enable) {
    Logger::stdoutLogger().printPrefix(enable);
    Logger::stderrLogger().printPrefix(enable);
}
} // namespace glutil

// Macro definitions for convenient usage
#if GLUTIL_DISABLE_LOG_ON_RELEASE && !GDM_DEBUG
    #define LOG_WARNING() if constexpr(true) ; else glutil::Logger::stdoutLogger().warning()
    #define LOG_ERROR()   if constexpr(true) ; else glutil::Logger::stderrLogger().error()
    #define LOG_INFO()    if constexpr(true) ; else glutil::Logger::stdoutLogger().info()
#else
    #define LOG_WARNING() glutil::Logger::stdoutLogger().warning()
    #define LOG_ERROR()   glutil::Logger::stderrLogger().error()
    #define LOG_INFO()    glutil::Logger::stdoutLogger().info()
#endif

#endif // GLUTIL_LOGGING_HPP
