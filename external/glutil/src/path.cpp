#include <glutil/path.hpp>
#include <glutil/logging.hpp>
#include "config.hpp"

#include <filesystem>
#include <sstream>
#include <string_view>

#ifdef _WIN32
#include <Windows.h>
#elif __APPLE__
#include <mach-o/dyld.h>
#elif __linux__
#include <unistd.h>
#endif

namespace glutil {

namespace fs = std::filesystem;

std::string getExecutableDirectory() {
#ifdef _WIN32
    char buffer[MAX_PATH];
    DWORD length = GetModuleFileNameA(nullptr, buffer, MAX_PATH);

    if (length == 0) {
        return {};
    }

    return fs::path(buffer).parent_path().string();

#elif __APPLE__
    char buffer[1024];
    uint32_t size = sizeof(buffer);

    if (_NSGetExecutablePath(buffer, &size) != 0) {
        return {};
    }

    return fs::canonical(buffer).parent_path().string();

#elif __linux__
    char buffer[1024];
    ssize_t length = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);

    if (length == -1) {
        return {};
    }

    buffer[length] = '\0';
    return fs::path(buffer).parent_path().string();

#else
#  error "Unknown OS in getExecutableDirectory()!"
    return {};
#endif
}

PathResolveResult pathResolve(const std::filesystem::path& inputPath) {
    PathResolveResult result;

    std::ostringstream message;

    auto testPath = [&](const fs::path& candidate, const std::string& description) -> bool {
        std::error_code ec;

        if (fs::exists(candidate, ec)) {
            result.success = true;
            result.resolvedPath = fs::weakly_canonical(candidate).string();

            message << "[Path Resolved] " << description << " : " << result.resolvedPath << '\n';
            return true;
        }

        message << "[Warning] Path not found (" << description << ") : " << candidate.string() << '\n';
        return false;
    };

    // 1. If absolute, use as-is; just check for existance.
    if (inputPath.is_absolute()) {
        testPath(inputPath, "Absolute path");

        result.message = message.str();
        return result;
    }

    // 2-1. Working directory base(same as fs::absolute(p))
    if (testPath(fs::current_path() / inputPath, "Working directory ")) {
        result.message = message.str();
        return result;
    }

    // 2-2. Executable directory base
    std::string exeDir = getExecutableDirectory();
    if (!exeDir.empty()) {
        if (testPath(fs::path(exeDir) / inputPath, "Executable directory")) {
            result.message = message.str();
            return result;
        }
    } else {
        message << "[Warning] Failed to get executable directory.\n";
    }

    // 2-3. PROJECT_ROOT base
    if (testPath(fs::path(PROJECT_ROOT) / inputPath, "PROJECT_ROOT")) {
        result.message = message.str();
        return result;
    }

    result.message = message.str();
    return result;
}

} // namespace glutil