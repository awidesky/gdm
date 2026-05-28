#include <glutil/glutil.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstddef>
#include <filesystem>
#include <iostream>

#include "config.hpp"

namespace fs = std::filesystem;

static GLFWwindow* initGLFWAndContext();

static const glutil::VertexPNT kTexturedCubeVertices[] = {
    {-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.002000f, 1.0f - 0.002000f },
    {-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.002000f, 1.0f - 0.334000f },
    {-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.334000f, 1.0f - 0.334000f },
    { 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.998000f, 1.0f - 0.002000f },
    {-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.666000f, 1.0f - 0.334000f },
    {-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.998000f, 1.0f - 0.334000f },
    { 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.666000f, 1.0f - 0.334000f },
    {-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.334000f, 1.0f - 0.666000f },
    { 1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.666000f, 1.0f - 0.666000f },
    { 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.998000f, 1.0f - 0.002000f },
    { 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.666000f, 1.0f - 0.002000f },
    {-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.666000f, 1.0f - 0.334000f },
    {-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.002000f, 1.0f - 0.002000f },
    {-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.334000f, 1.0f - 0.334000f },
    {-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.334000f, 1.0f - 0.002000f },
    { 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.666000f, 1.0f - 0.334000f },
    {-1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.334000f, 1.0f - 0.334000f },
    {-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.334000f, 1.0f - 0.666000f },
    {-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.998000f, 1.0f - 0.666000f },
    {-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.998000f, 1.0f - 0.334000f },
    { 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.666000f, 1.0f - 0.334000f },
    { 1.0f,  1.0f,  1.0f, 1.0f,   0.0f,  0.0f, 0.666000f, 1.0f - 0.002000f },
    { 1.0f, -1.0f, -1.0f, 1.0f,   0.0f,  0.0f, 0.334000f, 1.0f - 0.334000f },
    { 1.0f,  1.0f, -1.0f, 1.0f,   0.0f,  0.0f, 0.666000f, 1.0f - 0.334000f },
    { 1.0f, -1.0f, -1.0f, 1.0f,   0.0f,  0.0f, 0.334000f, 1.0f - 0.334000f },
    { 1.0f,  1.0f,  1.0f, 1.0f,   0.0f,  0.0f, 0.666000f, 1.0f - 0.002000f },
    { 1.0f, -1.0f,  1.0f, 1.0f,   0.0f,  0.0f, 0.334000f, 1.0f - 0.002000f },
    { 1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.002000f, 1.0f - 0.334000f },
    { 1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.002000f, 1.0f - 0.666000f },
    {-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.334000f, 1.0f - 0.666000f },
    { 1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.002000f, 1.0f - 0.334000f },
    {-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.334000f, 1.0f - 0.334000f },
    {-1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.334000f, 1.0f - 0.666000f },
    { 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.666000f, 1.0f - 0.666000f },
    {-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.998000f, 1.0f - 0.666000f },
    { 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.666000f, 1.0f - 0.334000f },
};

int main() {
    GLFWwindow* window = initGLFWAndContext();
    if (!window) return 1;
    glutil::debug::printRuntimeInfo(true);

    const fs::path shaderDir = glutil::EXAMPLE_ASSET_DIR / "shader";
    const fs::path textureDir = glutil::EXAMPLE_ASSET_DIR / "texture";
    const fs::path modelDir = glutil::EXAMPLE_ASSET_DIR / "model";

    glutil::GLProgram program = glutil::ShaderLoader::loadProgramToGL(shaderDir / "debugLabel.vert",
                                                                      shaderDir / "debugLabel.frag");
    if (!program.ok) {
        std::cerr << "Program load failed: " << program.error << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    glutil::GLModelData loadedModel = glutil::ModelLoader::loadOBJtoGL(modelDir / "cube.obj", true);
    if (loadedModel.meshes.empty() || !loadedModel.meshes[0].ok) {
        std::cerr << "Cube model load failed: " << (modelDir / "cube.obj") << std::endl;
        program.reset();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    glutil::GLMeshData& firstCube = loadedModel.meshes[0];
    glutil::GLTexture2D firstCubeTex = glutil::ImageLoader::loadDDSToGL(textureDir / "diffuse.DDS");
    if (!firstCubeTex.ok) {
        std::cerr << "First texture load failed: " << firstCubeTex.error << std::endl;
        program.reset();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    glutil::TextureImage secondImage = glutil::ImageLoader::loadImage(textureDir / "33.bmp");
    if (!secondImage.ok) {
        std::cerr << "Second texture load failed: " << secondImage.error << std::endl;
        firstCubeTex.reset();
        program.reset();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    GLuint secondTexture = 0;
    glGenTextures(1, &secondTexture);
    glBindTexture(GL_TEXTURE_2D, secondTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, secondImage.internalFormat(), secondImage.width(), secondImage.height(), 0,
                 secondImage.format(), GL_UNSIGNED_BYTE, secondImage.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    if (secondTexture == 0) {
        std::cerr << "Second texture upload failed" << std::endl;
        firstCubeTex.reset();
        program.reset();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    GLuint secondVao = 0;
    GLuint secondVbo = 0;
    glGenVertexArrays(1, &secondVao);
    glGenBuffers(1, &secondVbo);
    glBindVertexArray(secondVao);
    glBindBuffer(GL_ARRAY_BUFFER, secondVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kTexturedCubeVertices), kTexturedCubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glutil::VertexPNT),
                          reinterpret_cast<void*>(offsetof(glutil::VertexPNT, x)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glutil::VertexPNT),
                          reinterpret_cast<void*>(offsetof(glutil::VertexPNT, nx)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(glutil::VertexPNT),
                          reinterpret_cast<void*>(offsetof(glutil::VertexPNT, u)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    auto ss = glutil::debug::snapshot(false)
                .shaderStatus(true)
                .textureInfo(true, true)
                .bufferVAOInfo(true, true, true)
                .allVBOInfo(true)
                .boundInfo(true);

    const GLint mvpLoc = glGetUniformLocation(program.id, "uMVP");
    const GLint texLoc = glGetUniformLocation(program.id, "uTexture");

    glEnable(GL_DEPTH_TEST);
    glUseProgram(program.id);
    if (texLoc >= 0) glUniform1i(texLoc, 0);

    glutil::debug::SnapshotAsyncHandle lastSnapshot;

    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && !glfwWindowShouldClose(window)) {
        glfwPollEvents();

        int fbW = 0;
        int fbH = 0;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        if (fbW > 0 && fbH > 0) glViewport(0, 0, fbW, fbH);

        const float time = static_cast<float>(glfwGetTime());
        const float aspect = (fbH > 0) ? static_cast<float>(fbW) / static_cast<float>(fbH) : 1.0f;

        const glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
        const glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 6.0f),
                                           glm::vec3(0.0f, 0.0f, 0.0f),
                                           glm::vec3(0.0f, 1.0f, 0.0f));

        glClearColor(0.10f, 0.12f, 0.16f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program.id);
        glActiveTexture(GL_TEXTURE0);

        const glm::mat4 firstModel = glm::rotate(
            glm::translate(glm::mat4(1.0f), glm::vec3(-1.6f, 0.0f, 0.0f)),
            time * 0.65f, glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::mat4 firstMvp = proj * view * firstModel;
        if (mvpLoc >= 0) glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &firstMvp[0][0]);
        glBindTexture(GL_TEXTURE_2D, firstCubeTex.id);
        glBindVertexArray(firstCube.vao);
        glDrawElements(GL_TRIANGLES, (GLsizei)firstCube.indexCount, GL_UNSIGNED_INT, nullptr);

        const glm::mat4 secondModel = glm::rotate(
            glm::translate(glm::mat4(1.0f), glm::vec3(1.6f, 0.0f, 0.0f)),
            -time * 0.85f, glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::mat4 secondMvp = proj * view * secondModel;
        if (mvpLoc >= 0) glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &secondMvp[0][0]);
        glBindTexture(GL_TEXTURE_2D, secondTexture);
        glBindVertexArray(secondVao);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(sizeof(kTexturedCubeVertices) / sizeof(kTexturedCubeVertices[0])));

        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);

        lastSnapshot = ss.capture();

        glfwSwapBuffers(window);
    }

    if (lastSnapshot)
        lastSnapshot.wait();

    glDeleteTextures(1, &secondTexture);
    glDeleteBuffers(1, &secondVbo);
    glDeleteVertexArrays(1, &secondVao);
    loadedModel.reset();
    firstCubeTex.reset();
    program.reset();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

static GLFWwindow* initGLFWAndContext() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW." << std::endl;
        return nullptr;
    }
    auto version = glutil::debug::availableGLversion();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, version.major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, version.minor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(__APPLE__)
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(1280, 720, "glutil_debugLabelExample", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window.\n";
        const char* description = nullptr;
        int code = glfwGetError(&description);
        std::cerr << "GLFW Error(" << code << "): " << description << std::endl;
        glfwTerminate();
        return nullptr;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD." << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return nullptr;
    }
    glutil::debug::init();
    return window;
}