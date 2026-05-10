#include <glutil/debug_info.hpp>

#ifdef GDM_HAS_GLEW
#include <GL/glew.h>
#endif

#ifdef GDM_HAS_GLAD
#include <glad/gl.h>
#endif

#ifdef GDM_HAS_GLFW
#include <GLFW/glfw3.h>
#elif defined(GDM_HAS_FREEGLUT)
#include <GL/freeglut.h>
#endif

#ifdef GDM_HAS_GLM
#include <glm/glm.hpp>
#endif

#include <iostream>
#include <ostream>

namespace glutil::debug {

namespace {
static const char* profileMaskToString() {
#if defined(GL_VERSION_3_2)
    GLint profileMask;
    glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profileMask);

    if ((profileMask & GL_CONTEXT_CORE_PROFILE_BIT) != 0)
        return "core";
    if ((profileMask & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT) != 0)
        return "compatibility";
    
#endif
    return "Unknown";
}


static void printOpenGLLimits(GLVersion& ver, std::ostream& os) {
    os << "[OpenGL Capability Maximum Limits]\n";

#ifdef GL_VERSION_1_3
    if(ver >= "1.3") {
        GLint maxTextureSize = 0;
        GLint max3DTextureSize = 0;
        GLint maxCubeMapSize = 0;
        GLint viewportDims[2] = {0, 0};

        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
        glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &max3DTextureSize);
        glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &maxCubeMapSize);
        glGetIntegerv(GL_MAX_VIEWPORT_DIMS, viewportDims);

        os << "[OpenGL] Texture Size (2D/3D): " << maxTextureSize << " / " << max3DTextureSize << "\n";
        os << "[OpenGL] CubeMap Texture Size: " << maxCubeMapSize << "\n";
        os << "[OpenGL] Viewport Dimensions: " << viewportDims[0] << " x " << viewportDims[1] << "\n";
    }
#endif
#ifdef GL_VERSION_2_0
    if(ver >= "2.0") {
        GLint maxTexImageUnits = 0;
        GLint maxCombinedTexUnits = 0;
        GLint maxVertexAttribs = 0;
        GLint maxDrawBuffers = 0;

        glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxCombinedTexUnits);
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTexImageUnits);
        glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);

        os << "[OpenGL] Texture Units (Fragment shader/Combined): " << maxTexImageUnits << " / " << maxCombinedTexUnits << "\n";
        os << "[OpenGL] Vertex Attributes: " << maxVertexAttribs << "\n";
        os << "[OpenGL] Color Buffers in a Framebuffer: " << maxDrawBuffers << "\n";
    }
#endif
#ifdef GL_VERSION_3_2
    if(ver >= "3.2") {
        GLint maxSamples = 0;
        GLint maxRenderbufferSize = 0;
        GLint texBufferSize = 0;
        GLint uboBindings = 0;
        GLint uboBlockSize = 0;
        GLint uboAlign = 0;
        GLint colorSamples = 0;
        GLint depthSamples = 0;

        glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
        glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &maxRenderbufferSize);
        glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &texBufferSize);
        glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &uboBindings);
        glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &uboBlockSize);
        glGetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &colorSamples);
        glGetIntegerv(GL_MAX_DEPTH_TEXTURE_SAMPLES, &depthSamples);

        os << "[OpenGL] MSAA Samples: " << maxSamples << "\n";
        os << "[OpenGL] Renderbuffer Size: " << maxRenderbufferSize << ", Texture Buffer Size: " << texBufferSize << "\n";
        os << "[OpenGL] UBO Bindings: " << uboBindings << ", Block Size: " << uboBlockSize << "\n";
        os << "[OpenGL] Color Texture Samples: " << colorSamples << ", Depth Texture Samples: " << depthSamples << "\n";
    }
#endif
#ifdef GL_VERSION_4_3
    if(ver >= "4.3") {
        GLint attrBindings = 0;
        GLint relOffset = 0;
        GLint elementIndex = 0;
        GLint labelLength = 0;
        GLint debugStack = 0;

        glGetIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &attrBindings);
        glGetIntegerv(GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET, &relOffset);
        glGetIntegerv(GL_MAX_ELEMENT_INDEX, &elementIndex);
        glGetIntegerv(GL_MAX_LABEL_LENGTH, &labelLength);
        glGetIntegerv(GL_MAX_DEBUG_GROUP_STACK_DEPTH, &debugStack);

        os << "[OpenGL] Vertex Attribute Bindings: " << attrBindings << ", Offset: " << relOffset << ", Index: " << elementIndex << "\n";
        os << "[OpenGL] Object Label Length: " << labelLength << ", Debug Stack Depth: " << debugStack << "\n";
    }
#endif
}
} // namespace

void printRuntimeInfo(std::ostream& os, bool verbose) {
    os << "=== Runtime Dependency Info ===\n";
    GLVersion ver = getOpenGLVersion();

#ifdef GDM_HAS_GLFW
    int major = 0;
    int minor = 0;
    int rev = 0;
    glfwGetVersion(&major, &minor, &rev);
    os << "[GLFW] Version: " << major << "." << minor << "." << rev
       << ", String: " << glfwGetVersionString() << "\n";
#endif

#ifdef GDM_HAS_FREEGLUT
    const int glutVersion = glutGet(GLUT_VERSION);
    os << "[FreeGLUT] GLUT_VERSION(raw): " << glutVersion << "\n";
#endif

#ifdef GDM_HAS_GLEW
    const GLubyte* glewVer = glewGetString(GLEW_VERSION);
    os << "[GLEW] Version: " << (glewVer ? reinterpret_cast<const char*>(glewVer) : "(null)") << "\n";
#endif

#ifdef GDM_HAS_GLAD
    os << "[GLAD] Generator version: " << GLAD_GENERATOR_VERSION << ", Debug option "
#ifdef GLAD_OPTION_GL_DEBUG
       << "ON";
#else
       << "OFF";
#endif
    os << "\n";
#endif

#ifdef GDM_HAS_GLM
    os << "[GLM] Version: "
       << GLM_VERSION_MAJOR << "."
       << GLM_VERSION_MINOR << "."
       << GLM_VERSION_PATCH << "."
       << GLM_VERSION_REVISION << "\n";
#endif
    const char* version = (const char*)glGetString(GL_VERSION);
    const char* slVersion = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    const char* vendor = (const char*)glGetString(GL_VENDOR);
    const char* renderer = (const char*)glGetString(GL_RENDERER);

    os << "[OpenGL] Version: " << (version ? version : "(null)")
    << ", GLSL: " << (slVersion ? slVersion : "(null)") << "\n";
    os << "[OpenGL] Vendor: " << (vendor ? vendor : "(null)")
    << ", Renderer: " << (renderer ? renderer : "(null)") << "\n";

    GLint contextMajor = 0;
    GLint contextMinor = 0;
    GLint contextFlags = 0;
    GLint profileMask = 0;
    GLint extensionCount = 0;

#if defined(GL_VERSION_3_0)
if (ver >= "3.0") {
    glGetIntegerv(GL_MAJOR_VERSION, &contextMajor);
    glGetIntegerv(GL_MINOR_VERSION, &contextMinor);
    glGetIntegerv(GL_CONTEXT_FLAGS, &contextFlags);
    glGetIntegerv(GL_NUM_EXTENSIONS, &extensionCount);

    os << "[OpenGL] Context version ints: "
       << contextMajor << "." << contextMinor;
    if (ver >= "3.2") os << ", Profile: " << profileMaskToString();
    os << "\n" << "[OpenGL] Extension Count: " << extensionCount << "\n";

    #ifdef GL_CONTEXT_FLAG_DEBUG_BIT
    os << "[OpenGL] Debug context: " << (((contextFlags & GL_CONTEXT_FLAG_DEBUG_BIT) != 0) ? "YES" : "NO") << "\n";
    #endif
}
#endif

    if (verbose) {
        os <<  "\n";
        printOpenGLLimits(ver, os);
    }
    os << "===============================\n";
}

void printRuntimeInfo(bool verbose) {
    printRuntimeInfo(std::cout, verbose);
}

GLVersion getOpenGLVersion() {
    const char* versionStr = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    if (versionStr == nullptr) return GLVersion{};
    else return parseGLVersion(versionStr);
}

} // glutil::debug