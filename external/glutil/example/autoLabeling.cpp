#include <glutil/glutil.hpp>

#include <filesystem>
#include <iostream>
#include <string>

#include "config.hpp"

namespace fs = std::filesystem;

static GLFWwindow* initGLFWAndContext();
static void printObjectLabel(const char* stage, GLenum identifier, GLuint id);
static void printBoundInfo(const char* stage);
static bool checkShader(GLuint shader, const char* stage);
static bool checkProgram(GLuint program, const char* stage);

static const char* kManualFragmentShader = R"(
#version 330 core
out vec4 FragColor;
void main() {
    FragColor = vec4(1.0, 0.2, 0.1, 1.0);
}
)";

int main() {
    GLFWwindow* window = initGLFWAndContext();
    if (!window) return 1;

    glutil::debug::printRuntimeInfo(true);
    if (!glutil::debug::isGL_KHR_debugSupported())
        std::cout << "KHR_debug not supported; labels may be empty.\n";

    const fs::path shaderDir = glutil::EXAMPLE_ASSET_DIR / "shader";

    std::cout << "\n=== Shader (ShaderLoader) ===\n";
    glutil::GLShader fileVertex = glutil::ShaderLoader::loadShaderToGL(
        GL_VERTEX_SHADER, shaderDir / "debugLabel.vert");
    if (!fileVertex.ok) {
        std::cerr << "ShaderLoader vertex load failed: " << fileVertex.error << "\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    printObjectLabel("ShaderLoader VS after load (create+compile)", GL_SHADER, fileVertex.id);

    std::cout << "\n=== Shader (Manual) ===\n";
    GLuint manualFragment = glCreateShader(GL_FRAGMENT_SHADER);
    printObjectLabel("Manual FS after create", GL_SHADER, manualFragment);
    glShaderSource(manualFragment, 1, &kManualFragmentShader, nullptr);
    glCompileShader(manualFragment);
    if (!checkShader(manualFragment, "Manual FS")) {
        glDeleteShader(manualFragment);
        fileVertex.reset();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    printObjectLabel("Manual FS after compile", GL_SHADER, manualFragment);

    std::cout << "\n=== Program (Manual) ===\n";
    GLuint manualProgram = glCreateProgram();
    printObjectLabel("Manual program after create", GL_PROGRAM, manualProgram);

    glAttachShader(manualProgram, fileVertex.id);
    printObjectLabel("Manual program after attach file VS", GL_PROGRAM, manualProgram);
    printObjectLabel("ShaderLoader VS after attach", GL_SHADER, fileVertex.id);

    glAttachShader(manualProgram, manualFragment);
    printObjectLabel("Manual program after attach manual FS", GL_PROGRAM, manualProgram);
    printObjectLabel("Manual FS after attach", GL_SHADER, manualFragment);

    glLinkProgram(manualProgram);
    if (!checkProgram(manualProgram, "Manual program")) {
        glDeleteProgram(manualProgram);
        glDeleteShader(manualFragment);
        fileVertex.reset();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    printObjectLabel("Manual program after link", GL_PROGRAM, manualProgram);

    glUseProgram(manualProgram);
    printBoundInfo("after glUseProgram(manualProgram)");

    std::cout << "\n=== Program (ShaderLoader) ===\n";
    glutil::GLProgram fileProgram = glutil::ShaderLoader::loadProgramToGL(
        shaderDir / "debugLabel.vert", shaderDir / "debugLabel.frag");
    if (!fileProgram.ok) {
        std::cerr << "ShaderLoader program load failed: " << fileProgram.error << "\n";
        glDeleteProgram(manualProgram);
        glDeleteShader(manualFragment);
        fileVertex.reset();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    printObjectLabel("ShaderLoader program after load (create+attach+link)", GL_PROGRAM, fileProgram.id);

    glUseProgram(fileProgram.id);
    printBoundInfo("after glUseProgram(ShaderLoader program)");

    std::cout << "\n=== Vertex Array ===\n";
    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    printObjectLabel("VAO after glGenVertexArrays", GL_VERTEX_ARRAY, vao);
    glBindVertexArray(vao);
    printObjectLabel("VAO after glBindVertexArray", GL_VERTEX_ARRAY, vao);
    printBoundInfo("after glBindVertexArray");

    std::cout << "\n=== Buffers (VBO/EBO) ===\n";
    GLuint vbo = 0;
    glGenBuffers(1, &vbo);
    printObjectLabel("VBO after glGenBuffers", GL_BUFFER, vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    printObjectLabel("VBO after glBindBuffer(GL_ARRAY_BUFFER)", GL_BUFFER, vbo);
    printBoundInfo("after glBindBuffer(GL_ARRAY_BUFFER)");

    const float triangleVertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangleVertices), triangleVertices, GL_STATIC_DRAW);

    GLuint ebo = 0;
    glGenBuffers(1, &ebo);
    printObjectLabel("EBO after glGenBuffers", GL_BUFFER, ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    printObjectLabel("EBO after glBindBuffer(GL_ELEMENT_ARRAY_BUFFER)", GL_BUFFER, ebo);
    printBoundInfo("after glBindBuffer(GL_ELEMENT_ARRAY_BUFFER)");

    const unsigned int triangleIndices[] = { 0, 1, 2 };
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(triangleIndices), triangleIndices, GL_STATIC_DRAW);

    std::cout << "\n=== Texture ===\n";
    GLuint texture = 0;
    glGenTextures(1, &texture);
    printObjectLabel("Texture after glGenTextures", GL_TEXTURE, texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    printObjectLabel("Texture after glBindTexture(GL_TEXTURE_2D)", GL_TEXTURE, texture);
    printBoundInfo("after glBindTexture(GL_TEXTURE_2D)");

    const unsigned char texPixels[] = {
        255,   0,   0, 255,   0, 255,   0, 255,
          0,   0, 255, 255, 255, 255,   0, 255
    };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, texPixels);
    printObjectLabel("Texture after glTexImage2D", GL_TEXTURE, texture);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);

    glDeleteTextures(1, &texture);
    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

    glDeleteProgram(manualProgram);
    glDeleteShader(manualFragment);

    fileProgram.reset();
    fileVertex.reset();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

static void printObjectLabel(const char* stage, GLenum identifier, GLuint id) {
    const std::string label = glutil::debug::getGLobjectLabel(identifier, id);
    std::cout << stage << " id=" << id;
    if (label.empty())
        std::cout << " label=<none>";
    else
        std::cout << " label=\"" << label << "\"";
    std::cout << '\n';
}

static void printBoundInfo(const char* stage) {
    std::cout << "\n[BoundInfo] " << stage << '\n';
    glutil::debug::snapshot(false)
        .boundInfo(true)
        .enableTiming(false)
        .capture(std::cout, false);
}

static bool checkShader(GLuint shader, const char* stage) {
    const glutil::InspectResult result = glutil::Inspector::shaderCompileResult(shader);
    if (result.ok) return true;
    std::cerr << stage << " compile failed: " << result.message << "\n";
    return false;
}

static bool checkProgram(GLuint program, const char* stage) {
    const glutil::InspectResult result = glutil::Inspector::programLinkResult(program);
    if (result.ok) return true;
    std::cerr << stage << " link failed: " << result.message << "\n";
    return false;
}

static GLFWwindow* initGLFWAndContext() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW.\n";
        return nullptr;
    }

    auto version = glutil::debug::availableGLversion();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, version.major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, version.minor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(__APPLE__)
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(800, 600, "glutil_autoLabelingExample", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window.\n";
        glfwTerminate();
        return nullptr;
    }

    glfwMakeContextCurrent(window);
    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return nullptr;
    }

    glutil::debug::init();
    return window;
}
