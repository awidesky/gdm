#include <glutil/gl.hpp>
#include <glutil/inspector.hpp>
#include <glutil/texture.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <filesystem>
#include <iostream>
#include <string>

#include "config.hpp"

namespace glutil {

namespace {

GLuint compileShader(GLenum type, const char* source) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    const InspectResult r = Inspector::shaderCompileResult(shader);
    if (r.ok)
        return shader;

    std::cerr << "Shader compile failed:\n" << r.message << std::endl;

    glDeleteShader(shader);
    return 0;
}

GLuint createProgram(const char* vertexSource, const char* fragmentSource) {
    const GLuint vs = compileShader(GL_VERTEX_SHADER, vertexSource);
    const GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
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

    const InspectResult r = Inspector::programLinkResult(program);
    if (r.ok)
        return program;

    std::cerr << "Program link failed:\n" << r.message << std::endl;

    glDeleteProgram(program);
    return 0;
}

GLuint uploadStandard2D(const TextureImage& image) {
    if (!image.ok || image.data() == nullptr)
        return 0;

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 image.glInternalFormat(),
                 image.width(),
                 image.height(),
                 0,
                 image.glFormat(),
                 GL_UNSIGNED_BYTE,
                 image.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    return tex;
}

GLuint uploadDDS2D(const TextureDDS& dds) {
    if (!dds.ok || dds.data() == nullptr || dds.mips().empty())
        return 0;

    const int hasS3tcExt = glfwExtensionSupported("GL_EXT_texture_compression_s3tc");
    const int hasS3tcArb = glfwExtensionSupported("GL_ARB_texture_compression_s3tc");
    if (!hasS3tcExt && !hasS3tcArb) {
        std::cerr << "DDS upload skipped: S3TC extension is not available in current context.\n"
                  << "(need GL_EXT_texture_compression_s3tc or GL_ARB_texture_compression_s3tc)"
                  << std::endl;
        return 0;
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(dds.mips().size() - 1));

    for (size_t level = 0; level < dds.mips().size(); ++level) {
        const MipLevel& m = dds.mips()[level];
        glCompressedTexImage2D(GL_TEXTURE_2D,
                               static_cast<GLint>(level),
                               dds.glFormat(),
                               m.width,
                               m.height,
                               0,
                               m.size,
                               dds.data() + m.offset);

        const GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "DDS glCompressedTexImage2D failed at mip " << level
                      << " (glError=0x" << std::hex << err << std::dec << ")"
                      << std::endl;
            glBindTexture(GL_TEXTURE_2D, 0);
            glDeleteTextures(1, &tex);
            return 0;
        }
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

} // namespace

} // namespace glutil

int main() {
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

    GLFWwindow* window = glfwCreateWindow(1024, 768, "glutil texture test", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GL loader" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    const std::filesystem::path textureDir = glutil::TEST_ASSET_DIR / "texture";
    const std::filesystem::path ddsPath = textureDir / "uvtemplate.DDS";
    const std::filesystem::path bmpPath = textureDir / "dice.bmp";

    glutil::TextureDDS dds = glutil::ImageLoader::LoadDDS(ddsPath.string().c_str());
    if (!dds.ok) {
        std::cerr << "DDS load failed: " << ddsPath << "\n  reason: " << dds.error << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    glutil::TextureImage bmp = glutil::ImageLoader::LoadImage(bmpPath.string().c_str(), true);
    if (!bmp.ok) {
        std::cerr << "BMP load failed: " << bmpPath << "\n  reason: " << bmp.error << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    const GLuint ddsTex = glutil::uploadDDS2D(dds);
    const GLuint bmpTex = glutil::uploadStandard2D(bmp);
    if (ddsTex == 0 || bmpTex == 0) {
        std::cerr << "Texture upload failed (ddsTex=" << ddsTex << ", bmpTex=" << bmpTex << ")" << std::endl;
        if (ddsTex != 0) glDeleteTextures(1, &ddsTex);
        if (bmpTex != 0) glDeleteTextures(1, &bmpTex);
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    constexpr const char* kVS = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUV;

uniform mat4 uMVP;
out vec2 vUV;

void main() {
    vUV = aUV;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

    constexpr const char* kFS = R"(
#version 330 core
in vec2 vUV;
uniform sampler2D uTexture;
out vec4 FragColor;

void main() {
    FragColor = texture(uTexture, vUV);
}
)";

    const GLuint program = glutil::createProgram(kVS, kFS);
    if (program == 0) {
        glDeleteTextures(1, &ddsTex);
        glDeleteTextures(1, &bmpTex);
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    const GLint mvpLoc = glGetUniformLocation(program, "uMVP");
    const GLint texLoc = glGetUniformLocation(program, "uTexture");

    static const GLfloat kVertexData[] = {
        -1.0f,-1.0f,-1.0f, -1.0f,-1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
         1.0f, 1.0f,-1.0f, -1.0f,-1.0f,-1.0f, -1.0f, 1.0f,-1.0f,
         1.0f,-1.0f, 1.0f, -1.0f,-1.0f,-1.0f, 1.0f,-1.0f,-1.0f,
         1.0f, 1.0f,-1.0f,  1.0f,-1.0f,-1.0f, -1.0f,-1.0f,-1.0f,
        -1.0f,-1.0f,-1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f,-1.0f,
         1.0f,-1.0f, 1.0f, -1.0f,-1.0f, 1.0f, -1.0f,-1.0f,-1.0f,
        -1.0f, 1.0f, 1.0f, -1.0f,-1.0f, 1.0f,  1.0f,-1.0f, 1.0f,
         1.0f, 1.0f, 1.0f,  1.0f,-1.0f,-1.0f,  1.0f, 1.0f,-1.0f,
         1.0f,-1.0f,-1.0f,  1.0f, 1.0f, 1.0f,  1.0f,-1.0f, 1.0f,
         1.0f, 1.0f, 1.0f,  1.0f, 1.0f,-1.0f, -1.0f, 1.0f,-1.0f,
         1.0f, 1.0f, 1.0f, -1.0f, 1.0f,-1.0f, -1.0f, 1.0f, 1.0f,
         1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f,  1.0f,-1.0f, 1.0f
    };

    static const GLfloat kUvData[] = {
        0.000059f, 1.0f-0.000004f, 0.000103f, 1.0f-0.336048f, 0.335973f, 1.0f-0.335903f,
        1.000023f, 1.0f-0.000013f, 0.667979f, 1.0f-0.335851f, 0.999958f, 1.0f-0.336064f,
        0.667979f, 1.0f-0.335851f, 0.336024f, 1.0f-0.671877f, 0.667969f, 1.0f-0.671889f,
        1.000023f, 1.0f-0.000013f, 0.668104f, 1.0f-0.000013f, 0.667979f, 1.0f-0.335851f,
        0.000059f, 1.0f-0.000004f, 0.335973f, 1.0f-0.335903f, 0.336098f, 1.0f-0.000071f,
        0.667979f, 1.0f-0.335851f, 0.335973f, 1.0f-0.335903f, 0.336024f, 1.0f-0.671877f,
        1.000004f, 1.0f-0.671847f, 0.999958f, 1.0f-0.336064f, 0.667979f, 1.0f-0.335851f,
        0.668104f, 1.0f-0.000013f, 0.335973f, 1.0f-0.335903f, 0.667979f, 1.0f-0.335851f,
        0.335973f, 1.0f-0.335903f, 0.668104f, 1.0f-0.000013f, 0.336098f, 1.0f-0.000071f,
        0.000103f, 1.0f-0.336048f, 0.000004f, 1.0f-0.671870f, 0.336024f, 1.0f-0.671877f,
        0.000103f, 1.0f-0.336048f, 0.336024f, 1.0f-0.671877f, 0.335973f, 1.0f-0.335903f,
        0.667969f, 1.0f-0.671889f, 1.000004f, 1.0f-0.671847f, 0.667979f, 1.0f-0.335851f
    };

    GLuint vao = 0;
    GLuint vboPos = 0;
    GLuint vboUv = 0;

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vboPos);
    glBindBuffer(GL_ARRAY_BUFFER, vboPos);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(sizeof(kVertexData)),
                 kVertexData,
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    glGenBuffers(1, &vboUv);
    glBindBuffer(GL_ARRAY_BUFFER, vboUv);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(sizeof(kUvData)),
                 kUvData,
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0) {
        glfwPollEvents();

        int fbW = 0;
        int fbH = 0;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        glViewport(0, 0, fbW, fbH);

        const float t = static_cast<float>(glfwGetTime());
        const float aspect = (fbH > 0) ? (static_cast<float>(fbW) / static_cast<float>(fbH)) : 1.0f;

        const glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
        const glm::mat4 view = glm::lookAt(
            glm::vec3(4.0f, 3.0f, 6.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f));

        const glm::mat4 modelLeft =
            glm::translate(glm::mat4(1.0f), glm::vec3(-1.6f, 0.0f, 0.0f)) *
            glm::rotate(glm::mat4(1.0f), t * 0.9f, glm::vec3(0.0f, 1.0f, 0.0f)) *
            glm::rotate(glm::mat4(1.0f), t * 0.6f, glm::vec3(1.0f, 0.0f, 0.0f));

        const glm::mat4 modelRight =
            glm::translate(glm::mat4(1.0f), glm::vec3(1.6f, 0.0f, 0.0f)) *
            glm::rotate(glm::mat4(1.0f), t * 1.1f, glm::vec3(0.0f, 1.0f, 0.0f)) *
            glm::rotate(glm::mat4(1.0f), t * 0.5f, glm::vec3(1.0f, 0.0f, 0.0f));

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.0f, 0.0f, 0.4f, 1.0f);
        glUseProgram(program);
        glUniform1i(texLoc, 0);
        glBindVertexArray(vao);

        glActiveTexture(GL_TEXTURE0);

        const glm::mat4 mvpLeft = projection * view * modelLeft;
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &mvpLeft[0][0]);
        glBindTexture(GL_TEXTURE_2D, ddsTex);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        const glm::mat4 mvpRight = projection * view * modelRight;
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &mvpRight[0][0]);
        glBindTexture(GL_TEXTURE_2D, bmpTex);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glfwSwapBuffers(window);
    }

    glDeleteBuffers(1, &vboUv);
    glDeleteBuffers(1, &vboPos);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);
    glDeleteTextures(1, &bmpTex);
    glDeleteTextures(1, &ddsTex);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
