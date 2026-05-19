#include <glutil/debug.hpp>
#include <glutil/logging.hpp>

#include <sstream>
#include <string>
#include <string_view>
#include <stdarg.h>

namespace glutil::debug {
namespace callbacks {

#if defined(GDM_HAS_GLAD) && defined(GLAD_OPTION_GL_DEBUG)
static void noopPreCallback(const char* name, GLADapiproc apiproc, int len_args, ...) {
    (void)name; (void)apiproc; (void)len_args;
}
static void noopPostCallback(void* ret, const char* name, GLADapiproc apiproc, int len_args, ...) {
    (void)ret; (void)name; (void)apiproc; (void)len_args;
}
static void checkGLErrorOnlyPostCallback(void* ret, const char* name, GLADapiproc apiproc, int len_args, ...) {
    (void)ret; (void)apiproc; (void)len_args;
    const GLenum err = glad_glGetError();
    if (err != GL_NO_ERROR) {
        std::stringstream ss;
        ss << "[GL Error] " << glErrorToString(err) << '(' << err << ')' << " in function " << name;
        printStackTrace(ss.str());
        LOG_ERROR() << '\n';
    }
}

enum class GLFunctions {
    GenBuffers, CreateBuffers,
    GenVertexArrays, CreateVertexArrays,
    GenTextures, CreateTextures,
    GenFramebuffers, CreateFramebuffers,
    CreateShader, CreateProgram,
    DeleteBuffers, DeleteVertexArrays, DeleteTextures,
    DeleteFramebuffers, DeleteShader, DeleteProgram,
    BindBuffer, BindVertexArray, BufferData, DrawElements,

    Unknown
};
static GLFunctions classifyGLFunctions(std::string_view fname) {
    if (fname == "glGenBuffers") return GLFunctions::GenBuffers;
    if (fname == "glCreateBuffers") return GLFunctions::CreateBuffers;
    if (fname == "glGenVertexArrays") return GLFunctions::GenVertexArrays;
    if (fname == "glCreateVertexArrays") return GLFunctions::CreateVertexArrays;
    if (fname == "glGenTextures") return GLFunctions::GenTextures;
    if (fname == "glCreateTextures") return GLFunctions::CreateTextures;
    if (fname == "glGenFramebuffers") return GLFunctions::GenFramebuffers;
    if (fname == "glCreateFramebuffers") return GLFunctions::CreateFramebuffers;
    if (fname == "glCreateShader") return GLFunctions::CreateShader;
    if (fname == "glCreateProgram") return GLFunctions::CreateProgram;
    if (fname == "glDeleteBuffers") return GLFunctions::DeleteBuffers;
    if (fname == "glDeleteVertexArrays") return GLFunctions::DeleteVertexArrays;
    if (fname == "glDeleteTextures") return GLFunctions::DeleteTextures;
    if (fname == "glDeleteFramebuffers") return GLFunctions::DeleteFramebuffers;
    if (fname == "glDeleteShader") return GLFunctions::DeleteShader;
    if (fname == "glDeleteProgram") return GLFunctions::DeleteProgram;
    if (fname == "glBindBuffer") return GLFunctions::BindBuffer;
    if (fname == "glBindVertexArray") return GLFunctions::BindVertexArray;
    if (fname == "glBufferData") return GLFunctions::BufferData;
    if (fname == "glDrawElements") return GLFunctions::DrawElements;
    return GLFunctions::Unknown;
}
#pragma warning(push)
#pragma warning(disable : 6269)
static void trackGLFunctions(void* ret, const char* name, int len_args, va_list args) {
    auto& tracker = GLStateTracker::instance();

    GLFunctions func = classifyGLFunctions(name);
    switch (func) {
        // ── Create ──
        case GLFunctions::GenBuffers:
        case GLFunctions::CreateBuffers:
        case GLFunctions::GenVertexArrays:
        case GLFunctions::CreateVertexArrays:
        case GLFunctions::GenTextures:
        case GLFunctions::GenFramebuffers:
        case GLFunctions::CreateFramebuffers: {
            GLsizei count = va_arg(args, GLsizei);
            GLuint* ids = va_arg(args, GLuint*);

            std::string type;
            if (func == GLFunctions::GenVertexArrays || func == GLFunctions::CreateVertexArrays)
                type = "VAO";
            else if (func == GLFunctions::GenTextures)
                type = "Texture";
            else if (func == GLFunctions::GenFramebuffers || func == GLFunctions::CreateFramebuffers)
                type = "FBO";

            for (GLsizei i = 0; i < count; i++) {
                if (func == GLFunctions::GenBuffers)
                    tracker.buffers.create(ids[i]);
                else
                    tracker.objects.create(type, ids[i]);
            }
            break;
        }

        case GLFunctions::CreateShader: {
            GLuint id = *static_cast<GLuint*>(ret);
            tracker.objects.create("Shader", id);
            break;
        }

        case GLFunctions::CreateProgram: {
            GLuint id = *static_cast<GLuint*>(ret);
            tracker.objects.create("Program", id);
            break;
        }

        case GLFunctions::CreateTextures: {
            va_arg(args, GLenum); // target ignored
            GLsizei count = va_arg(args, GLsizei);
            GLuint* ids = va_arg(args, GLuint*);
            for (GLsizei i = 0; i < count; i++)
                tracker.objects.create("Texture", ids[i]);
            break;
        }

        // ── Delete ──
        case GLFunctions::DeleteBuffers: {
            GLsizei count = va_arg(args, GLsizei);
            const GLuint* ids = va_arg(args, const GLuint*);
            for (GLsizei i = 0; i < count; i++)
                tracker.buffers.destroy(ids[i]);
            break;
        }

        case GLFunctions::DeleteVertexArrays: {
            GLsizei count = va_arg(args, GLsizei);
            const GLuint* ids = va_arg(args, const GLuint*);
            for (GLsizei i = 0; i < count; i++)
                tracker.objects.destroy("VAO", ids[i]);
            break;
        }

        case GLFunctions::DeleteTextures: {
            GLsizei count = va_arg(args, GLsizei);
            const GLuint* ids = va_arg(args, const GLuint*);
            for (GLsizei i = 0; i < count; i++)
                tracker.objects.destroy("Texture", ids[i]);
            break;
        }

        case GLFunctions::DeleteFramebuffers: {
            GLsizei count = va_arg(args, GLsizei);
            const GLuint* ids = va_arg(args, const GLuint*);
            for (GLsizei i = 0; i < count; i++)
                tracker.objects.destroy("FBO", ids[i]);
            break;
        }

        case GLFunctions::DeleteShader: {
            GLuint id = va_arg(args, GLuint);
            tracker.objects.destroy("Shader", id);
            break;
        }

        case GLFunctions::DeleteProgram: {
            GLuint id = va_arg(args, GLuint);
            tracker.objects.destroy("Program", id);
            break;
        }

        // ── Bind ──
        case GLFunctions::BindBuffer: {
            GLenum target = va_arg(args, GLenum);
            GLuint id = va_arg(args, GLuint);
            if (target == GL_ARRAY_BUFFER)
                tracker.boundArrayBuffer = id;
            else if (target == GL_ELEMENT_ARRAY_BUFFER)
                tracker.boundElementArrayBuffer = id;
            else {
                LOG_WARNING() << "[glBindBuffer] Unknown buffer binding target : " << glBufferTypeToString(target);
                break;
            }
            if (auto* info = tracker.buffers.get(id))
                info->role = (target == GL_ARRAY_BUFFER) ? BufferRole::VBO : BufferRole::EBO;
            
            break;
        }

        case GLFunctions::BindVertexArray: {
            GLuint id = va_arg(args, GLuint);
            tracker.boundVAO = id;
            break;
        }

        // ── Buffer MetaData ──
        case GLFunctions::BufferData: {
            GLenum target = va_arg(args, GLenum);
            GLsizeiptr size = va_arg(args, GLsizeiptr);

            GLuint id = 0;
            if (target == GL_ARRAY_BUFFER)
                id = tracker.boundArrayBuffer;
            else if (target == GL_ELEMENT_ARRAY_BUFFER)
                id = tracker.boundElementArrayBuffer;
            else {
                LOG_WARNING() << "[glBufferData] Unknown buffer binding target : " << glBufferTypeToString(target);
                break;
            }
            if (auto* info = tracker.buffers.get(id)) {
                info->role = (target == GL_ARRAY_BUFFER) ? BufferRole::VBO : BufferRole::EBO;
                info->size = size;
            }
            break;
        }

        case GLFunctions::DrawElements: {
            va_arg(args, GLenum);  // mode ignore
            va_arg(args, GLsizei); // count ignore
            GLenum type = va_arg(args, GLenum);

            GLuint id = tracker.boundElementArrayBuffer;
            if (auto* info = tracker.buffers.get(id)) {
                info->dataType = type;
            }
            break;
        }

        case GLFunctions::Unknown:
        default: break;
    }
}
#pragma warning(pop)

static void checkGLErrorPostCallback(void* ret, const char* name, GLADapiproc apiproc, int len_args, ...) {
    (void)ret; (void)apiproc; (void)len_args;

    gladSetGLPostCallback(checkGLErrorOnlyPostCallback);
    
    va_list args;
    va_start(args, len_args);
    trackGLFunctions(ret, name, len_args, args);
    va_end(args);


    const GLenum err = glad_glGetError();
    if (err != GL_NO_ERROR) {
        LOG_ERROR() << "[GL Error] " << glErrorToString(err) << '(' << err << ')';
        printStackTrace(std::string("In function ") + name);
        snapshot();
        LOG_ERROR() << "---- End of \"" << glErrorToString(err) << '(' << err << ')' << " in function " << name << "\"\n\n";
    }

    gladSetGLPostCallback(checkGLErrorPostCallback);
}
#endif
} // namespace callbacks

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


static void initGladCallbacks(bool openglDebugExtension) {
#if defined(GDM_HAS_GLAD) && defined(GLAD_OPTION_GL_DEBUG)
    LOG_INFO() << "Using GLAD post callback for OpenGL error checking."; //TODO : remove
    gladSetGLPreCallback(callbacks::noopPreCallback);
    gladSetGLPostCallback(callbacks::checkGLErrorPostCallback);
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

    if (funcptr != nullptr && hasCapability) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(debugMessageCallback, nullptr);
        LOG_INFO() << "OpenGL glDebugMessageCallback enabled.";
        return true;
    } else {
        LOG_WARNING() << "OpenGL debug output is available at compile time, but not supported by the current context!";
        LOG_WARNING() << "glDebugMessageCallback=" << funcptr << ", "
        #if defined(GDM_HAS_GLAD) && !defined(GDM_HAS_GLEW_GLAD)
            << "GLAD_GL_VERSION_4_3=" << GLAD_GL_VERSION_4_3 << ", GLAD_GL_KHR_debug=" 
            #if defined(GL_KHR_debug)
              << GLAD_GL_KHR_debug
            #else
              << "undefined"
            #endif
        #elif defined(GDM_HAS_GLAD)
            << "GLAD_GL_VERSION_4_3=" << GLAD_GL_VERSION_4_3
        #elif defined(GDM_HAS_GLEW)
            << "GLEW_KHR_debug=" << GLEW_KHR_debug << ", GLEW_VERSION_4_3=" << GLEW_VERSION_4_3
        #endif
            << ", GL_EXTENSION \"GL_KHR_debug\"=" << glutil::debug::hasGLExtension("GL_KHR_debug");
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