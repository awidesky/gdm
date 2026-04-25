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

#ifdef GDM_HAS_GLUTIL
#include <glutil/glutil.hpp>
#endif

#include <array>
#include <iostream>
#include <string>

namespace {

#ifdef GDM_HAS_FREEGLUT
bool g_shouldClose = false;

void onFreeglutKeyboard(unsigned char key, int, int) {
    if (key == 27) {
        g_shouldClose = true;
    }
}
#endif

GLuint compileShader(GLenum type, const char* source) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

#ifdef GDM_HAS_GLUTIL
    const glutil::InspectResult result = glutil::Inspector::shaderCompileResult(shader);
    if (result.ok) {
        return shader;
    }

    std::cerr << "Shader compile failed:\n" << result.message << std::endl;
#else
    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_TRUE) {
        return shader;
    }

    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    std::string log(static_cast<size_t>(length), '\0');
    glGetShaderInfoLog(shader, length, nullptr, log.data());
    std::cerr << "Shader compile failed:\n" << log << std::endl;
#endif

    glDeleteShader(shader);
    return 0;
}

GLuint createProgram() {
    constexpr const char* kVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
out vec3 vColor;

void main() {
    gl_Position = vec4(aPos, 1.0);
    vColor = aColor;
}
)";

    constexpr const char* kFragmentShader = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(vColor, 1.0);
}
)";

    const GLuint vs = compileShader(GL_VERTEX_SHADER, kVertexShader);
    const GLuint fs = compileShader(GL_FRAGMENT_SHADER, kFragmentShader);
    if (vs == 0 || fs == 0) {
        if (vs != 0) glDeleteShader(vs);
        if (fs != 0) glDeleteShader(fs);
        return 0;
    }

    const GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

#ifdef GDM_HAS_GLUTIL
    const glutil::InspectResult result = glutil::Inspector::programLinkResult(program);
    if (result.ok) {
        return program;
    }

    std::cerr << "Program link failed:\n" << result.message << std::endl;
#else
    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success == GL_TRUE) {
        return program;
    }

    GLint length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    std::string log(static_cast<size_t>(length), '\0');
    glGetProgramInfoLog(program, length, nullptr, log.data());
    std::cerr << "Program link failed:\n" << log << std::endl;
#endif

    glDeleteProgram(program);
    return 0;
}

void printRuntimeInfo() {
    std::cout << "=== Runtime Dependency Info ===\n";

#ifdef GDM_HAS_GLFW
    int major = 0;
    int minor = 0;
    int rev = 0;
    glfwGetVersion(&major, &minor, &rev);
    std::cout << "[GLFW] version: " << major << "." << minor << "." << rev
              << ", string: " << glfwGetVersionString() << "\n";
#endif

#ifdef GDM_HAS_FREEGLUT
    const int glutVersion = glutGet(GLUT_VERSION); // ex: 30400 = 3.4.0
    std::cout << "[FreeGLUT] GLUT_VERSION(raw): " << glutVersion << "\n";
#endif

#ifdef GDM_HAS_GLEW
    const GLubyte* glewVer = glewGetString(GLEW_VERSION);
    std::cout << "[GLEW] version: " << (glewVer ? reinterpret_cast<const char*>(glewVer) : "(null)") << "\n";
#endif

#ifdef GDM_HAS_GLAD
    std::cout << "[GLAD] Generator version : " << GLAD_GENERATOR_VERSION << ", Debug option "
    #ifdef GLAD_OPTION_GL_DEBUG
        << "ON"
    #else
        << "OFF"
    #endif
    << "\n";
#endif

#ifdef GDM_HAS_GLM
    std::cout << "[GLM]  version: "
              << GLM_VERSION_MAJOR << "."
              << GLM_VERSION_MINOR << "."
              << GLM_VERSION_PATCH << "."
              << GLM_VERSION_REVISION << "\n";
#endif

#ifdef GDM_HAS_GLUTIL
    // TODO : print glutil version
    std::cout << "[GLUtil] enabled (Inspector API available)\n";
#else
    std::cout << "[GLUtil] disabled\n";
#endif
    std::cout << "\n";

    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    const char* slVersion = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
    const char* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));

    std::cout << "[OpenGL] version: " << (version ? version : "(null)") << "\n";
    std::cout << "[OpenGL] GLSL: " << (slVersion ? slVersion : "(null)") << "\n";
    std::cout << "[OpenGL] vendor: " << (vendor ? vendor : "(null)")
              << ", renderer: " << (renderer ? renderer : "(null)") << "\n";
    std::cout << "===============================\n";
}

bool initializeLoader() {
#ifdef GDM_HAS_GLAD
    int gladVersion = 0;
    #ifdef GDM_HAS_GLFW
    gladVersion = gladLoadGL(glfwGetProcAddress);
    #elif defined(GDM_HAS_FREEGLUT)
    gladVersion = gladLoadGL(reinterpret_cast<GLADloadfunc>(glutGetProcAddress));
    #endif

    if (gladVersion == 0) {
        std::cerr << "Failed to initialize GLAD context." << std::endl;
        return false;
    }

    std::cout << "GLAD loaded OpenGL: "
              << GLAD_VERSION_MAJOR(gladVersion) << "."
              << GLAD_VERSION_MINOR(gladVersion) << std::endl;
    return true;
#elif defined(GDM_HAS_GLEW)
    glewExperimental = GL_TRUE;
    const GLenum glewErr = glewInit();
    std::cout << "GLEW init : " << glewGetErrorString(glewErr) << std::endl;
    if (glewErr != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW: "
                  << reinterpret_cast<const char*>(glewGetErrorString(glewErr))
                  << std::endl;
        return false;
    }
    return true;
#else
    std::cerr << "No loader selected. (expected GLAD or GLEW)" << std::endl;
    return false;
#endif
}

} // namespace

int main(int argc, char** argv) {
#ifdef GDM_HAS_GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(800, 600, "GDM Example 1 - Hello Window", nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);

#elif defined(GDM_HAS_FREEGLUT)
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutInitContextVersion(3, 3);
#ifdef GLUT_CORE_PROFILE
    glutInitContextProfile(GLUT_CORE_PROFILE);
#endif

    const int window = glutCreateWindow("GDM Example 1 - Hello Window");
    if (window <= 0) {
        std::cerr << "Failed to create FreeGLUT window" << std::endl;
        return 1;
    }

    glutKeyboardFunc(onFreeglutKeyboard);

#else
    std::cerr << "No window backend selected. (expected GLFW or FreeGLUT)" << std::endl;
    return 1;
#endif

    if (!initializeLoader()) {
#ifdef GDM_HAS_GLFW
        glfwDestroyWindow(window);
        glfwTerminate();
#elif defined(GDM_HAS_FREEGLUT)
        glutDestroyWindow(glutGetWindow());
#endif
        return 1;
    }

    printRuntimeInfo();

    const std::array<float, 18> vertices = {
        -0.5f, -0.5f, 0.0f, 1.0f, 0.2f, 0.2f,
         0.5f, -0.5f, 0.0f, 0.2f, 1.0f, 0.2f,
         0.0f,  0.5f, 0.0f, 0.2f, 0.4f, 1.0f,
    };

    GLuint vao = 0;
    GLuint vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(6 * sizeof(float)), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(6 * sizeof(float)), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    const GLuint program = createProgram();
    if (program == 0) {
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
#ifdef GDM_HAS_GLFW
        glfwDestroyWindow(window);
        glfwTerminate();
#elif defined(GDM_HAS_FREEGLUT)
        glutDestroyWindow(glutGetWindow());
#endif
        return 1;
    }

#ifdef GDM_HAS_GLM
    const glm::vec3 clearColor(0.0f, 0.0f, 0.4f);
#else
    struct {
        float r;
        float g;
        float b;
    } clearColor = {0.0f, 0.0f, 0.4f};
#endif

#ifdef GDM_HAS_GLFW
    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && !glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glClearColor(clearColor.r, clearColor.g, clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
    }
#elif defined(GDM_HAS_FREEGLUT)
    while (!g_shouldClose) {
        glutMainLoopEvent();

        glClearColor(clearColor.r, clearColor.g, clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glutSwapBuffers();
    }
#endif

    glDeleteProgram(program);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

#ifdef GDM_HAS_GLFW
    glfwDestroyWindow(window);
    glfwTerminate();
#elif defined(GDM_HAS_FREEGLUT)
    glutDestroyWindow(glutGetWindow());
#endif

    return 0;
}
