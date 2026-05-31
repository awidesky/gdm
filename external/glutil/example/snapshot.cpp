#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glutil/glutil.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <filesystem>
#include "config.hpp"

void framebuffer_size_callback(GLFWwindow* window, int width, int height) { (void)window; glViewport(0, 0, width, height); }

GLuint makeDummyTexture(int unit, int w, int h, unsigned char r, unsigned char g, unsigned char b) {
    GLuint tex;
    glGenTextures(1, &tex);
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, tex);

    // w×h 픽셀 전체 할당
    std::vector<unsigned char> px(w * h * 4);
    for (int i = 0; i < w * h; i++) {
        px[i * 4 + 0] = r;
        px[i * 4 + 1] = g;
        px[i * 4 + 2] = b;
        px[i * 4 + 3] = 255;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    return tex;
}
GLuint makeTestShader() {
    const char* vsSource = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec2 aUV;
        uniform mat4 model;
        uniform vec3 lightPos;
        uniform float alpha;
        out vec2 uv;
        void main() {
            gl_Position = model * vec4(aPos + lightPos * 0.0, 1.0);
            uv = aUV;
        }
    )";

    const char* fsSource = R"(
        #version 330 core
        in vec2 uv;
        uniform sampler2D tex;
        out vec4 FragColor;
        uniform float alpha;

        void main() {
            FragColor = vec4(texture(tex, uv).rgb, alpha);
        }
    )";

    // 버텍스 셰이더
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vsSource, nullptr);
    glCompileShader(vs);

    // 프래그먼트 셰이더
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fsSource, nullptr);
    glCompileShader(fs);

    // 링크
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

// 1 VAO --> 1 VBO(FLOAT) , 2 VBO(FLOAT)
void makeTestVAO() {
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    float vertices[] = {
        0.0f, 0.5f, 0.0f,       0.5f, 1.0f,
        -0.5f, -0.5f, 0.0f,     0.0f, 0.0f,
        0.5f, -0.5f, 0.0f,      1.0f, 0.0f};

    unsigned int indices[] = {
      0, 1, 2, 
      0, 2, 3  
    };

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo); 
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 20, (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 20, (void*)12);
    glEnableVertexAttribArray(1);
}

// 1 VAO --> 1 VBO(FLOAT) , 2 VBO(INT)
void makeTestVAO2() {
    GLuint vao;
    GLuint vbo; (void)vbo;
    GLuint vbo2; (void)vbo2;
    GLuint ebo;

    float positions[] = {0.0f, 0.5f, 0.0f, -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f};
    int texcoords[] = {0, 1, 0, 0, 1, 0};

    unsigned int indices[] = {0, 1, 2, 0, 2, 3};

    GLuint vbo_pos, vbo_tex;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo_pos);
    glGenBuffers(1, &vbo_tex);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    // EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // VBO 1: position → attrib 0
    glBindBuffer(GL_ARRAY_BUFFER, vbo_pos);
    glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    // VBO 2: texcoord → attrib 1
    glBindBuffer(GL_ARRAY_BUFFER, vbo_tex);
    glBufferData(GL_ARRAY_BUFFER, sizeof(texcoords), texcoords, GL_STATIC_DRAW);
    glVertexAttribIPointer(1, 2, GL_INT, 0, (void*)0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}



int main() 
{ 
	glfwInit();
    auto version = glutil::debug::availableGLversion();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, version.major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, version.minor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(800, 800, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGL(glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    glutil::debug::initDebugCallbacks(); 

    // Make texture 2 , vao, vbo
    GLuint tex1 = makeDummyTexture(0, 1, 1, 255, 0, 0); (void)tex1;
    GLuint tex2 = makeDummyTexture(1, 2, 2, 0, 255, 0); (void)tex2;
    makeTestVAO();
    makeTestVAO2();
    GLuint shader = makeTestShader();
    glUseProgram(shader);
    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3f(glGetUniformLocation(shader, "lightPos"), 1.0f, 2.0f, 3.0f);
    glUniform1f(glGetUniformLocation(shader, "alpha"), 0.8f);
    glUniform1i(glGetUniformLocation(shader, "tex"), 0);
    glBindTexture(GL_TEXTURE_2D, 0);


    //glutil::debug::snapshot();
    //glutil::debug::snapshot ss = glutil::debug::snapshot{}.bufferVAOInfo(true, true, true, true);
    std::filesystem::path p1("C:/Temp/");
    glutil::debug::snapshot ss = glutil::debug::snapshot{}.bufferVAOInfo(true, true, true, true).printPerCall(true);
    ss.capture(p1,true);
    ss.capture();
    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();

    //glDeleteTextures(1, &tex1);
    //glDeleteTextures(1, &tex2);
    //glDeleteVertexArrays(1, &vao);
    //glDeleteBuffers(1, &vbo);
}