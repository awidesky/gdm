#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glutil/glutil.hpp>

int main() {
    // GLFW 초기화
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
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

    float vertices[] = {0.f, 0.5f, 0.f, -0.5f, -0.5f, 0.f, 0.5f, -0.5f, 0.f};
    unsigned int indices[] = {0, 1, 2};

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
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

    // 렌더 루프 한 번
    glUseProgram(program);
    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);


    glEnableVertexAttribArray(0);



    // ── tracker 상태 확인 ──
    auto& tracker = glutil::debug::GLStateTracker::instance();

    // buffers 확인
    for (auto& [id, info] : tracker.buffers.getAll()) {
        printf("Buffer id=%u role=%s size=%lld dataType=0x%X\n", id,
               info.role == glutil::debug::BufferRole::VBO   ? "VBO"
               : info.role == glutil::debug::BufferRole::EBO ? "EBO"
                                                             : "Unknown",
               (long long)info.size, info.dataType);
    }

    // objects 확인
    for (auto& [name, ids] : tracker.objects.getAll()) {
        for (auto id : ids) {
            printf("Object id=%u type=%s\n", id, name.c_str());
        }
    }

    // ── 누수 테스트: VBO는 일부러 delete 안 함 ──
    glDeleteVertexArrays(1, &VAO);
    // glDeleteBuffers(1, &VBO);  ← 누수
    glDeleteBuffers(1, &EBO);



    printf("\n=== Leak Check ===\n");
    for (auto& [id, info] : tracker.buffers.getAll()) {
        printf("[LEAK] Buffer id=%u role=%s\n", id,
               info.role == glutil::debug::BufferRole::VBO   ? "VBO"
               : info.role == glutil::debug::BufferRole::EBO ? "EBO"
                                                             : "Unknown");
    }
    for (auto& [type, ids] : tracker.objects.getAll()) {
        for (auto id : ids) {
            printf("[LEAK] Object id=%u type=%s\n", id, type.c_str());
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}