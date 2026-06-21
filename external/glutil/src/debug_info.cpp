#include <glutil/glutil.hpp>
#include <glutil/debug_info.hpp>

#ifdef GDM_HAS_GLM
#include <glm/glm.hpp>
#endif

// for using /sys/class/drm/card0/device/
#ifdef __linux__
#include <fstream>
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
        GLint maxVertexAttribs = 0;
        GLint maxVertexUniform = 0;
        GLint maxFragUniform = 0;
        GLint maxTexImageUnits = 0;
        GLint maxCombinedTexUnits = 0;
        GLint maxDrawBuffers = 0;

        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);
        glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &maxVertexUniform);
        glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &maxFragUniform);
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTexImageUnits);
        glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxCombinedTexUnits);
        glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);

        os << "[OpenGL] Vertex Attributes: " << maxVertexAttribs << "\n";
        os << "[OpenGL] Shader Uniform Components (Vertex/Fragment): " << maxVertexUniform << "/ " << maxFragUniform << "\n";
        os << "[OpenGL] Shader Texture Units (Fragment/Combined): " << maxTexImageUnits << " / " << maxCombinedTexUnits << "\n";
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
    GLint attrBindings = -1;
    GLint relOffset = -1;
    GLint64 maxElementIndex = -1;
    GLint labelLength = -1;
    GLint debugStack = -1;

#ifndef GL_MAX_VERTEX_ATTRIB_BINDINGS
    constexpr GLenum GL_MAX_VERTEX_ATTRIB_BINDINGS = 0x82DA;
#endif
#ifndef GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET
    constexpr GLenum GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET = 0x82D9;
#endif
#ifndef GL_MAX_ELEMENT_INDEX
    constexpr GLenum GL_MAX_ELEMENT_INDEX = 0x8D6B;
#endif
#ifndef GL_MAX_LABEL_LENGTH
    constexpr GLenum GL_MAX_LABEL_LENGTH = 0x82E8;
#endif
#ifndef GL_MAX_DEBUG_GROUP_STACK_DEPTH
    constexpr GLenum GL_MAX_DEBUG_GROUP_STACK_DEPTH = 0x826C;
#endif

    if (ver >= "4.3" || hasGLExtension("GL_ARB_vertex_attrib_binding")) {
        glGetIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &attrBindings);
        glGetIntegerv(GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET, &relOffset);
    }

    if (ver >= "4.3" || hasGLExtension("GL_ARB_ES3_compatibility")) {
        glGetInteger64v(GL_MAX_ELEMENT_INDEX, &maxElementIndex);
    }

    if (ver >= "4.3" || hasGLExtension("GL_KHR_debug")) {
        glGetIntegerv(GL_MAX_LABEL_LENGTH, &labelLength);
        glGetIntegerv(GL_MAX_DEBUG_GROUP_STACK_DEPTH, &debugStack);
    }

    if (attrBindings != -1)
        os << "[OpenGL] Vertex Attribute Binding Slots: " << attrBindings << ", Relative Offset Limit: " << relOffset
           << "\n";
    if (maxElementIndex != -1)
        os << "[OpenGL] Max EBO Element Index: " << maxElementIndex << "\n";
    if (labelLength != -1)
        os << "[OpenGL] Object Debug Label Length: " << labelLength << ", Debug Group Stack Depth: " << debugStack
           << "\n";
}

static void checkOpenGLDebugExtension(std::ostream& os) {
    const GL_KHR_DebugSupport support = isGL_KHR_debugSupported();
    os << "[OpenGL] Debug output API available at compile time : " << (support.compiledIn ? "YES" : "NO") << "\n";
    if (!support.compiledIn) return;

// this guard is needed in case the glDebugMessageCallback does not exist.
#if defined(GL_VERSION_4_3) || defined(GL_KHR_debug)
    os << "[OpenGL] Debug output supported by the current context: " << (support ? "YES" : "NO") << '\n';
    if (support) {
        const GLboolean debugOutputEnabled = glIsEnabled(GL_DEBUG_OUTPUT);
        const GLboolean debugOutputSyncEnabled = glIsEnabled(GL_DEBUG_OUTPUT_SYNCHRONOUS);

        os << "[OpenGL] GL_DEBUG_OUTPUT=" << (debugOutputEnabled ? "Enabled" : "Disabled")
           << ", GL_DEBUG_OUTPUT_SYNCHRONOUS=" << (debugOutputSyncEnabled ? "Enabled" : "Disabled") << '\n';
    } else {
        os << "[OpenGL] glDebugMessageCallback=" << support.glDebugMessageCallbackPtr << ", "
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
            << ", GL_EXTENSION \"GL_KHR_debug\"=" << glutil::debug::hasGLExtension("GL_KHR_debug") << '\n';
    }
#endif
}

static const char* maxDefinedGLVersionSymbol() {
#if defined(GL_VERSION_4_6)
    return "GL_VERSION_4_6";
#elif defined(GL_VERSION_4_5)
    return "GL_VERSION_4_5";
#elif defined(GL_VERSION_4_4)
    return "GL_VERSION_4_4";
#elif defined(GL_VERSION_4_3)
    return "GL_VERSION_4_3";
#elif defined(GL_VERSION_4_2)
    return "GL_VERSION_4_2";
#elif defined(GL_VERSION_4_1)
    return "GL_VERSION_4_1";
#elif defined(GL_VERSION_4_0)
    return "GL_VERSION_4_0";
#elif defined(GL_VERSION_3_3)
    return "GL_VERSION_3_3";
#elif defined(GL_VERSION_3_2)
    return "GL_VERSION_3_2";
#elif defined(GL_VERSION_3_1)
    return "GL_VERSION_3_1";
#elif defined(GL_VERSION_3_0)
    return "GL_VERSION_3_0";
#elif defined(GL_VERSION_2_1)
    return "GL_VERSION_2_1";
#elif defined(GL_VERSION_2_0)
    return "GL_VERSION_2_0";
#elif defined(GL_VERSION_1_5)
    return "GL_VERSION_1_5";
#elif defined(GL_VERSION_1_4)
    return "GL_VERSION_1_4";
#elif defined(GL_VERSION_1_3)
    return "GL_VERSION_1_3";
#elif defined(GL_VERSION_1_2)
    return "GL_VERSION_1_2";
#elif defined(GL_VERSION_1_1)
    return "GL_VERSION_1_1";
#elif defined(GL_VERSION_1_0)
    return "GL_VERSION_1_0";
#else
    return "GL_VERSION_UNKNOWN";
#endif
}

#if defined(GDM_HAS_GLEW)
static const char* maxLoadedGLVersion_glew() {
#if defined(GL_VERSION_4_6)
    if (GLEW_VERSION_4_6) return "GLEW_VERSION_4_6";
#endif
#if defined(GL_VERSION_4_5)
    if (GLEW_VERSION_4_5) return "GLEW_VERSION_4_5";
#endif
#if defined(GL_VERSION_4_4)
    if (GLEW_VERSION_4_4) return "GLEW_VERSION_4_4";
#endif
#if defined(GL_VERSION_4_3)
    if (GLEW_VERSION_4_3) return "GLEW_VERSION_4_3";
#endif
#if defined(GL_VERSION_4_2)
    if (GLEW_VERSION_4_2) return "GLEW_VERSION_4_2";
#endif
#if defined(GL_VERSION_4_1)
    if (GLEW_VERSION_4_1) return "GLEW_VERSION_4_1";
#endif
#if defined(GL_VERSION_4_0)
    if (GLEW_VERSION_4_0) return "GLEW_VERSION_4_0";
#endif
#if defined(GL_VERSION_3_3)
    if (GLEW_VERSION_3_3) return "GLEW_VERSION_3_3";
#endif
#if defined(GL_VERSION_3_2)
    if (GLEW_VERSION_3_2) return "GLEW_VERSION_3_2";
#endif
#if defined(GL_VERSION_3_1)
    if (GLEW_VERSION_3_1) return "GLEW_VERSION_3_1";
#endif
#if defined(GL_VERSION_3_0)
    if (GLEW_VERSION_3_0) return "GLEW_VERSION_3_0";
#endif
#if defined(GL_VERSION_2_1)
    if (GLEW_VERSION_2_1) return "GLEW_VERSION_2_1";
#endif
#if defined(GL_VERSION_2_0)
    if (GLEW_VERSION_2_0) return "GLEW_VERSION_2_0";
#endif
#if defined(GL_VERSION_1_5)
    if (GLEW_VERSION_1_5) return "GLEW_VERSION_1_5";
#endif
#if defined(GL_VERSION_1_4)
    if (GLEW_VERSION_1_4) return "GLEW_VERSION_1_4";
#endif
#if defined(GL_VERSION_1_3)
    if (GLEW_VERSION_1_3) return "GLEW_VERSION_1_3";
#endif
#if defined(GL_VERSION_1_2)
    if (GLEW_VERSION_1_2) return "GLEW_VERSION_1_2";
#endif
#if defined(GL_VERSION_1_1)
    if (GLEW_VERSION_1_1) return "GLEW_VERSION_1_1";
#endif
#if defined(GL_VERSION_1_0)
    if (GLEW_VERSION_1_0) return "GLEW_VERSION_1_0";
#endif
    return "UNKNOWN";
}
#endif
#if defined(GDM_HAS_GLAD)
static const char* maxLoadedGLVersion_glad() {
#if defined(GL_VERSION_4_6)
    if (GLAD_GL_VERSION_4_6) return "GLAD_GL_VERSION_4_6";
#endif
#if defined(GL_VERSION_4_5)
    if (GLAD_GL_VERSION_4_5) return "GLAD_GL_VERSION_4_5";
#endif
#if defined(GL_VERSION_4_4)
    if (GLAD_GL_VERSION_4_4) return "GLAD_GL_VERSION_4_4";
#endif
#if defined(GL_VERSION_4_3)
    if (GLAD_GL_VERSION_4_3) return "GLAD_GL_VERSION_4_3";
#endif
#if defined(GL_VERSION_4_2)
    if (GLAD_GL_VERSION_4_2) return "GLAD_GL_VERSION_4_2";
#endif
#if defined(GL_VERSION_4_1)
    if (GLAD_GL_VERSION_4_1) return "GLAD_GL_VERSION_4_1";
#endif
#if defined(GL_VERSION_4_0)
    if (GLAD_GL_VERSION_4_0) return "GLAD_GL_VERSION_4_0";
#endif
#if defined(GL_VERSION_3_3)
    if (GLAD_GL_VERSION_3_3) return "GLAD_GL_VERSION_3_3";
#endif
#if defined(GL_VERSION_3_2)
    if (GLAD_GL_VERSION_3_2) return "GLAD_GL_VERSION_3_2";
#endif
#if defined(GL_VERSION_3_1)
    if (GLAD_GL_VERSION_3_1) return "GLAD_GL_VERSION_3_1";
#endif
#if defined(GL_VERSION_3_0)
    if (GLAD_GL_VERSION_3_0) return "GLAD_GL_VERSION_3_0";
#endif
#if defined(GL_VERSION_2_1)
    if (GLAD_GL_VERSION_2_1) return "GLAD_GL_VERSION_2_1";
#endif
#if defined(GL_VERSION_2_0)
    if (GLAD_GL_VERSION_2_0) return "GLAD_GL_VERSION_2_0";
#endif
#if defined(GL_VERSION_1_5)
    if (GLAD_GL_VERSION_1_5) return "GLAD_GL_VERSION_1_5";
#endif
#if defined(GL_VERSION_1_4)
    if (GLAD_GL_VERSION_1_4) return "GLAD_GL_VERSION_1_4";
#endif
#if defined(GL_VERSION_1_3)
    if (GLAD_GL_VERSION_1_3) return "GLAD_GL_VERSION_1_3";
#endif
#if defined(GL_VERSION_1_2)
    if (GLAD_GL_VERSION_1_2) return "GLAD_GL_VERSION_1_2";
#endif
#if defined(GL_VERSION_1_1)
    if (GLAD_GL_VERSION_1_1) return "GLAD_GL_VERSION_1_1";
#endif
#if defined(GL_VERSION_1_0)
    if (GLAD_GL_VERSION_1_0) return "GLAD_GL_VERSION_1_0";
#endif
    return "UNKNOWN";
}
#endif
} // namespace

void printRuntimeInfo(bool verbose, std::ostream& os) {
    os << "=== Runtime Dependency Info ===\n";
    GLVersion ver = getOpenGLVersion();

    os << "[GLUTIL] Version: " << GLUTIL_VERSION_MAJOR << "." << GLUTIL_VERSION_MINOR << "." << GLUTIL_VERSION_REVISION
       << "\n";

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
    os << "\n[GLEW] Defined GL Version: " << maxDefinedGLVersionSymbol();
    os << ", Loaded GL Version: " << maxLoadedGLVersion_glew() << '\n';
#endif

#ifdef GDM_HAS_GLAD
    os << "[GLAD] Generator version: " << GLAD_GENERATOR_VERSION << ", Debug option "
#ifdef GLAD_OPTION_GL_DEBUG
       << "ON";
#else
       << "OFF";
#endif
    os << "\n[GLAD] Defined GL Version: " << maxDefinedGLVersionSymbol();
    os << ", Loaded GL Version: " << maxLoadedGLVersion_glad() << '\n';
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
    os << "[OpenGL] Created as Debug context: " << (((contextFlags & GL_CONTEXT_FLAG_DEBUG_BIT) != 0) ? "YES" : "NO") << "\n";
    #endif
}
#endif

    checkOpenGLDebugExtension(os);

    if (verbose) {
        os <<  "\n";
        printOpenGLLimits(ver, os);
    }
    os << "===============================\n";
}

GLVersion getOpenGLVersion() {
    const char* versionStr = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    if (versionStr == nullptr) return GLVersion{};
    else return parseGLVersion(versionStr);
}

GLVersion availableGLversion() {
    constexpr GLVersion versions[] = {{4, 6}, {4, 5}, {4, 4}, {4, 3}, {4, 2}, {4, 1}, {4, 0}, {3, 3}, {3, 2},
                                      {3, 1}, {3, 0}, {2, 1}, {2, 0}, {1, 5}, {1, 4}, {1, 3}, {1, 2}, {1, 1}};

    for (auto version : versions) {
#if defined(GDM_HAS_GLFW)
        glfwDefaultWindowHints();
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, version.major);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, version.minor);

        // Core profile is supported since OpenGL 3.2
        if (version >= "3.2")
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
        GLFWwindow* window = glfwCreateWindow(1, 1, "", nullptr, nullptr);
        if (window) {
            glfwDestroyWindow(window);
            glfwDefaultWindowHints();
            return version;
        } else {
            const char* description = nullptr;
            int code = glfwGetError(&description);
            if(code != GLFW_VERSION_UNAVAILABLE)
                std::cerr << "[glutil::availableGLversion] GLFW Error(" << code << "): " << description << std::endl;
        }
#elif defined(GDM_HAS_FREEGLUT)
        glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);
        glutInitContextVersion(version.major, version.minor);
        // Core profile is supported since OpenGL 3.2
        glutInitContextProfile((version >= "3.2") ? GLUT_CORE_PROFILE : GLUT_COMPATIBILITY_PROFILE);

#ifdef __APPLE__
        glutInitContextFlags(GLUT_FORWARD_COMPATIBLE);
#else
        glutInitContextFlags(0);
#endif
        int window = glutCreateWindow("");
        if (window > 0) {
            glutDestroyWindow(window);
            return version;
        }
#endif
    }

    LOG_ERROR() << "No available OpenGL version is found from glfw!";
    return {};
}


#ifndef GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX
#define GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX 0x9047
#define GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX 0x9048
#define GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX 0x9049
#define GL_GPU_MEMORY_INFO_EVICTION_COUNT_NVX 0x904A
#define GL_GPU_MEMORY_INFO_EVICTED_MEMORY_NVX 0x904B
#endif

#ifndef GL_VBO_FREE_MEMORY_ATI
#define GL_VBO_FREE_MEMORY_ATI 0x87FB
#define GL_TEXTURE_FREE_MEMORY_ATI 0x87FC
#define GL_RENDERBUFFER_FREE_MEMORY_ATI 0x87FD
#endif

bool printGpuMemoryInfo(std::ostream& os) {
    bool printed = false;
    os << "Vendor : " << glGetString(GL_VENDOR) << ", Renderer : " << glGetString(GL_RENDERER) << '\n';

    // NVIDIA
    if (hasGLExtension("GL_NVX_gpu_memory_info")) {
        printed = true;
        GLint v;
        glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &v);
        os << "NV Dedicated VRAM  : " << v / 1024 << " MB\n"; // total VRAM
        glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &v);
        os << "NV Total Available : " << v / 1024 << " MB\n"; // available total vram
        glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &v);
        os << "NV Current Free    : " << v / 1024 << " MB\n";
        glGetIntegerv(GL_GPU_MEMORY_INFO_EVICTION_COUNT_NVX, &v);
        os << "NV Evictions Count : " << v << '\n';
        glGetIntegerv(GL_GPU_MEMORY_INFO_EVICTED_MEMORY_NVX, &v);
        os << "NV Evicted Memory  : " << v / 1024 << " MB\n";
    } else
        os << "(GL_NVX_gpu_memory_info not available)\n";

    // AMD
    if (hasGLExtension("GL_ATI_meminfo")) {
        printed = true;
        GLint tex[4], vbo[4], rb[4];

        glGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, tex);
        glGetIntegerv(GL_VBO_FREE_MEMORY_ATI, vbo);
        glGetIntegerv(GL_RENDERBUFFER_FREE_MEMORY_ATI, rb);

        os << "ATI Texture Local Free  : " << tex[0] / 1024 << " MB\n"
           << "ATI Texture Local Block : " << tex[1] / 1024 << " MB\n"
           << "ATI Texture Aux Free    : " << tex[2] / 1024 << " MB\n"
           << "ATI Texture Aux Block   : " << tex[3] / 1024 << " MB\n"

           << "ATI VBO Local Free      : " << vbo[0] / 1024 << " MB\n"
           << "ATI VBO Local Block     : " << vbo[1] / 1024 << " MB\n"
           << "ATI VBO Aux Free        : " << vbo[2] / 1024 << " MB\n"
           << "ATI VBO Aux Block       : " << vbo[3] / 1024 << " MB\n"

           << "ATI RB Local Free       : " << rb[0] / 1024 << " MB\n"
           << "ATI RB Local Block      : " << rb[1] / 1024 << " MB\n"
           << "ATI RB Aux Free         : " << rb[2] / 1024 << " MB\n"
           << "ATI RB Aux Block        : " << rb[3] / 1024 << " MB\n";
    } else
        os << "(GL_ATI_meminfo not available)\n";

#ifdef __linux__
    auto printFile = [&os](const char* name, const char* path) {
        std::ifstream f(path);
        uint64_t value;
        if (f >> value)
            os << name << " : " << value << '\n';
    };
    printed = true;
    printFile("GPU Busy (%)", "/sys/class/drm/card0/device/gpu_busy_percent");
    printFile("VRAM Total", "/sys/class/drm/card0/device/mem_info_vram_total");
    printFile("VRAM Used", "/sys/class/drm/card0/device/mem_info_vram_used");
    printFile("GTT Total", "/sys/class/drm/card0/device/mem_info_gtt_total");
    printFile("GTT Used", "/sys/class/drm/card0/device/mem_info_gtt_used");
#else
    os << "(Linux physical hardware symlink not available)\n";
#endif

    if (!printed)
        os << "GPU memory information is not available!\n";

    return printed;
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
} // glutil::debug