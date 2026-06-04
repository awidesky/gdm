#ifndef GLUTIL_DEBUG_CALLBACK_HPP
#define GLUTIL_DEBUG_CALLBACK_HPP

#include <glutil/gl.hpp>

namespace glutil::debug {

#if GDM_DEBUG

void initDebugCallbacks();

void setDebugCallbackSeverityThreshold(GLenum debugCallbackSeverityThreshold);

#else
inline constexpr void initDebugCallbacks() noexcept {}
inline constexpr void setDebugCallbackSeverityThreshold(GLenum) noexcept {};
#endif
} // namespace glutil::debug

#endif // GLUTIL_DEBUG_CALLBACK_HPP