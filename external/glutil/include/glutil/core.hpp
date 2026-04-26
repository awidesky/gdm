#ifndef GLUTIL_CORE_HPP
#define GLUTIL_CORE_HPP

// TODO : rename the header to pathResolve or something and fix pathresolve feature

#include <glutil/gl.hpp>

#include <string>
#include <vector>

namespace glutil {

struct LogEntry {
    std::string type;
    std::string status;
    std::string path;
    std::string message;
};

struct PathResolveResult {
    bool success = false;
    std::string originalPath;
    std::string resolvedPath;
    std::string message;
};

PathResolveResult pathResolve(const std::string& inputPath);

class ResourceLogger {
public:
    static void addLog(const LogEntry& entry);
    static void printLogs();
    static void clearLogs();
    static const std::vector<LogEntry>& getLogs();

private:
    static std::vector<LogEntry> logs;
};

} // namespace glutil

#endif // GLUTIL_CORE_HPP