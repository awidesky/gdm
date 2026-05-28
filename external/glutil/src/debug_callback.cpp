#include <glutil/debug.hpp>
#include <glutil/logging.hpp>

#include <sstream>
#include <string>
#include <string_view>
#include <stdarg.h>
#include <vector>

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
    CreateShader, CompileShader, CreateProgram, LinkProgram,
    DeleteBuffers, DeleteVertexArrays, DeleteTextures,
    DeleteFramebuffers, DeleteShader, DeleteProgram,
    BindBuffer, BindVertexArray, BindTexture, 
    BufferData, DrawElements, TexImage2D, TexImage3D,
    VertexAttribPointer, VertexAttribIPointer, VertexAttribLPointer,

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
    if (fname == "glCompileShader") return GLFunctions::CompileShader;
    if (fname == "glCreateProgram") return GLFunctions::CreateProgram;
    if (fname == "glLinkProgram") return GLFunctions::LinkProgram;
    if (fname == "glDeleteBuffers") return GLFunctions::DeleteBuffers;
    if (fname == "glDeleteVertexArrays") return GLFunctions::DeleteVertexArrays;
    if (fname == "glDeleteTextures") return GLFunctions::DeleteTextures;
    if (fname == "glDeleteFramebuffers") return GLFunctions::DeleteFramebuffers;
    if (fname == "glDeleteShader") return GLFunctions::DeleteShader;
    if (fname == "glDeleteProgram") return GLFunctions::DeleteProgram;
    if (fname == "glBindBuffer") return GLFunctions::BindBuffer;
    if (fname == "glBindVertexArray") return GLFunctions::BindVertexArray;
    if (fname == "glBindTexture") return GLFunctions::BindTexture;
    if (fname == "glBufferData") return GLFunctions::BufferData;
    if (fname == "glTexImage2D") return GLFunctions::TexImage2D;
    if (fname == "glTexImage3D") return GLFunctions::TexImage3D;
    if (fname == "glDrawElements") return GLFunctions::DrawElements;
    if (fname == "glVertexAttribPointer") return GLFunctions::VertexAttribPointer;
    if (fname == "glVertexAttribIPointer") return GLFunctions::VertexAttribIPointer;
    if (fname == "glVertexAttribLPointer") return GLFunctions::VertexAttribLPointer;
    return GLFunctions::Unknown;
}
static GLenum normalizeTextureTarget(GLenum target) {
    switch (target) {
        // Cube faces are one texture
        case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z: return GL_TEXTURE_CUBE_MAP;
        default: return target;
    }
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

        case GLFunctions::BindTexture: {
            GLenum target = va_arg(args, GLenum);
            GLuint id = va_arg(args, GLuint);
            tracker.boundTextures[normalizeTextureTarget(target)] = id;
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

        case GLFunctions::VertexAttribPointer:
        case GLFunctions::VertexAttribIPointer:
        case GLFunctions::VertexAttribLPointer:
        {
            GLuint currentVao = tracker.boundVAO;
            GLuint currentVbo = tracker.boundArrayBuffer;
            if (currentVao != 0 && currentVbo != 0) {
                if (auto* vboInfo = tracker.buffers.get(currentVbo)) {
                    vboInfo->associatedVaos.insert(currentVao);
                }
            }
            break;
        }
        default: break;
    }
}

static void autoLabelGLObjects(void* ret, const char* name, int len_args, va_list args) {
    if (disableAutoLabelGLObjects) return;

    auto& tracker = GLStateTracker::instance();
    GLFunctions func = classifyGLFunctions(name);
    const std::string codeline = '(' + getCalledGLfunctionName() + ')';
    // debug label auto generate in glCreate/Gen
    switch (func) {
        case GLFunctions::CreateShader: {
            GLenum type = va_arg(args, GLenum);
            GLuint id = *static_cast<GLuint*>(ret);
            labelGLobject(GL_SHADER, id, std::string(glShaderTypeToShortString(type)) + '#' + std::to_string(id) + codeline);
            break;
        }
        case GLFunctions::CreateProgram: {
            GLuint id = *static_cast<GLuint*>(ret);
            labelGLobject(GL_PROGRAM, id, "Program#" + std::to_string(id) + codeline);
            break;
        }
        case GLFunctions::CompileShader: {
            GLuint shader = va_arg(args, GLuint);
            GLint compileStatus = GL_FALSE;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);

            std::string label = getGLobjectLabel(GL_SHADER, shader);
            if (label.empty()) {
                GLint shaderType = 0;
                glGetShaderiv(shader, GL_SHADER_TYPE, &shaderType);
                label = std::string(glShaderTypeToShortString(shaderType)) + '#' + std::to_string(shader) + codeline;
            }
            label += std::string("[Compile: ") + (compileStatus == GL_TRUE ? "OK" : "FAILED") + ']';
            labelGLobject(GL_SHADER, shader, label);
            break;
        }
        case GLFunctions::LinkProgram: {
            GLuint program = va_arg(args, GLuint);
            GLint attachedCount = 0;
            glGetProgramiv(program, GL_ATTACHED_SHADERS, &attachedCount);

            std::ostringstream ss;
            ss << "Program#" << program << "(";
            if (attachedCount > 0) {
                std::vector<GLuint> attachedShaders(static_cast<size_t>(attachedCount));
                GLsizei shaderCount = 0;
                glGetAttachedShaders(program, attachedCount, &shaderCount, attachedShaders.data());

                std::vector<std::string> shaderLabels;
                shaderLabels.reserve(static_cast<size_t>(shaderCount));
                for (GLsizei i = 0; i < shaderCount; ++i) {
                    const GLuint shader = attachedShaders[static_cast<size_t>(i)];
                    GLint shaderType = 0;
                    glGetShaderiv(shader, GL_SHADER_TYPE, &shaderType);

                    std::string shaderLabel = getGLobjectLabel(GL_SHADER, shader);
                    if (shaderLabel.empty()) {
                        shaderLabel = std::string(glShaderTypeToShortString(static_cast<GLenum>(shaderType))) + '#' + std::to_string(shader);
                    }
                    shaderLabels.push_back(std::move(shaderLabel));
                }

                for (size_t i = 0; i < shaderLabels.size(); ++i) {
                    if (i != 0) ss << ", ";
                    ss << shaderLabels[i];
                }
            } else { ss << "attached=none"; }

            GLint linkStatus = GL_FALSE;
            glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
            ss << ")[Link: " << (linkStatus == GL_TRUE ? "OK" : "FAILED") << ']';
            labelGLobject(GL_PROGRAM, program, ss.str());
            break;
        }
        case GLFunctions::GenVertexArrays:
        case GLFunctions::GenBuffers:
        case GLFunctions::GenTextures: {
            GLsizei count = va_arg(args, GLsizei);
            GLuint* ids = va_arg(args, GLuint*);
            GLenum identifier = GL_NONE;
            std::string shortType = "Obj";
            if (func == GLFunctions::GenVertexArrays) {
                identifier = GL_VERTEX_ARRAY;
                shortType = "VAO";
            } else if (func == GLFunctions::GenBuffers) {
                identifier = GL_BUFFER;
                shortType = "Buffer";
            } else if (func == GLFunctions::GenTextures) {
                identifier = GL_TEXTURE;
                shortType = "Tex";
            }

            for (GLsizei i = 0; i < count; i++)
                labelGLobject(identifier, ids[i], shortType + '#' + std::to_string(ids[i]) + codeline);
            break;
        }

        /** Some implementation does not create object before use, so we track glBind* too */
        case GLFunctions::BindBuffer: {
            GLenum target = va_arg(args, GLenum);
            GLuint id = va_arg(args, GLuint);
            const std::string prevLabel = getGLobjectLabel(GL_BUFFER, id);
            if (!prevLabel.empty() && prevLabel.rfind("Buffer#", 0) != 0) 
                break;
            labelGLobject(GL_BUFFER, id, std::string(glBufferTypeToShortString(target)) + '#' + std::to_string(id) + codeline);
            break;
        }
        case GLFunctions::BindVertexArray: {
            GLuint id = va_arg(args, GLuint);
            const std::string prevLabel = getGLobjectLabel(GL_VERTEX_ARRAY, id);
            if (!prevLabel.empty())
                break;
            labelGLobject(GL_VERTEX_ARRAY, id, "VAO#" + std::to_string(id) + codeline);
            break;
        }
        case GLFunctions::BindTexture: {
            GLenum target = va_arg(args, GLenum);
            GLuint id = va_arg(args, GLuint);
            const std::string prevLabel = getGLobjectLabel(GL_TEXTURE, id);
            if (!prevLabel.empty() && prevLabel.rfind("Tex#", 0) != 0)
                break;
            labelGLobject(GL_TEXTURE, id, std::string(glTextureTargetToShortString(target)) + '#' + std::to_string(id) + codeline);
            break;
        }

        case GLFunctions::TexImage2D: {
            GLenum target = va_arg(args, GLenum);
            va_arg(args, GLint);
            GLint internalFormat = va_arg(args, GLint);
            GLsizei width = va_arg(args, int);
            GLsizei height = va_arg(args, int);
            GLuint id = tracker.boundTextures[normalizeTextureTarget(target)];
            std::stringstream ss;
            ss << glTextureTargetToShortString(target) << '#' << std::to_string(id) << '[' << std::to_string(width)
               << 'x' << std::to_string(height) << ' ' << glTextureInternalFormatToString(internalFormat) << ']';
            labelGLobject(GL_TEXTURE, id, ss.str());
            break;
        }
        case GLFunctions::TexImage3D: {
            GLenum target = va_arg(args, GLenum);
            va_arg(args, GLint); // level
            GLint internalFormat = va_arg(args, GLint);
            GLsizei width = va_arg(args, int);
            GLsizei height = va_arg(args, int);
            GLsizei depth = va_arg(args, int);
            GLuint id = tracker.boundTextures[normalizeTextureTarget(target)];
            std::stringstream ss;
            ss << glTextureTargetToShortString(target) << '#' << std::to_string(id) << '[' << std::to_string(width)
               << 'x' << std::to_string(height) << 'x' << std::to_string(depth) << ' '
               << glTextureInternalFormatToString(internalFormat) << ']';
            labelGLobject(GL_TEXTURE, id,ss.str());
            break;
        }

        default: break;
    }    
}
#pragma warning(pop)

static void checkGLErrorPostCallback(void* ret, const char* name, GLADapiproc apiproc, int len_args, ...) {
    (void)ret; (void)apiproc; (void)len_args;

    gladSetGLPostCallback(checkGLErrorOnlyPostCallback);
    
    va_list args;
    va_start(args, len_args);
    va_list args_copy;
    va_copy(args_copy, args);
    trackGLFunctions(ret, name, len_args, args);
    autoLabelGLObjects(ret, name, len_args, args_copy);
    va_end(args_copy);
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
    const GL_KHR_DebugSupport support = isGL_KHR_debugSupported();
    if (!support.compiledIn) {
            LOG_INFO() << "OpenGL debug output not available at compile time (GL_VERSION_4_3 or GL_KHR_debug not defined).";
            return false;
    }
// this guard is needed in case the glDebugMessageCallback does not exist.
#if defined(GL_VERSION_4_3) || defined(GL_KHR_debug)
    if (support) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(debugMessageCallback, nullptr);
        LOG_INFO() << "OpenGL glDebugMessageCallback enabled.";
        return true;
    } else {
        LOG_WARNING() << "OpenGL debug output is available at compile time, but not supported by the current context!";
        LOG_WARNING() << "glDebugMessageCallback=" << support.glDebugMessageCallbackPtr << ", "
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
#endif
}

void initDebugCallbacks() {
    bool openglDebugExtension = initOpenGLDebugExtension();
    initGladCallbacks(openglDebugExtension);
}

} // namespace glutil