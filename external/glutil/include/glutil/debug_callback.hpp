#ifndef GLUTIL_DEBUG_CALLBACK_HPP
#define GLUTIL_DEBUG_CALLBACK_HPP

#include <glutil/gl.hpp>

namespace glutil::debug {

void initDebugCallbacks();

extern GLenum debugCallbackSeverityThreshold;

} // namespace glutil

#endif // GLUTIL_DEBUG_CALLBACK_HPP