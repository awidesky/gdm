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

std::filesystem::path getExecutableDirectory() {
#ifdef _WIN32
    char buffer[MAX_PATH];
    DWORD length = GetModuleFileNameA(nullptr, buffer, MAX_PATH);

    if (length == 0) {
        return {};
    }

    return fs::path(buffer).parent_path();

#elif __APPLE__
    char buffer[1024];
    uint32_t size = sizeof(buffer);

    if (_NSGetExecutablePath(buffer, &size) != 0) {
        return {};
    }

    return fs::canonical(buffer).parent_path();

#elif __linux__
    char buffer[1024];
    ssize_t length = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);

    if (length == -1) {
        return {};
    }

    buffer[length] = '\0';
    return fs::path(buffer).parent_path();

#else
#  error "Unknown OS in getExecutableDirectory()!"
    return {};
#endif
}

std::filesystem::path getProjectRootDirectory() { return PROJECT_ROOT; }

PathResolveResult pathResolve(const std::filesystem::path& inputPath) {
    PathResolveResult result;

    std::ostringstream message;

    auto testPath = [&](const fs::path& candidate, const std::string& description) -> bool {
        std::error_code ec;

        auto canonical = fs::weakly_canonical(candidate);
        if (fs::exists(canonical, ec)) {
            result.success = true;
            result.resolvedPath = canonical.string();

            message << "[FOUND] " << description << ": " << result.resolvedPath << '\n';
            return true;
        }

        message << "[Warning] Not found in " << description << ": " << canonical.string() << '\n';
        return false;
    };

    // 1. If absolute, use as-is; just check for existance.
    if (inputPath.is_absolute()) {
        testPath(inputPath, "Absolute path");

        result.message = message.str();
        return result;
    }

    // 2-1. Working directory base(same as fs::absolute(p))
    if (testPath(fs::current_path() / inputPath, "Working dir")) {
        result.message = message.str();
        return result;
    }

    // 2-2. Executable directory base
    auto exeDir = getExecutableDirectory();
    if (!exeDir.empty()) {
        if (testPath(exeDir / inputPath, "Executable dir")) {
            result.message = message.str();
            return result;
        }
    } else {
        message << "[Warning] Failed to get executable directory.\n";
    }

    // 2-3. PROJECT_ROOT base
    if (testPath(getProjectRootDirectory() / inputPath, "Project root dir")) {
        result.message = message.str();
        return result;
    }

    result.message = message.str();
    return result;
}

} // namespace glutil