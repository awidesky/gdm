#ifndef GLUTIL_PATH_HPP
#define GLUTIL_PATH_HPP

#include <glutil/gl.hpp>

#include <filesystem>

namespace glutil {

struct PathResolveResult {
    bool success = false;
    std::string resolvedPath{};
    std::string message;
};

std::string getExecutableDirectory();
PathResolveResult pathResolve(const std::filesystem::path& inputPath);

} // namespace glutil

#endif // GLUTIL_PATH_HPP
