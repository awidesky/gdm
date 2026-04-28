#ifndef GLUTIL_PATH_HPP
#define GLUTIL_PATH_HPP

#include <glutil/gl.hpp>

#include <string>

namespace glutil {

struct PathResolveResult {
    bool success = false;
    std::string originalPath;
    std::string resolvedPath;
    std::string message;
};

PathResolveResult pathResolve(const std::string& inputPath);

} // namespace glutil

#endif // GLUTIL_PATH_HPP
