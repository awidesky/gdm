#ifndef GLUTIL_DEBUG_STACKTRACE_HPP
#define GLUTIL_DEBUG_STACKTRACE_HPP

#include <string>
#include <iostream>

namespace glutil::debug {

#if GDM_DEBUG

/**
 * Prints a formatted stack trace to the given output stream using cpptrace.
 *
 * Stack trace configuration:
 * - skip: number of top frames to ignore
 * - depth: maximum number of frames to print
 * - snippets: enables source code snippets per frame
 * - snippet_context: number of surrounding source lines
 *
 * Output is handled by cpptrace::formatter and written to `out` (defaults to std::cerr).
 */
void printStackTrace(std::ostream& out = std::cerr, std::string header = "Stack trace:", int skip = 3,
                     int depth = 15, bool snippets = true, int snippet_context = 2);

/**
 * Formats a stack trace into a std::string and returns it.
 *
 * Same configuration as printStackTrace(), but the result is captured into a
 * string instead of being written to a stream. Color is disabled by default
 * so the returned string contains no ANSI escape sequences. Enable color
 * only when you know the string will be written to a terminal (e.g. via
 * Logger::getOutput()).
 */
std::string stringStackTrace(std::string header = "Stack trace:", int skip = 3, int depth = 15,
                             bool snippets = true, int snippet_context = 2, bool color = false);

/**
 * Extracts the GL call site from a single-frame stack trace at given depth.
 *
 * Uses cpptrace's stacktrace_frame data (filename/line) and cpptrace::get_snippet.
 * If the source file is not available on the machine, only "file:line" is returned;
 * if even line info is missing, returns "unknown location".
 *
 * @param skip Number of stack frames to skip before capturing.
 * @return something like: "tracker.cpp:39 -> glBindBuffer(GL_ARRAY_BUFFER, VBO);"
 */
std::string getCalledGLfunctionName(int skip = 4);

#else
/** Debug stub: stack trace printing disabled in release builds. */
inline void printStackTrace(std::ostream& out = std::cerr, std::string h = "", int sk = 0, int d = 0,
                            bool s = false, int sc = 0) noexcept {
    (void)out; (void)h; (void)sk; (void)d; (void)s; (void)sc;
}
/** Debug stub: returns empty string in release builds. */
inline std::string stringStackTrace(std::string h = "", int sk = 0, int d = 0, bool s = false, int sc = 0,
                                    bool color = false) noexcept {
    (void)h; (void)sk; (void)d; (void)s; (void)sc; (void)color; return "";
}
/** Debug stub: returns empty string in release builds. */
inline std::string getCalledGLfunctionName(int skip = 0) noexcept { (void)skip; return ""; }
#endif

} // namespace glutil::debug

#endif //GLUTIL_DEBUG_STACKTRACE_HPP