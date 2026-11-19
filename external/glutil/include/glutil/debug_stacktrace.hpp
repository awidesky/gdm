#ifndef GLUTIL_DEBUG_STACKTRACE_HPP
#define GLUTIL_DEBUG_STACKTRACE_HPP

#include <string>
#include <iostream>

namespace glutil::debug {

/**
 * Color mode for stack trace output.
 * Mirrors the conventional --color=auto|always|never CLI semantics.
 */
enum class ColorMode {
    Auto,    // Decide based on output stream and environment
    Always,  // Force color on
    Never    // Force color off
};

#if GDM_DEBUG

/**
 * Returns true when the force-color environment variable is set (FORCE_COLOR).
 * Overrides every heuristic, including NO_COLOR.
 */
bool colorForced();

/**
 * Decides whether ANSI color should be emitted for output written to `out`.
 *
 * Checked in order, first match wins:
 *   1. NO_COLOR set                       -> off (highest priority)
 *   2. FORCE_COLOR set                    -> on
 *   3. Xcode console (XCODE_VERSION_ACTUAL and not TERM_PROGRAM) -> off
 *   4. TERM=dumb                          -> off
 *   5. isatty() on the stream's own fd    -> that result
 *
 * Streams with no mappable file descriptor (string streams, custom sinks)
 * return false; use colorForced() for those.
 * Note: this is only compiled in debug builds (cpptrace is debug-only).
 */
bool colorEnabled(std::ostream& out = std::cerr);

/**
 * Prints a formatted stack trace to the given output stream using cpptrace.
 *
 * Stack trace configuration:
 * - skip: number of top frames to ignore
 * - depth: maximum number of frames to print
 * - snippets: enables source code snippets per frame
 * - snippet_context: number of surrounding source lines
 *
 * Color defaults to Auto (decide based on stream and environment).
 * Pass Always/Never to override.
 */
void printStackTrace(std::ostream& out = std::cerr, std::string header = "Stack trace:", int skip = 3,
                     int depth = 15, bool snippets = true, int snippet_context = 2,
                     ColorMode color = ColorMode::Auto);

/**
 * Formats a stack trace into a std::string and returns it.
 *
 * The result has no file descriptor to probe, so color is off unless
 * explicitly requested (Always) or forced via the environment (FORCE_COLOR).
 * Only enable when you know the string will land on an ANSI-capable sink.
 */
std::string stringStackTrace(std::string header = "Stack trace:", int skip = 3, int depth = 15,
                             bool snippets = true, int snippet_context = 2, ColorMode color = ColorMode::Never);

/**
 * Extracts the GL call site from a single-frame stack trace at given depth.
 *
 * Uses cpptrace's stacktrace_frame data (filename/line) and cpptrace::get_snippet.
 * If the source file is not available on the machine, only "file:line" is returned;
 * if even line info is missing, returns "unknown source location".
 *
 * @param skip Number of stack frames to skip before capturing.
 * @return something like: "tracker.cpp:39 -> glBindBuffer(GL_ARRAY_BUFFER, VBO);"
 */
std::string getCalledGLfunctionName(int skip = 4);

#else
/** Debug stub: color policy is a debug-only concern. */
inline bool colorForced() noexcept { return false; }
inline bool colorEnabled(std::ostream& out = std::cerr) noexcept { (void)out; return false; }
/** Debug stub: stack trace printing disabled in release builds. */
inline void printStackTrace(std::ostream& out = std::cerr, std::string h = "", int sk = 0, int d = 0,
                            bool s = false, int sc = 0, ColorMode color = ColorMode::Auto) noexcept {
    (void)out; (void)h; (void)sk; (void)d; (void)s; (void)sc; (void)color;
}
/** Debug stub: returns empty string in release builds. */
inline std::string stringStackTrace(std::string h = "", int sk = 0, int d = 0, bool s = false, int sc = 0,
                                    ColorMode color = ColorMode::Never) noexcept {
    (void)h; (void)sk; (void)d; (void)s; (void)sc; (void)color; return "";
}
/** Debug stub: returns empty string in release builds. */
inline std::string getCalledGLfunctionName(int skip = 0) noexcept { (void)skip; return ""; }
#endif

} // namespace glutil::debug

#endif //GLUTIL_DEBUG_STACKTRACE_HPP