#ifndef GLUTIL_DEBUG_HPP
#define GLUTIL_DEBUG_HPP

#include <glutil/debug_callback.hpp>
#include <glutil/debug_info.hpp>
#include <glutil/debug_snapshot.hpp>
#include <glutil/debug_stacktrace.hpp>
#include <glutil/debug_tracker.hpp>
#include <glutil/gl.hpp>
#include <glutil/glToString.hpp>

#include <string>

namespace glutil::debug {

struct GL_KHR_DebugSupport {
    const void* glDebugMessageCallbackPtr; // pointer to glDebugMessageCallback function (runtime lookup result).
    const bool isLoaded; // true if debug extension symbols are loaded at runtime.
    const bool compiledIn; // true if build is compiled with OpenGL debug support enabled.

	explicit operator bool() const {
		return compiledIn && glDebugMessageCallbackPtr != nullptr && isLoaded;
	}
};

#if GDM_DEBUG

/** Check runtime + compile-time availability of GL_KHR_debug / GL 4.3 debug output. */
GL_KHR_DebugSupport isGL_KHR_debugSupported();

/** Global toggle to disable automatic GL object labeling. */
extern bool disableAutoLabelGLObjects;
/** Global toggle to disable automatic inspection hooks. */
extern bool disableAutoInspcector;
// Labels a GL object, only if KHR_debug is supported.
bool labelGLobject(GLenum identifier, GLuint name, const std::string& label);
// Returns the GL object label only if KHR_debug is supported.
std::string getGLobjectLabel(GLenum identifier, GLuint name);

/** Initialize debug system for current OpenGL context (callbacks, tracking, etc.). */
void init(bool printInfo = true);

#else
// always returns null in release build.
inline constexpr GL_KHR_DebugSupport isGL_KHR_debugSupported() noexcept { return {nullptr, false, false}; }
// always false in release build. Even if the value is changed, It won't be used.
inline bool disableAutoLabelGLObjects = true, disableAutoInspcector = true;
// always returns false in release build.
inline constexpr bool labelGLobject(GLenum, GLuint, const std::string&) noexcept { return false; }
// always returns empty string in release build.
inline std::string getGLobjectLabel(GLenum, GLuint) noexcept { return ""; };
// does nothing in release build.
inline constexpr void init(bool) noexcept {};
#endif // GDM_DEBUG

} // namespace glutil::debug

#endif // GLUTIL_DEBUG_HPP