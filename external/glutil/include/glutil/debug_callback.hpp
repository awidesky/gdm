#ifndef GLUTIL_DEBUG_CALLBACK_HPP
#define GLUTIL_DEBUG_CALLBACK_HPP

#include <glutil/gl.hpp>

namespace glutil::debug {

#if GDM_DEBUG

void initDebugCallbacks();

/*
The debug callbacks are automatically enabled in initDebugCallbacks(), use this function to toggle the feature.
Passing false as the parameter will have same effect as calling initDebugCallbacks()
*/
void disableDebugCallbacks(bool disable = true);

void setDebugCallbackSeverityThreshold(GLenum debugCallbackSeverityThreshold);

#else
inline constexpr void initDebugCallbacks() noexcept {}
inline constexpr void disableDebugCallbacks(bool disable = true) noexcept { (void)disable; }
inline constexpr void setDebugCallbackSeverityThreshold(GLenum) noexcept {};
#endif
} // namespace glutil::debug

#endif // GLUTIL_DEBUG_CALLBACK_HPP