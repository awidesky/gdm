#ifndef GLUTIL_DEBUG_STACKTRACE_HPP
#define GLUTIL_DEBUG_STACKTRACE_HPP

#include <string>

namespace glutil::debug {

#if GDM_DEBUG

/**
 * Prints a formatted stack trace to stderr using cpptrace.
 *
 * Stack trace configuration:
 * - skip: number of top frames to ignore
 * - depth: maximum number of frames to print
 * - snippets: enables source code snippets per frame
 * - snippet_context: number of surrounding source lines
 *
 * Output is handled by cpptrace::formatter and written to std::cerr.
 */
void printStackTrace(std::string header = "Stack trace:", int skip = 3, int depth = 15, bool snippets = true,
                     int snippet_context = 2);

/**
 * Extracts a line of code from a single-frame stack trace in given depth.
 * 
 * This function is used to get the GL function call line from gladPostAallback, hence the name.
 *
 * @param skip Number of stack frames to skip before capturing.
 * @return something like: "    > 207 : GLuint shader = glCreateShader(type);"
 */
std::string getCalledGLfunctionName(int skip = 4);

#else
/** Debug stub: stack trace printing disabled in release builds. */
inline void printStackTrace(std::string h = "", int sk = 0, int d = 0, bool s = false, int sc = 0) noexcept {
    (void)h; (void)sk; (void)d; (void)s; (void)sc;
}
/** Debug stub: returns empty string in release builds. */
inline std::string getCalledGLfunctionName(int skip = 0) noexcept { (void)skip; return ""; }
#endif

} // namespace glutil::debug

#endif //GLUTIL_DEBUG_STACKTRACE_HPP