#include <glutil/debug_stacktrace.hpp>
#include <cpptrace/cpptrace.hpp>
#include <cpptrace/formatting.hpp>
#include <iostream>


namespace glutil::debug {
void printStackTrace(std::string header, int skip, int depth, bool snippets, int snippet_context) {
    cpptrace::formatter{}
      .header(header)                                     // header
      .colors(cpptrace::formatter::color_mode::always)    // Color: always / none / automatic
      .addresses(cpptrace::formatter::address_mode::raw)  // Adress: raw / object / none
      .paths(cpptrace::formatter::path_mode::basename)    // Path: full / basename
      .snippets(snippets)                                 // source snippet 
      .snippet_context(snippet_context)                   // How many lines of source context to show in a snippet	
      .columns(true)                                      // column number enable
      .print(std::cerr, cpptrace::stacktrace::current(skip, depth), false);
}
} 