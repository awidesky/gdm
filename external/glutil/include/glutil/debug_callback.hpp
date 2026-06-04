#ifndef GLUTIL_DEBUG_CALLBACK_HPP
#define GLUTIL_DEBUG_CALLBACK_HPP

#include <glutil/gl.hpp>

#if GLUTIL_DEBUG

namespace glutil::debug {

void initDebugCallbacks();

void setDebugCallbackSeverityThreshold(GLenum debugCallbackSeverityThreshold);

} // namespace glutil

#else
namespace glutil::debug {
inline constexpr void initDebugCallbacks() noexcept {}
inline constexpr void setDebugCallbackSeverityThreshold(GLenum) noexcept {};
} // namespace glutil::debug
#endif

#endif // GLUTIL_DEBUG_CALLBACK_HPP