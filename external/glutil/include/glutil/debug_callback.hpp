#ifndef GLUTIL_DEBUG_CALLBACK_HPP
#define GLUTIL_DEBUG_CALLBACK_HPP

#include <glutil/gl.hpp>

namespace glutil::debug {

/** Debug callback modes */
enum GladDebugCallbacksMode {
    /** Register no-op pre/post callbacks. */
    noop,
    /** Register only error checking post callback. pre-callback is no-op */
    errorCheck,
    /**
     * Register full debug callbacks. 
     * Includes pre-callback for pre-inspection
     * and post-callback for error checking, GL tracking, Object auto-labeling,
     * and post-inspection.
     */
    full
};

#if GDM_DEBUG

/** Initialize OpenGL/GLAD debug callbacks and error tracking system. */
void initDebugCallbacks();

/**
 * Enable or disable GL debug callbacks and GLAD hooks
 * The debug callbacks are automatically enabled in initDebugCallbacks(), use this function to toggle the feature.
 * Passing false as the parameter will have same effect as calling initDebugCallbacks()
*/
void disableDebugCallbacks(bool disable = true);
/**
 * Set GLAD callback modes.
 * @see GladDebugCallbacksMode for available modes.
 */
GladDebugCallbacksMode setGladDebugCallbacks(GladDebugCallbacksMode mode);
/** 
 * Set minimum severity level for OpenGL debug message filtering.
 * e.g. If passed GL_DEBUG_SEVERITY_LOW, GL_DEBUG_SEVERITY_NOTIFICATION callback wouldn't print
 */
void setDebugCallbackSeverityThreshold(GLenum debugCallbackSeverityThreshold);

#else
/** No-op in non-debug build. */
inline constexpr void initDebugCallbacks() noexcept {}
/** No-op in non-debug build. */
inline constexpr void disableDebugCallbacks(bool disable = true) noexcept { (void)disable; }
/** No-op in non-debug build. */
inline constexpr GladDebugCallbacksMode setGladDebugCallbacks(GladDebugCallbacksMode mode) noexcept {
    (void)mode;
    return GladDebugCallbacksMode::noop;
}
/** No-op in non-debug build. */
inline constexpr void setDebugCallbackSeverityThreshold(GLenum) noexcept {};
#endif
} // namespace glutil::debug

#endif // GLUTIL_DEBUG_CALLBACK_HPP