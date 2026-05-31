#include <glutil/debug.hpp>
#include <glutil/logging.hpp>
#include <glutil/glToString.hpp>

#include <cstring>
#include <sstream>
#include <string_view>

namespace glutil::debug {

bool disableAutoLabelGLObjects = false;

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

static bool isValidGLobject(GLenum identifier, GLuint name) {
    switch (identifier) {
        case GL_BUFFER: return glIsBuffer(name);
        case GL_TEXTURE: return glIsTexture(name);
        case GL_VERTEX_ARRAY: return glIsVertexArray(name);
        case GL_PROGRAM: return glIsProgram(name);
        case GL_SHADER: return glIsShader(name);
        case GL_FRAMEBUFFER: return glIsFramebuffer(name);
        case GL_RENDERBUFFER: return glIsRenderbuffer(name);
#if defined(GL_SAMPLER)
        case GL_SAMPLER: return glIsSampler(name);
#endif
#if defined(GL_QUERY)
        case GL_QUERY: return glIsQuery(name);
#endif
#if defined(GL_TRANSFORM_FEEDBACK)
        case GL_TRANSFORM_FEEDBACK: return glIsTransformFeedback(name);
#endif
        default:
        LOG_ERROR() << "Unknown identifier : " << identifier << ", object ID : " << name;
        return false;
    }
}

static GLint getMaxGLobjectLabelLength() {
    static GLint maxLabelLength = -1;
#ifdef GL_MAX_LABEL_LENGTH
    if (maxLabelLength < 0)
        glGetIntegerv(GL_MAX_LABEL_LENGTH, &maxLabelLength);
#endif
    return maxLabelLength;
}

static std::string makeSafeGLobjectLabel(const std::string& label) {
    const GLint maxLabelLength = getMaxGLobjectLabelLength();
    if (maxLabelLength <= 0) return label;

    // Per KHR_debug, GL_MAX_LABEL_LENGTH includes the null terminator.
    // The maximum number of characters we may store in the label (content) is
    // therefore maxLabelLength - 1. Ensure we never pass a content length
    // larger than that to glObjectLabel.
    const GLint maxContent = maxLabelLength - 1;
    if (label.size() <= static_cast<size_t>(maxContent)) return label;

    if (maxContent <= 3)
        return label.substr(0, static_cast<size_t>(std::max<GLint>(0, maxContent)));

    // Reserve room for the ellipsis so the final content length does not exceed maxContent.
    return label.substr(0, static_cast<size_t>(maxContent - 3)) + "...";
}

static void syncTrackerLabel(GLenum identifier, GLuint name, const std::string& label) {
    auto& tracker = GLStateTracker::instance();
    switch (identifier) {
        case GL_BUFFER:
            if (auto* info = tracker.buffers.get(name))
                info->label = label;
            break;
        case GL_VERTEX_ARRAY:
            if (auto* info = tracker.objects.get("VAO", name))
                info->label = label;
            break;
        case GL_TEXTURE:
            if (auto* info = tracker.objects.get("Texture", name))
                info->label = label;
            break;
        case GL_SHADER:
            if (auto* info = tracker.objects.get("Shader", name))
                info->label = label;
            break;
        case GL_PROGRAM:
            if (auto* info = tracker.objects.get("Program", name))
                info->label = label;
            break;
        case GL_FRAMEBUFFER:
            if (auto* info = tracker.objects.get("FBO", name))
                info->label = label;
            break;
        default:
            break;
    }
}

bool labelGLobject(GLenum identifier, GLuint name, const std::string& label) {
    const GL_KHR_DebugSupport support = isGL_KHR_debugSupported();
    if (!support || identifier == 0 || name == 0 || !isValidGLobject(identifier, name))
        return false;

#if defined(GL_VERSION_4_3) || defined(GL_KHR_debug)
    const std::string safeLabel = makeSafeGLobjectLabel(label);
    glObjectLabel(identifier, name, static_cast<GLsizei>(safeLabel.size()), safeLabel.c_str());
    syncTrackerLabel(identifier, name, safeLabel);
    return true;
#else
    return false;
#endif
}

std::string getGLobjectLabel(GLenum identifier, GLuint name) {
    const GL_KHR_DebugSupport support = isGL_KHR_debugSupported();
    if (!support || identifier == 0 || name == 0 || !isValidGLobject(identifier, name))
        return {};

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
    syncTrackerLabel(identifier, name, label);
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