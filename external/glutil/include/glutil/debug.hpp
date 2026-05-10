#ifndef GLUTIL_DEBUG_HPP
#define GLUTIL_DEBUG_HPP

#include <glutil/debug_callback.hpp>
#include <glutil/debug_stacktrace.hpp>
#include <glutil/debug_info.hpp>

#include <string>

namespace glutil::debug {

void init();

void dumpGLState();

std::string glErrorToString(GLenum err);
void checkGLError(const std::string& msg = {});

} // namespace glutil::debug

#endif // GLUTIL_DEBUG_HPP