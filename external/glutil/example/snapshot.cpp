#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glutil/glutil.hpp>
#include <iostream>
#include "config.hpp"

void framebuffer_size_callback(GLFWwindow* window, int width, int height) { glViewport(0, 0, width, height); }

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


void makeTestVAO(GLuint& vao, GLuint& vbo) {
    float vertices[] = {
        0.0f, 0.5f, 0.0f,       0.5f, 1.0f,
        -0.5f, -0.5f, 0.0f,     0.0f, 0.0f,
        0.5f, -0.5f, 0.0f,      1.0f, 0.0f};
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 20, (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 20, (void*)12);
    glEnableVertexAttribArray(1);
}

int main() 
{ 
	glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
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


    // Make texture 2 , vao, vbo
    GLuint tex1 = makeDummyTexture(0, 1, 1, 255, 0, 0);
    GLuint tex2 = makeDummyTexture(1, 2, 2, 0, 255, 0);
    GLuint vao, vbo;
    makeTestVAO(vao, vbo);
    glutil::debug::snapshot();



    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();

    glDeleteTextures(1, &tex1);
    glDeleteTextures(1, &tex2);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}