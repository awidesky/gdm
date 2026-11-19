#if GDM_DEBUG

#include <glutil/gl.hpp>
#include <glutil/debug_stacktrace.hpp>

#include <cpptrace/cpptrace.hpp>
#include <cpptrace/formatting.hpp>

#include <iostream>
#include <sstream>
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

// Present, non-empty, and not literally "0"
bool envSet(const char* name) {
    const char* v = std::getenv(name);
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
    if(const char* t = std::getenv("TERM"); t && !std::strcmp(t, "dumb")) return false; // 4. TERM=dumb
    const FILE* f = fileOf(out);                                  // 5. Real terminal check
    return f && isTerminal(const_cast<FILE*>(f));
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
    cpptrace::formatter{}
      .header(header)                                     // header
      .colors(useColor ? cpptrace::formatter::color_mode::always
                       : cpptrace::formatter::color_mode::none) // policy decided here, never 'automatic'
      .addresses(cpptrace::formatter::address_mode::raw)  // Adress: raw / object / none
      .paths(cpptrace::formatter::path_mode::basename)    // Path: full / basename
      .snippets(snippets)                                 // source snippet
      .snippet_context(snippet_context)                   // How many lines of source context to show in a snippet
      .columns(true)                                      // column number enable
      .print(out, cpptrace::stacktrace::current(skip, depth));
}

std::string stringStackTrace(std::string header, int skip, int depth, bool snippets,
                             int snippet_context, ColorMode color) {
    const bool useColor = (color == ColorMode::Always) || (color == ColorMode::Auto && colorForced());
    std::ostringstream ss;
    cpptrace::formatter{}
      .header(header)                                     // header
      .colors(useColor ? cpptrace::formatter::color_mode::always
                       : cpptrace::formatter::color_mode::none)
      .addresses(cpptrace::formatter::address_mode::raw)  // Adress: raw / object / none
      .paths(cpptrace::formatter::path_mode::basename)    // Path: full / basename
      .snippets(snippets)                                 // source snippet
      .snippet_context(snippet_context)                   // How many lines of source context to show in a snippet
      .columns(true)                                      // column number enable
      .print(ss, cpptrace::stacktrace::current(skip, depth));
    return ss.str();
}

std::string getCalledGLfunctionName(int skip) {
    cpptrace::stacktrace trace = cpptrace::stacktrace::current(skip, 1);
    if (trace.frames.empty()) return "unknown source location";
    
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