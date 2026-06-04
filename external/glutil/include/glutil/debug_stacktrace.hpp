#ifndef GLUTIL_DEBUG_STACKTRACE_HPP
#define GLUTIL_DEBUG_STACKTRACE_HPP

#include <string>

#if GLUTIL_DEBUG

// DEBUG TODO : the stacktrace library itself must excluded
namespace glutil::debug {
void printStackTrace(std::string header = "Stack trace:", int skip = 3, int depth = 15, bool snippets = true,
                     int snippet_context = 2);

std::string getCalledGLfunctionName(int skip = 4);
}

#else
namespace glutil::debug {
inline void printStackTrace(std::string, int, int, bool, int) noexcept {}
inline std::string getCalledGLfunctionName(int) noexcept { return ""; }
} // namespace glutil::debug
#endif

#endif //GLUTIL_DEBUG_STACKTRACE_HPP