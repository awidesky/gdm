#include <glutil/glutil.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstddef>
#include <filesystem>
#include <iostream>
#include <string>

#include "config.hpp"

namespace fs = std::filesystem;

static GLFWwindow* initGLFWAndContext();
static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

static const glm::vec3 kTangents[] = {
    {-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f},
    {-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f},
    { 0.0f, 0.0f, 1.0f}, { 0.0f, 0.0f, 1.0f}, { 0.0f, 0.0f, 1.0f},
    { 0.0f, 0.0f, 1.0f}, { 0.0f, 0.0f, 1.0f}, { 0.0f, 0.0f, 1.0f},
    {-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f},
    {-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f},
    { 0.0f, 1.0f, 0.0f}, { 0.0f, 1.0f, 0.0f}, { 0.0f, 1.0f, 0.0f},
    { 0.0f, 1.0f, 0.0f}, { 0.0f, 1.0f, 0.0f}, { 0.0f, 1.0f, 0.0f},
    {-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f},
    {-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f},
    { 0.0f, 0.0f, 1.0f}, { 0.0f, 0.0f, 1.0f}, { 0.0f, 0.0f, 1.0f},
    { 0.0f, 0.0f, 1.0f}, { 0.0f, 0.0f, 1.0f}, { 0.0f, 0.0f, 1.0f}
};

static const glm::vec3 kBitangents[] = {
    { 0.0f, 1.0f, 0.0f}, { 0.0f, 1.0f, 0.0f}, { 0.0f, 1.0f, 0.0f},
    { 0.0f, 1.0f, 0.0f}, { 0.0f, 1.0f, 0.0f}, { 0.0f, 1.0f, 0.0f},
    { 0.0f, 1.0f,-0.0f}, { 0.0f, 1.0f,-0.0f}, { 0.0f, 1.0f,-0.0f},
    { 0.0f, 1.0f,-0.0f}, { 0.0f, 1.0f,-0.0f}, { 0.0f, 1.0f,-0.0f},
    { 0.0f,-1.0f, 0.0f}, { 0.0f,-1.0f, 0.0f}, { 0.0f,-1.0f, 0.0f},
    { 0.0f,-1.0f, 0.0f}, { 0.0f,-1.0f, 0.0f}, { 0.0f,-1.0f, 0.0f},
    { 0.0f, 0.0f, 1.0f}, { 0.0f, 0.0f, 1.0f}, { 0.0f, 0.0f, 1.0f},
    { 0.0f, 0.0f, 1.0f}, { 0.0f, 0.0f, 1.0f}, { 0.0f, 0.0f, 1.0f},
    { 0.0f,-0.0f, 1.0f}, { 0.0f,-0.0f, 1.0f}, { 0.0f,-0.0f, 1.0f},
    { 0.0f,-0.0f, 1.0f}, { 0.0f,-0.0f, 1.0f}, { 0.0f,-0.0f, 1.0f},
    {-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f},
    {-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}
};

struct InputState {
    bool w=false, s=false, a=false, d=false;
    bool space=false, shift=false;
    bool left=false, right=false, up=false, down=false;
} g_input;


int main() {
    GLFWwindow* window = initGLFWAndContext();
    if (!window) return 1;
    glfwSetKeyCallback(window, keyCallback);

    const fs::path shaderDir = glutil::EXAMPLE_ASSET_DIR / "shader";
    const fs::path vsPath = shaderDir / "normalMapping.vert";
    const fs::path fsPath = shaderDir / "normalMapping.frag";

    const fs::path textureDir = glutil::EXAMPLE_ASSET_DIR / "texture";
    const fs::path diffusePath = textureDir / "diffuse.DDS";
    const fs::path normalPath = textureDir / "normal.bmp";
    const fs::path specularPath = textureDir / "specular.DDS";

    glutil::GLProgram programResult = glutil::ShaderLoader::loadProgramToGL(vsPath, fsPath);
    if (!programResult.ok) {
        std::cerr << "Program load failed: " << programResult.error << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    const GLuint program = programResult.id;

    glutil::GLTexture2D diffuseTex = glutil::ImageLoader::loadDDSToGL(diffusePath);
    glutil::GLTexture2D normalTex = glutil::ImageLoader::loadImageToGL(normalPath);
    glutil::GLTexture2D specularTex = glutil::ImageLoader::loadDDSToGL(specularPath);

    if (!diffuseTex.ok || !normalTex.ok || !specularTex.ok) {
        std::cerr << "Texture load failed: diffuse=" << diffuseTex.error
                  << " normal=" << normalTex.error
                  << " specular=" << specularTex.error << std::endl;
        diffuseTex.reset();
        normalTex.reset();
        specularTex.reset();
        programResult.reset();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    const fs::path modelPath = glutil::EXAMPLE_ASSET_DIR / "model" / "cube.obj";
    glutil::GLModelData glModel = glutil::ModelLoader::loadOBJtoGL(modelPath, false);
    if (!glModel.ok || glModel.meshes.empty()) {
        std::cerr << "Model load failed: " << modelPath << "\n  reason: " << glModel.error << std::endl;
        diffuseTex.reset();
        normalTex.reset();
        specularTex.reset();
        programResult.reset();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    glutil::GLMeshData& gmesh = glModel.meshes[0];
    const size_t vertexCount = static_cast<size_t>(gmesh.indexCount);

    glBindVertexArray(gmesh.vao);

    GLuint vboTangent = 0;
    GLuint vboBitangent = 0;

    glGenBuffers(1, &vboTangent);
    glBindBuffer(GL_ARRAY_BUFFER, vboTangent);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(kTangents)), kTangents, GL_STATIC_DRAW);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);

    glGenBuffers(1, &vboBitangent);
    glBindBuffer(GL_ARRAY_BUFFER, vboBitangent);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(kBitangents)), kBitangents, GL_STATIC_DRAW);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glutil::debug::snapshot(false)
        .shaderStatus(true)
        .textureInfo(true, true)
        .bufferVAOInfo(true, true)
        .boundInfo(true)
        .capture();

    glEnable(GL_DEPTH_TEST);

    const GLint mvpLoc = glGetUniformLocation(program, "uMVP");
    const GLint modelLoc = glGetUniformLocation(program, "uModel");
    const GLint viewLoc = glGetUniformLocation(program, "uView");
    const GLint normalMatLoc = glGetUniformLocation(program, "uNormalMat");
    const GLint lightPosLoc = glGetUniformLocation(program, "uLightPos");
    const GLint viewPosLoc = glGetUniformLocation(program, "uViewPos");
    const GLint lightColorLoc = glGetUniformLocation(program, "uLightColor");
    const GLint lightPowerLoc = glGetUniformLocation(program, "uLightPower");
    const GLint diffuseTexLoc = glGetUniformLocation(program, "uDiffuseTex");
    const GLint normalTexLoc = glGetUniformLocation(program, "uNormalTex");
    const GLint specularTexLoc = glGetUniformLocation(program, "uSpecularTex");

    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && !glfwWindowShouldClose(window)) {
        glfwPollEvents();

        int fbW = 0;
        int fbH = 0;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        if (fbW > 0 && fbH > 0) {
            glViewport(0, 0, fbW, fbH);
        }

        const float t = static_cast<float>(glfwGetTime());
        const float aspect = (fbH > 0) ? static_cast<float>(fbW) / static_cast<float>(fbH) : 1.0f;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.08f, 0.08f, 0.12f, 1.0f);

        glUseProgram(program);

        glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(glm::vec3(2.5f, 2.0f, 2.5f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 modelMat(1.0f);
        modelMat = glm::rotate(modelMat, t * 0.6f, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 mvp = proj * view * modelMat;
        glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(modelMat)));

        if (mvpLoc >= 0) glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
        if (modelLoc >= 0) glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMat));
        if (viewLoc >= 0) glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        if (normalMatLoc >= 0) glUniformMatrix3fv(normalMatLoc, 1, GL_FALSE, glm::value_ptr(normalMat));
        if (lightPosLoc >= 0) glUniform3f(lightPosLoc, 2.0f, 3.0f, 2.0f);
        if (viewPosLoc >= 0) glUniform3f(viewPosLoc, 2.5f, 2.0f, 2.5f);
        if (lightColorLoc >= 0) glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);
        if (lightPowerLoc >= 0) glUniform1f(lightPowerLoc, 2.0f);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseTex.id);
        if (diffuseTexLoc >= 0) glUniform1i(diffuseTexLoc, 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normalTex.id);
        if (normalTexLoc >= 0) glUniform1i(normalTexLoc, 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, specularTex.id);
        if (specularTexLoc >= 0) glUniform1i(specularTexLoc, 2);

        glBindVertexArray(gmesh.vao);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertexCount));

        glfwSwapBuffers(window);
    }

    glDeleteBuffers(1, &vboBitangent);
    glDeleteBuffers(1, &vboTangent);
    gmesh.reset();
    glModel.meshes.clear();
    specularTex.reset();
    normalTex.reset();
    diffuseTex.reset();
    programResult.reset();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

static GLFWwindow* initGLFWAndContext() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return nullptr;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* ret = glfwCreateWindow(1024, 768, "glutil debug label example", nullptr, nullptr);
    if (!ret) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return nullptr;
    }

    glfwMakeContextCurrent(ret);
    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GL loader" << std::endl;
        glfwDestroyWindow(ret);
        glfwTerminate();
        return nullptr;
    }

    glutil::debug::init();

    glfwSetInputMode(ret, GLFW_STICKY_KEYS, GL_TRUE);
    return ret;
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)window; (void)scancode; (void)mods;
    const bool down = (action == GLFW_PRESS || action == GLFW_REPEAT);
    switch (key) {
        case GLFW_KEY_W: g_input.w = down; break;
        case GLFW_KEY_S: g_input.s = down; break;
        case GLFW_KEY_A: g_input.a = down; break;
        case GLFW_KEY_D: g_input.d = down; break;
        case GLFW_KEY_SPACE: g_input.space = down; break;
        case GLFW_KEY_LEFT_SHIFT: case GLFW_KEY_RIGHT_SHIFT: g_input.shift = down; break;
        case GLFW_KEY_LEFT: g_input.left = down; break;
        case GLFW_KEY_RIGHT: g_input.right = down; break;
        case GLFW_KEY_UP: g_input.up = down; break;
        case GLFW_KEY_DOWN: g_input.down = down; break;
        default: break;
    }
}
