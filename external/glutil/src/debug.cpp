#include <glutil/debug.hpp>
#include <glutil/logging.hpp>
#include <glutil/glToString.hpp>

#include <cstring>
#include <sstream>
#include <string_view>

namespace glutil::debug {

namespace {
static GLenum getGlError() {
#if defined(GDM_HAS_GLAD) && defined(GLAD_OPTION_GL_DEBUG)
    return glad_glGetError();
#else
    return glGetError();
#endif
}

static std::set<std::string> loadGLExtensions() {
    std::set<std::string> extensions;
    GLint count = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &count);

    if (count > 0) {
        for (GLint i = 0; i < count; ++i) {
            const GLubyte* ext = glGetStringi(GL_EXTENSIONS, static_cast<GLuint>(i));
            if (ext != nullptr && *ext != '\0') {
                extensions.emplace(reinterpret_cast<const char*>(ext));
            }
        }
    }
    return extensions;
}
} // namespace


const std::set<std::string>& getGLExtensions() {
    static const std::set<std::string> extensions = loadGLExtensions();
    return extensions;
}

bool hasGLExtension(const char* extName) {
    if (extName == nullptr || *extName == '\0') {
        return false;
    }

    const auto& extensions = getGLExtensions();
    return extensions.find(extName) != extensions.end();
}


void checkGLError(const std::string& msg) {
    const GLenum err = getGlError();
    if (err != GL_NO_ERROR) {
        if (!msg.empty()) {
            LOG_ERROR() << msg;
        }
        LOG_ERROR() << "OpenGL Error: " << glErrorToString(err) << '(' << err << ')';
        printStackTrace();
    }
}

void init() {
    initDebugCallbacks();
}

} // namespace glutil::debug