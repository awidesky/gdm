#if GDM_DEBUG

#include <glutil/gl.hpp>
#include <glutil/debug_stacktrace.hpp>

#include <cpptrace/cpptrace.hpp>
#include <cpptrace/formatting.hpp>

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <sstream>
#include <string_view>
#include <regex>

#include <cstdio>    // FILE, stdout, stderr, fileno
#include <cstdlib>   // getenv
#include <cstring>   // strcmp
#ifdef _WIN32
#include <io.h>      // _isatty, _fileno
#else
#include <unistd.h>  // isatty
#endif


namespace glutil::debug {
namespace {

// Environment variable names
constexpr const char* kForce   = "FORCE_COLOR";   // force-color.org informal standard
constexpr const char* kNoColor = "NO_COLOR";      // no-color.org informal standard

// Cross-platform safe getenv wrapper
const char* safe_getenv(const char* name) {
#ifdef _WIN32
    size_t len = 0;
    char* buf = nullptr;
    if (_dupenv_s(&buf, &len, name) == 0 && buf) {
        // Note: buf is allocated by _dupenv_s, caller must free
        // We'll use a static thread_local buffer to avoid leaks
        static thread_local std::string cached;
        cached = buf;
        free(buf);
        return cached.c_str();
    }
    return nullptr;
#else
    return std::getenv(name);
#endif
}

// Present, non-empty, and not literally "0"
bool envSet(const char* name) {
    const char* v = safe_getenv(name);
    return v && *v && std::strcmp(v, "0") != 0;
}

// cout/cerr/clog are the only std streams reliably mappable to an fd:
// libc++ backs them with internal __stdoutbuf, and sync_with_stdio(false)
// swaps the buffer type at runtime, so rdbuf() probing is not dependable.
FILE* fileOf(std::ostream& out) {
    if(&out == &std::cout) return stdout;
    if(&out == &std::cerr || &out == &std::clog) return stderr;
    return nullptr;
}

bool isTerminal(FILE* f) {
#ifdef _WIN32
    return _isatty(_fileno(f)) != 0;
#else
    return isatty(fileno(f)) != 0;
#endif
}

bool resolveColor(ColorMode mode, std::ostream& out) {
    if(mode == ColorMode::Always) return true;
    if(mode == ColorMode::Never)  return false;
    // Auto: use environment + stream detection
    if(envSet(kNoColor)) return false;                            // 1. NO_COLOR (highest priority)
    if(envSet(kForce)) return true;                               // 2. FORCE_COLOR
    if(envSet("XCODE_VERSION_ACTUAL") && !envSet("TERM_PROGRAM")) return false; // 3. Xcode console (but not Terminal.app/iTerm)
    if(const char* t = safe_getenv("TERM"); t && !std::strcmp(t, "dumb")) return false; // 4. TERM=dumb
    const FILE* f = fileOf(out);                                  // 5. Real terminal check
    return f && isTerminal(const_cast<FILE*>(f));
}

// A frame belongs to the GL debug-callback machinery when its function is one of
// these. Matching on the symbol name (prefix) instead of counting frames keeps this
// correct under optimization: an inlined callee still gets its own (inline) frame
// from cpptrace, a tail-called wrapper is simply absent, and compiler suffixes such
// as ".constprop.0" / "[clone .cold]" are absorbed by the prefix match.
// glutil's own code that calls GL (shader.cpp, ...) is deliberately NOT listed:
// its GL calls are reported at their own location.
bool isInternalFrame(const cpptrace::stacktrace_frame& frame) {
    static constexpr std::string_view kPrefixes[] = {
        "glutil::debug::callbacks::",
        "glutil::debug::printStackTrace",
        "glutil::debug::stringStackTrace",
        "glutil::debug::getCalledGLfunctionName",
        "glad_debug_impl_",
    };
    const std::string_view symbol = frame.symbol;
    for (std::string_view prefix : kPrefixes)
        if (symbol.substr(0, prefix.size()) == prefix) return true;
    return false;
}

// How many extra frames are captured so the internal ones at the top can be found.
constexpr std::size_t kInternalScanDepth = 12;

// Drops the leading run of internal frames, then `extraSkip` more, and keeps at most
// `depth` frames. `trace` must have been captured inside one of the public functions
// below (never inside a helper), so its top frame is always an internal one.
// Returns false when that top frame is not recognized: the binary carries no usable
// symbols, nothing can be classified, and `trace` is left untouched.
bool keepCallerFrames(cpptrace::stacktrace& trace, int extraSkip, int depth) {
    auto& frames = trace.frames;
    if (frames.empty() || !isInternalFrame(frames.front())) return false;

    auto firstUser = std::find_if_not(frames.begin(), frames.end(), isInternalFrame);
    const auto remaining = static_cast<std::size_t>(frames.end() - firstUser);
    firstUser += static_cast<std::ptrdiff_t>(
        std::min<std::size_t>(remaining, static_cast<std::size_t>(std::max(extraSkip, 0))));
    frames.erase(frames.begin(), firstUser);

    const auto keep = static_cast<std::size_t>(std::max(depth, 0));
    if (frames.size() > keep) frames.erase(frames.begin() + static_cast<std::ptrdiff_t>(keep), frames.end());
    return true;
}

// Frames to capture: the internal ones + what the caller asked to skip + what it wants.
std::size_t captureDepth(int skip, int depth) {
    return kInternalScanDepth + static_cast<std::size_t>(std::max(skip, 0))
                              + static_cast<std::size_t>(std::max(depth, 0));
}

} // namespace

bool colorForced() {
    return envSet(kForce);
}

bool colorEnabled(std::ostream& out) {
    return resolveColor(ColorMode::Auto, out);
}

void printStackTrace(std::ostream& out, std::string header, int skip, int depth, bool snippets,
                     int snippet_context, ColorMode color) {
    const bool useColor = resolveColor(color, out);
    cpptrace::stacktrace trace = cpptrace::stacktrace::current(0, captureDepth(skip, depth));
    if (!keepCallerFrames(trace, skip, depth)) {
        out << "[glutil] no symbols available: internal frames cannot be skipped, raw trace follows\n";
    }
    cpptrace::formatter{}
      .header(header)                                     // header
      .colors(useColor ? cpptrace::formatter::color_mode::always
                       : cpptrace::formatter::color_mode::none) // policy decided here, never 'automatic'
      .addresses(cpptrace::formatter::address_mode::raw)  // Adress: raw / object / none
      .paths(cpptrace::formatter::path_mode::basename)    // Path: full / basename
      .snippets(snippets)                                 // source snippet
      .snippet_context(snippet_context)                   // How many lines of source context to show in a snippet
      .columns(true)                                      // column number enable
      .print(out, trace);
}

std::string stringStackTrace(std::string header, int skip, int depth, bool snippets,
                             int snippet_context, ColorMode color) {
    const bool useColor = (color == ColorMode::Always) || (color == ColorMode::Auto && colorForced());
    std::ostringstream ss;
    cpptrace::stacktrace trace = cpptrace::stacktrace::current(0, captureDepth(skip, depth));
    if (!keepCallerFrames(trace, skip, depth)) {
        ss << "[glutil] no symbols available: internal frames cannot be skipped, raw trace follows\n";
    }
    cpptrace::formatter{}
      .header(header)                                     // header
      .colors(useColor ? cpptrace::formatter::color_mode::always
                       : cpptrace::formatter::color_mode::none)
      .addresses(cpptrace::formatter::address_mode::raw)  // Adress: raw / object / none
      .paths(cpptrace::formatter::path_mode::basename)    // Path: full / basename
      .snippets(snippets)                                 // source snippet
      .snippet_context(snippet_context)                   // How many lines of source context to show in a snippet
      .columns(true)                                      // column number enable
      .print(ss, trace);
    return ss.str();
}

std::string getCalledGLfunctionName(int skip) {
    cpptrace::stacktrace trace = cpptrace::stacktrace::current(0, captureDepth(skip, 1));
    // No symbols means no file/line either, so there is nothing to report.
    if (!keepCallerFrames(trace, skip, 1) || trace.frames.empty()) return "unknown source location";

    const cpptrace::stacktrace_frame& frame = trace.frames.front();
    if (!frame.line.has_value() || frame.filename.empty()) return "unknown source location";

    // get_snippet returns "       > 39: glBufferData(GL_ARRAY_BUFFER, ...);"
    // or "" when the source file is not available (e.g. artifact run on another machine)
    std::string code;
    std::string snippet = cpptrace::get_snippet(frame.filename, frame.line.value(), 0, false);
    std::size_t marker = snippet.find('>');
    if (marker != std::string::npos) {
        std::size_t colon = snippet.find(':', marker);
        if (colon != std::string::npos) {
            code = snippet.substr(colon + 1);
            const char* ws = " \t\r\n";
            std::size_t begin = code.find_first_not_of(ws);
            std::size_t end = code.find_last_not_of(ws);
            if (begin == std::string::npos) {
                code.clear();
            } else {
                code = code.substr(begin, end - begin + 1);
            }
        }
    }

    std::string location = cpptrace::basename(frame.filename) + ":" + std::to_string(frame.line.value());
    return code.empty() ? location : location + " -> " + code;
}
} // namespace glutil::debug
#endif