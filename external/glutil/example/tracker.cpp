#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glutil/glutil.hpp>

int main() {
    // GLFW 초기화
    glfwInit();
    auto version = glutil::debug::availableGLversion();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, version.major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, version.minor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "tracker test", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    if (!gladLoadGL(glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }


    glutil::debug::initDebugCallbacks();

    // ── 리소스 생성 ──
    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    GLuint VAO2, VBO2, EBO2;
    glGenVertexArrays(1, &VAO2);
    glGenBuffers(1, &VBO2);
    glGenBuffers(1, &EBO2);

    float vertices[] = {0.f, 0.5f, 0.f, -0.5f, -0.5f, 0.f, 0.5f, -0.5f, 0.f};
    unsigned int indices[] = {0, 1, 2};

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    //glBindBuffer(GL_ARRAY_BUFFER, VBO2);
    //glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    LOG_INFO() << "Not binding buffer id=" << VBO2 << ", may cause the tracker to not know the type.";

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO2);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    const char* vertSrc = R"(
    #version 330 core
    layout(location = 0) in vec3 aPos;
    void main() { gl_Position = vec4(aPos, 1.0); }
)";
    const char* fragSrc = R"(
    #version 330 core
    out vec4 FragColor;
    void main() { FragColor = vec4(1.0); }
)";

    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &vertSrc, nullptr);
    glCompileShader(vert);

    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &fragSrc, nullptr);
    glCompileShader(frag);

    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);

    glUseProgram(program);
    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);


    glEnableVertexAttribArray(0);


    auto& tracker = glutil::debug::GLStateTracker::instance();

    for (auto& [id, info] : tracker.buffers.getAll()) {
        LOG_INFO() << "Buffer id=" << id
                   << " role="
                   << (info.role == glutil::debug::BufferRole::VBO   ? "VBO"
                       : info.role == glutil::debug::BufferRole::EBO ? "EBO"
                                                                     : "Unknown")
                   << " size=" << static_cast<long long>(info.size)
                   << " dataType=0x" << std::hex << info.dataType << std::dec;
    }

    for (auto& [key, info] : tracker.objects.getAll()) {
        const auto& [type, id] = key;
        LOG_INFO() << "Object id=" << id << " type=" << type;
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &EBO);

    LOG_INFO() << "=== Leak Check(in trackerExample, not dtor of GLStateTracker) ===";
    for (auto& [id, info] : tracker.buffers.getAll()) {
        LOG_ERROR() << "[LEAK] Buffer id=" << id
                    << " role="
                    << (info.role == glutil::debug::BufferRole::VBO   ? "VBO"
                        : info.role == glutil::debug::BufferRole::EBO ? "EBO"
                                                                      : "Unknown")
                    << " label=" << (info.label.empty() ? "(none)" : info.label);
    }
    for (auto& [key, info] : tracker.objects.getAll()) {
        const auto& [type, id] = key;
        LOG_ERROR() << "[LEAK] Object id=" << id << " type=" << type
                    << " label=" << (info.label.empty() ? "(none)" : info.label);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}