#ifndef GLUTIL_DEBUG_HPP
#define GLUTIL_DEBUG_HPP

#include <glutil/debug_callback.hpp>
#include <glutil/debug_stacktrace.hpp>
#include <glutil/debug_info.hpp>
#include <glutil/debug_snapshot.hpp>
#include <glutil/debug_tracker.hpp>
#include <glutil/glToString.hpp>
#include <glutil/gl.hpp>

#include <string>

namespace glutil::debug {

struct GL_KHR_DebugSupport {
	const void* glDebugMessageCallbackPtr;
	const bool isLoaded;
	const bool compiledIn;

	explicit operator bool() const {
		return compiledIn && glDebugMessageCallbackPtr != nullptr && isLoaded;
	}
};

#if GDM_DEBUG

GL_KHR_DebugSupport isGL_KHR_debugSupported();

extern bool disableAutoLabelGLObjects;
// Labels a GL object, only if KHR_debug is supported.
bool labelGLobject(GLenum identifier, GLuint name, const std::string& label);
// Returns the GL object label only if KHR_debug is supported.
std::string getGLobjectLabel(GLenum identifier, GLuint name);

void init();

#else 
// always returns null in release build.
inline constexpr GL_KHR_DebugSupport isGL_KHR_debugSupported() noexcept { return {nullptr, false, false}; }
// always false in release build. Even if the value is changed, It won't be used.
inline bool disableAutoLabelGLObjects = true;
// always returns false in release build.
inline constexpr bool labelGLobject(GLenum, GLuint, const std::string&) noexcept { return false; }
// always returns empty string in release build.
inline std::string getGLobjectLabel(GLenum, GLuint) noexcept { return ""; };
// does nothing in release build.
inline constexpr void init() noexcept {};
#endif // GDM_DEBUG

} // namespace glutil::debug

#endif // GLUTIL_DEBUG_HPP