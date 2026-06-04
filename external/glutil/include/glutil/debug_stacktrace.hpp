#ifndef GLUTIL_DEBUG_STACKTRACE_HPP
#define GLUTIL_DEBUG_STACKTRACE_HPP

#include <string>

namespace glutil::debug {

#if GDM_DEBUG

void printStackTrace(std::string header = "Stack trace:", int skip = 3, int depth = 15, bool snippets = true,
                     int snippet_context = 2);

std::string getCalledGLfunctionName(int skip = 4);

#else
inline void printStackTrace(std::string h = "", int sk = 0, int d = 0, bool s = false, int sc = 0) noexcept {
    (void)h; (void)sk; (void)d; (void)s; (void)sc;
}
inline std::string getCalledGLfunctionName(int skip = 0) noexcept { (void)skip; return ""; }
#endif

} // namespace glutil::debug

#endif //GLUTIL_DEBUG_STACKTRACE_HPP