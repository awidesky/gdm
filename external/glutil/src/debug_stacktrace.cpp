#if GDM_DEBUG

#include <glutil/gl.hpp>
#include <glutil/debug_stacktrace.hpp>

#include <cpptrace/cpptrace.hpp>
#include <cpptrace/formatting.hpp>

#include <iostream>
#include <sstream>


namespace glutil::debug {
void printStackTrace(std::ostream& out, std::string header, int skip, int depth, bool snippets, int snippet_context) {
    cpptrace::formatter{}
      .header(header)                                     // header
      .colors(cpptrace::formatter::color_mode::automatic) // Color: always / none / automatic
      .addresses(cpptrace::formatter::address_mode::raw)  // Adress: raw / object / none
      .paths(cpptrace::formatter::path_mode::basename)    // Path: full / basename
      .snippets(snippets)                                 // source snippet 
      .snippet_context(snippet_context)                   // How many lines of source context to show in a snippet	
      .columns(true)                                      // column number enable
      .print(out, cpptrace::stacktrace::current(skip, depth));
}

std::string stringStackTrace(std::string header, int skip, int depth, bool snippets, int snippet_context, bool color) {
    std::ostringstream ss;
    cpptrace::formatter{}
      .header(header)                                     // header
      .colors(color ? cpptrace::formatter::color_mode::always
                    : cpptrace::formatter::color_mode::none) // color only when explicitly requested
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