#ifndef GLUTIL_DEBUG_HPP
#define GLUTIL_DEBUG_HPP

#include <glutil/debug_callback.hpp>

#include <string>

namespace glutil::debug {

std::string glErrorToString(GLenum err);
void dumpGLState();
void checkGLError(const std::string& msg = {});
void init();

} // namespace glutil

#endif // GLUTIL_DEBUG_HPP