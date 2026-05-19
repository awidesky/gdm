#ifndef GLUTIL_DEBUG_HPP
#define GLUTIL_DEBUG_HPP

#include <glutil/debug_callback.hpp>
#include <glutil/debug_stacktrace.hpp>
#include <glutil/debug_info.hpp>
#include <glutil/debug_snapshot.hpp>
#include <glutil/debug_tracker.hpp>
#include <glutil/glToString.hpp>

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

void init();

void checkGLError(const std::string& msg = {});

} // namespace glutil::debug

#endif // GLUTIL_DEBUG_HPP