#include <glutil/gl.hpp>
#include <glutil/inspector.hpp>
#include <glutil/texture.hpp>
#include <glutil/model.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <filesystem>
#include <iostream>
#include <string>

#include "config.hpp"

GLFWwindow* initGLFWAndContext();
GLuint compileShader(GLenum type, const char* source);
GLuint createProgram(const char* vertexSource, const char* fragmentSource);
GLuint uploadStandard2D(const glutil::TextureImage& image);
glm::mat4 makeMvp(float t, float aspect);
GLuint uploadDDS2D(const glutil::TextureDDS& dds);

constexpr const char* kVS = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

uniform mat4 uMVP;
out vec2 vUV;
out vec3 vNormal;

void main() {
    vUV     = aUV;
    vNormal = aNormal;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

constexpr const char* kFS = R"(
#version 330 core
in vec2  vUV;
in vec3  vNormal;

uniform sampler2D uTexture;
uniform int       uHasTexture;

out vec4 FragColor;

void main() {
    if (uHasTexture == 1)
        FragColor = texture(uTexture, vUV);
    else
        FragColor = vec4(vNormal * 0.5 + 0.5, 1.0);
}
)";

int main()
{
    GLFWwindow* ctx;
    if (!(ctx = initGLFWAndContext()))
        return 1;

    const std::filesystem::path objPath = glutil::EXAMPLE_ASSET_DIR / "model" / "cube.obj";

    glutil::ModelData model = glutil::ModelLoader::loadOBJ(objPath.string().c_str());

    if (!model.warn.empty())
        std::cerr << "Model load warning: " << model.warn << std::endl;

    if (!model.ok) {
        std::cerr << "Model load failed: " << objPath
                  << "\n  reason: " << model.error << std::endl;
        glfwDestroyWindow(ctx);
        glfwTerminate();
        return 1;
    }

    std::cout << "[ModelExample] Loaded " << model.meshes.size() << " mesh(es) from "
              << objPath.filename() << std::endl;

    const GLuint program = createProgram(kVS, kFS);
    if (program == 0) {
        glfwDestroyWindow(ctx);
        glfwTerminate();
        return 1;
    }

    const GLint mvpLoc        = glGetUniformLocation(program, "uMVP");
    const GLint texLoc        = glGetUniformLocation(program, "uTexture");
    const GLint hasTextureLoc = glGetUniformLocation(program, "uHasTexture");

    struct GpuMesh {
        GLuint vao = 0;
        GLuint vbo = 0;
        GLuint ebo = 0;
        GLuint tex = 0;
        GLsizei indexCount = 0;
    };

    std::vector<GpuMesh> gpuMeshes;
    gpuMeshes.reserve(model.meshes.size());

    for (const glutil::MeshData& mesh : model.meshes) {


        GpuMesh gm;
        gm.indexCount = static_cast<GLsizei>(mesh.indexCount());

        glGenVertexArrays(1, &gm.vao);
        glBindVertexArray(gm.vao);

        glGenBuffers(1, &gm.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, gm.vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(mesh.vertexCount() * sizeof(glutil::VertexPNT)),
                     mesh.vertexData(),
                     GL_STATIC_DRAW);

        glGenBuffers(1, &gm.ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gm.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(mesh.indexCount() * sizeof(unsigned int)),
                     mesh.indexData(),
                     GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                              sizeof(glutil::VertexPNT),
                              (void*)offsetof(glutil::VertexPNT, x));

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                              sizeof(glutil::VertexPNT),
                              (void*)offsetof(glutil::VertexPNT, nx));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
                              sizeof(glutil::VertexPNT),
                              (void*)offsetof(glutil::VertexPNT, u));

        glBindVertexArray(0);

        if (!mesh.diffuseTexturePath.empty()) {
            if (glutil::ImageLoader::isDDS(mesh.diffuseTexturePath.c_str())) {
                glutil::TextureDDS dds = glutil::ImageLoader::loadDDS(mesh.diffuseTexturePath.c_str());
                if (dds.ok) {
                    gm.tex = uploadDDS2D(dds);
                    std::cout << "[ModelExample] mesh \"" << mesh.name << "\": DDS texture loaded" << std::endl;
                } else {
                    std::cerr << "[ModelExample] mesh \"" << mesh.name << "\": DDS load failed: " << dds.error << std::endl;
                }
            } else {
                glutil::TextureImage img = glutil::ImageLoader::loadImage(mesh.diffuseTexturePath.c_str());
                if (img.ok) {
                    gm.tex = uploadStandard2D(img);
                    std::cout << "[ModelExample] mesh \"" << mesh.name << "\": texture loaded" << std::endl;
                } else {
                    std::cerr << "[ModelExample] mesh \"" << mesh.name << "\": texture load failed: " << img.error << std::endl;
                }
            }
        }

        gpuMeshes.push_back(gm);

    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    while (glfwGetKey(ctx, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
           glfwWindowShouldClose(ctx) == 0)
    {
        glfwPollEvents();

        int fbW = 0, fbH = 0;
        glfwGetFramebufferSize(ctx, &fbW, &fbH);
        glViewport(0, 0, fbW, fbH);

        const float t      = static_cast<float>(glfwGetTime());
        const float aspect = (fbH > 0) ? (static_cast<float>(fbW) /
                                          static_cast<float>(fbH)) : 1.0f;

        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program);
        glUniform1i(texLoc, 0);

        const glm::mat4 mvp = makeMvp(t, aspect);
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &mvp[0][0]);

        for (const GpuMesh& gm : gpuMeshes) {
            glUniform1i(hasTextureLoc, gm.tex != 0 ? 1 : 0);
            if (gm.tex != 0) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, gm.tex);
            }
            glBindVertexArray(gm.vao);
            glDrawElements(GL_TRIANGLES, gm.indexCount, GL_UNSIGNED_INT, nullptr);
        }

        glfwSwapBuffers(ctx);
    }

    for (GpuMesh& gm : gpuMeshes) {
        if (gm.tex != 0) glDeleteTextures(1, &gm.tex);
        glDeleteBuffers(1, &gm.ebo);
        glDeleteBuffers(1, &gm.vbo);
        glDeleteVertexArrays(1, &gm.vao);
    }

    glDeleteProgram(program);
    glfwDestroyWindow(ctx);
    glfwTerminate();
    return 0;
}

GLFWwindow* initGLFWAndContext()
{
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

    GLFWwindow* ret = glfwCreateWindow(1024, 768, "glutil model example", nullptr, nullptr);
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

    glfwSetInputMode(ret, GLFW_STICKY_KEYS, GL_TRUE);
    return ret;
}

GLuint compileShader(GLenum type, const char* source)
{
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    const auto r = glutil::Inspector::shaderCompileResult(shader);
    if (r.ok)
        return shader;

    std::cerr << "Shader compile failed:\n" << r.message << std::endl;
    glDeleteShader(shader);
    return 0;
}

GLuint createProgram(const char* vertexSource, const char* fragmentSource)
{
    const GLuint vs = compileShader(GL_VERTEX_SHADER,   vertexSource);
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

    const auto r = glutil::Inspector::programLinkResult(program);
    if (r.ok)
        return program;

    std::cerr << "Program link failed:\n" << r.message << std::endl;
    glDeleteProgram(program);
    return 0;
}

GLuint uploadStandard2D(const glutil::TextureImage& image)
{
    if (!image.ok || image.data() == nullptr)
        return 0;

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0,
                 image.internalFormat(),
                 image.width(), image.height(), 0,
                 image.format(), GL_UNSIGNED_BYTE,
                 image.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}
GLuint uploadDDS2D(const glutil::TextureDDS& dds) {
    if (!dds.ok || dds.data() == nullptr || dds.mips().empty())
        return 0;

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, (GLint)(dds.mips().size() - 1));

    for (size_t level = 0; level < dds.mips().size(); ++level) {
        const glutil::MipLevel& m = dds.mips()[level];
        glCompressedTexImage2D(GL_TEXTURE_2D, (GLint)level, dds.format(), m.width, m.height, 0, m.size,
                               dds.data() + m.offset);

        const GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "DDS glCompressedTexImage2D failed at mip " << level << " (glError=0x" << std::hex << err
                      << std::dec << ")" << std::endl;
            glBindTexture(GL_TEXTURE_2D, 0);
            glDeleteTextures(1, &tex);
            return 0;
        }
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}
glm::mat4 makeMvp(float t, float aspect)
{
    const glm::mat4 proj  = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    const glm::mat4 view  = glm::lookAt(
        glm::vec3(4.0f, 3.0f, 6.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::mat4 model =
        glm::rotate(glm::mat4(1.0f), t * 0.9f, glm::vec3(0.0f, 1.0f, 0.0f)) *
        glm::rotate(glm::mat4(1.0f), t * 0.5f, glm::vec3(1.0f, 0.0f, 0.0f));
    return proj * view * model;
}
