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

GL_KHR_DebugSupport isGL_KHR_debugSupported() {
#if defined(GL_VERSION_4_3) || defined(GL_KHR_debug)
        const void* funcptr = (void*)
                #if defined(GDM_HAS_GLAD)
                    glad_glDebugMessageCallback;
                #elif defined(GDM_HAS_GLEW)
                    __glewDebugMessageCallback;
                #else
                    glDebugMessageCallback;
                #endif
        const bool hasCapability = (
                #if defined(GDM_HAS_GLEW_GLAD)
                    (GLEW_KHR_debug || GLEW_VERSION_4_3 || GLAD_GL_KHR_debug || GLAD_GL_VERSION_4_3)
                #elif defined(GDM_HAS_GLAD)
                    #if defined(GL_KHR_debug)
                        GLAD_GL_KHR_debug ||
                    #endif
                        GLAD_GL_VERSION_4_3
                #elif defined(GDM_HAS_GLEW)
                    (GLEW_KHR_debug || GLEW_VERSION_4_3)
                #else
                    false
                #endif
                ) || glutil::debug::hasGLExtension("GL_KHR_debug");

        return GL_KHR_DebugSupport{funcptr, hasCapability, true};
#else
        return GL_KHR_DebugSupport{nullptr, false, false};
#endif
}

void labelGLobject(GLenum identifier, GLuint name, const std::string& label) {
    const GL_KHR_DebugSupport support = isGL_KHR_debugSupported();
    if (!support || identifier == 0)
        return;

#if defined(GL_VERSION_4_3) || defined(GL_KHR_debug)
    glObjectLabel(identifier, name, static_cast<GLsizei>(label.size()), label.c_str());
#endif
}

std::string getGLobjectLable(GLenum identifier, GLuint name) {
    const GL_KHR_DebugSupport support = isGL_KHR_debugSupported();
    if (!support) {
        return {};
    }

#if defined(GL_VERSION_4_3) || defined(GL_KHR_debug)
    GLsizei length = 0;
    glGetObjectLabel(identifier, name, 0, &length, nullptr);
    if (length <= 0) return {};

    length++;
    std::string label(static_cast<size_t>(length), '\0');
    glGetObjectLabel(identifier, name, length, &length, label.data());
    if (length < static_cast<GLsizei>(label.size())) {
        label.resize(static_cast<size_t>(length));
    }
    return label;
#else
    return {};
#endif
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