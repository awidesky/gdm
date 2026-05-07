#ifndef GLUTIL_DEBUG_HPP
#define GLUTIL_DEBUG_HPP

#include <glutil/debug_callback.hpp>
#include <glutil/debug_stacktrace.hpp>

#include <string>
#include <set>

namespace glutil::debug {

void init();

void dumpGLState();

std::string glErrorToString(GLenum err);
void checkGLError(const std::string& msg = {});

const std::set<std::string>& getGLExtensions();
bool hasGLExtension(const char* extName);

} // namespace glutil

#endif // GLUTIL_DEBUG_HPP