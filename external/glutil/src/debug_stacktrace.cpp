#include <cpptrace/cpptrace.hpp>
#include <cpptrace/formatting.hpp>
#include "debug_stacktrace.hpp"
#include <iostream>


namespace glutil {
void printStackTrace(int skip, int depth) {
    cpptrace::formatter{}
      .header("Stack trace:")                            // header
      .colors(cpptrace::formatter::color_mode::none)     // Color: always / none / automatic
      .addresses(cpptrace::formatter::address_mode::raw) // Adress: raw / object / none
      .paths(cpptrace::formatter::path_mode::basename)   // Path: full / basename
      .snippets(true)                                    // source snippet 
      .columns(true)                                     // column number enable
      .print(std::cerr, cpptrace::stacktrace::current(1, 15), false);
}
} 