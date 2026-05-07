#include <glutil/debug.hpp>
#include <glutil/logging.hpp>

#include <sstream>

namespace glutil::debug {
namespace {

#if defined(GDM_HAS_GLAD) && defined(GLAD_OPTION_GL_DEBUG)
static void checkGLErrorPostCallback(void* ret, const char* name, GLADapiproc apiproc, int len_args, ...) {
    (void)ret; (void)apiproc; (void)len_args;

    const GLenum err = glad_glGetError();
    if (err != GL_NO_ERROR) {
        std::stringstream ss;
        ss << "[GL Error] " << glErrorToString(err) << " in function " << name;
        printStackTrace(ss.str());
    }
}

static void noopPreCallback(const char* name, GLADapiproc apiproc, int len_args, ...) {
    (void)name; (void)apiproc; (void)len_args;
}
static void noopPostCallback(void* ret, const char* name, GLADapiproc apiproc, int len_args, ...) {
    (void)ret; (void)name; (void)apiproc; (void)len_args;
}
#endif

#if defined(GDM_HAS_GLEW) || defined(GDM_HAS_GLAD)
static void debugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                                 const GLchar* message, const void* userParam) {
    (void)length;
    (void)userParam;

    std::ostringstream ss;
    ss << "OpenGL debug message callback invoked!\n";
    ss << "---------------------gldebugCallback-start----------------\n";
    ss << "Message: " << message << '\n';
    ss << "ID: " << id << '\n';
    ss << "Source: ";
    switch (source) {
        case GL_DEBUG_SOURCE_API: ss << "API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM: ss << "WINDOW_SYSTEM"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: ss << "SHADER_COMPILER"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY: ss << "THIRD_PARTY"; break;
        case GL_DEBUG_SOURCE_APPLICATION: ss << "APPLICATION"; break;
        case GL_DEBUG_SOURCE_OTHER: ss << "OTHER"; break;
        default: ss << "unknown source (" << source << ')'; break;
    }
    ss << '\n';
    ss << "Type: ";
    switch (type) {
        case GL_DEBUG_TYPE_ERROR: ss << "ERROR"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: ss << "DEPRECATED_BEHAVIOR"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: ss << "UNDEFINED_BEHAVIOR"; break;
        case GL_DEBUG_TYPE_PORTABILITY: ss << "PORTABILITY"; break;
        case GL_DEBUG_TYPE_PERFORMANCE: ss << "PERFORMANCE"; break;
        case GL_DEBUG_TYPE_MARKER: ss << "MARKER"; break;
        case GL_DEBUG_TYPE_OTHER: ss << "OTHER"; break;
        default: ss << "unknown type (" << type << ')'; break;
    }
    ss << '\n';
    ss << "Severity: ";
    switch (severity) {
        case GL_DEBUG_SEVERITY_LOW: ss << "LOW"; break;
        case GL_DEBUG_SEVERITY_MEDIUM: ss << "MEDIUM"; break;
        case GL_DEBUG_SEVERITY_HIGH: ss << "HIGH"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: ss << "NOTIFICATION"; break;
        default: ss << "unknown severity (" << severity << ')'; break;
    }
    ss << '\n';
    ss << "---------------------gldebugCallback-end------------------";
    LOG_ERROR() << ss.str();
}
#endif

} // namespace

static void initGladCallbacks(bool openglDebugExtension) {
#if defined(GDM_HAS_GLAD) && defined(GLAD_OPTION_GL_DEBUG)
    LOG_INFO() << "Using GLAD post callback for OpenGL error checking."; //TODO : remove
    gladSetGLPreCallback(noopPreCallback);
    gladSetGLPostCallback(checkGLErrorPostCallback);
#else
    LOG_INFO() << "GLAD post callback support is not available in this build.";
#endif
}

static bool initOpenGLDebugExtension() {
#if defined(GL_VERSION_4_3) || defined(GL_KHR_debug)
    const void* funcptr = (void*)
        #if defined(GDM_HAS_GLAD)
          glad_glCreateVertexArrays;
        #elif defined(GDM_HAS_GLEW)
          __glewDebugMessageCallback;
        #else
          glDebugMessageCallback;
        #endif
    const bool hasCapability =
        #if defined(GDM_HAS_GLAD) && !defined(GDM_HAS_GLEW_GLAD)
          #if defined(GLAD_GL_KHR_debug)
            (GLAD_GL_KHR_debug || GLAD_GL_VERSION_4_3)
          #else
            GLAD_GL_VERSION_4_3
          #endif
        #elif defined(GDM_HAS_GLAD)
          GLAD_GL_VERSION_4_3
        #elif defined(GDM_HAS_GLEW)
          (GLEW_KHR_debug || GLEW_VERSION_4_3)
        #else
          false
        #endif
    ;

    if (funcptr != nullptr && hasCapability) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(debugMessageCallback, nullptr);
        return true;
    } else {
        LOG_WARNING() << "OpenGL debug output is available at compile time, but not supported by the current context!";
        LOG_WARNING() << "glDebugMessageCallback=" << funcptr << ", "
        #if defined(GDM_HAS_GLAD) && !defined(GDM_HAS_GLEW_GLAD)
            << "GLAD_GL_VERSION_4_3=" << GLAD_GL_VERSION_4_3 << ", GLAD_GL_KHR_debug=" 
            #if defined(GL_KHR_debug)
              << GLAD_GL_KHR_debug;
            #else
              << "undefined";
            #endif
        #elif defined(GDM_HAS_GLAD)
            << "GLAD_GL_VERSION_4_3=" << GLAD_GL_VERSION_4_3;
        #elif defined(GDM_HAS_GLEW)
            << "GLEW_KHR_debug=" << GLEW_KHR_debug << ", GLEW_VERSION_4_3=" << GLEW_VERSION_4_3;
        #endif
        return false;
    }
#endif //  defined(GL_VERSION_4_3) || defined(GL_KHR_debug)
    LOG_INFO() << "OpenGL debug output not available at compile time (GL_VERSION_4_3 or GL_KHR_debug not defined).";
    return false;
}

void initDebugCallbacks() {
    bool openglDebugExtension = initOpenGLDebugExtension();
    initGladCallbacks(openglDebugExtension);
}

} // namespace glutil