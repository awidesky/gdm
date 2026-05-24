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

GL_KHR_DebugSupport isGL_KHR_debugSupported();

// Labels a GL object, only if KHR_debug is supported.
void labelGLobject(GLenum identifier, GLuint name, const std::string& label);
// Returns the GL object label only if KHR_debug is supported.
std::string getGLobjectLable(GLenum identifier, GLuint name);

void init();

void checkGLError(const std::string& msg = {});

} // namespace glutil::debug

#endif // GLUTIL_DEBUG_HPP