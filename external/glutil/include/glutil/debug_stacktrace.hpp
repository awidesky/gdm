#ifndef GLUTIL_DEBUG_STACKTRACE_HPP
#define GLUTIL_DEBUG_STACKTRACE_HPP

#include <string>

namespace glutil::debug {
void printStackTrace(std::string header = "Stack trace:", int skip = 3, int depth = 15, bool snippets = true,
                     int snippet_context = 2);

std::string getCalledGLfunctionName(int skip = 4);
}

#endif //GLUTIL_DEBUG_STACKTRACE_HPP