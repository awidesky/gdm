#include <glutil/debug.hpp>
#include <glutil/logging.hpp>

#include <sstream>

namespace glutil::debug {

namespace {
static GLenum getGlError() {
#if defined(GDM_HAS_GLAD) && defined(GLAD_OPTION_GL_DEBUG)
    return glad_glGetError();
#else
    return glGetError();
#endif
}
} // namespace

std::string glErrorToString(GLenum err) {
    std::stringstream ss;
    switch (err) {
        case GL_NO_ERROR: ss << "GL_NO_ERROR"; break;
        case GL_INVALID_ENUM: ss << "GL_INVALID_ENUM"; break;
        case GL_INVALID_VALUE: ss << "GL_INVALID_VALUE"; break;
        case GL_INVALID_OPERATION: ss << "GL_INVALID_OPERATION"; break;
        case GL_OUT_OF_MEMORY: ss << "GL_OUT_OF_MEMORY"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION: ss << "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
        case GL_FRAMEBUFFER_COMPLETE: return "GL_FRAMEBUFFER_COMPLETE";
        case GL_FRAMEBUFFER_UNDEFINED: return "GL_FRAMEBUFFER_UNDEFINED";
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: return "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: return "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
        case GL_FRAMEBUFFER_UNSUPPORTED: return "GL_FRAMEBUFFER_UNSUPPORTED";
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: return "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
        default: ss << "UNKNOWN_ERROR";
    }
    ss << '(' << err << ')';
    return ss.str();
}

void dumpGLState() {
    const GLenum fbStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    LOG_ERROR() << "[OpenGL state dump]";
    LOG_ERROR() << "  Framebuffer status: " << glErrorToString(fbStatus);

    GLint bound = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &bound);
    if (bound != 0) {
        GLint linkStatus = 0;
        glGetProgramiv(bound, GL_LINK_STATUS, &linkStatus);
        GLint infoLogLength = 0;
        glGetProgramiv(bound, GL_INFO_LOG_LENGTH, &infoLogLength);
        LOG_ERROR() << "  Current shader program ID: " << bound
                    << ", LinkStatus: " << (linkStatus == GL_TRUE ? "OK" : "FAIL");
        std::string infoLog(infoLogLength, '\0');
        if (infoLogLength > 0) {
            glGetProgramInfoLog(bound, infoLogLength, nullptr, infoLog.data());
            LOG_ERROR() << "  Program InfoLog: " << infoLog;
        }
    } else {
        LOG_ERROR() << "  No shader program bound";
    }

    glGetIntegerv(GL_TEXTURE_BINDING_2D, &bound);
    if (bound != 0) {
        GLint width = 0, height = 0, internalFormat = 0;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalFormat);
        LOG_ERROR() << "  Current 2D texture bound (ID: " << bound << ") Size: " << width << "x" << height
                    << ", Format: " << internalFormat;
    } else {
        LOG_ERROR() << "  No 2D texture bound";
    }

    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &bound);
    LOG_ERROR() << "  Current VAO bound: " << bound;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &bound);
    LOG_ERROR() << "  Current VBO bound: " << bound;
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &bound);
    LOG_ERROR() << "  Current EBO bound: " << bound;

    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    LOG_ERROR() << "  Viewport: x=" << vp[0] << ", y=" << vp[1] << ", w=" << vp[2] << ", h=" << vp[3];
}

void checkGLError(const std::string& msg) {
    const GLenum err = getGlError();
    if (err != GL_NO_ERROR) {
        if (!msg.empty()) {
            LOG_ERROR() << msg;
        }
        LOG_ERROR() << "OpenGL Error: " << glErrorToString(err);
        dumpGLState();
    }
}

void init() {
    initDebugCallbacks();
}

} // namespace glutil