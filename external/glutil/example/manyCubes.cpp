#include <glutil/glutil.hpp>
#include <glutil/math.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "config.hpp"

namespace fs = std::filesystem;

constexpr float kBoundaryRadius = 15.0f;
constexpr float kCollisionRadius = 1.0f;
constexpr float kBoundaryBounce = 0.9f;
constexpr float kNormalMapRatio = 0.5f;
constexpr float kMoveSpeed = 2.5f;
constexpr float kRotateSpeed = 90.0f;
constexpr float kInitialSpeedMin = 4.0f;
constexpr float kInitialSpeedMax = 7.0f;

const glutil::VertexPNT kVertices[] = {
    // Front (+Z)
    {-1.0f, -1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    { 1.0f, -1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f},
    { 1.0f,  1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f},
    { 1.0f,  1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f},
    {-1.0f,  1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
    {-1.0f, -1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    // Back (-Z)
    { 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    {-1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f},
    {-1.0f,  1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f},
    {-1.0f,  1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f},
    { 1.0f,  1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
    { 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    // Left (-X)
    {-1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    {-1.0f, -1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f},
    {-1.0f,  1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f},
    {-1.0f,  1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f},
    {-1.0f,  1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
    {-1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    // Right (+X)
    { 1.0f, -1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    { 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f},
    { 1.0f,  1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f},
    { 1.0f,  1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f},
    { 1.0f,  1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
    { 1.0f, -1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    // Top (+Y)
    {-1.0f,  1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    { 1.0f,  1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f},
    { 1.0f,  1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f},
    { 1.0f,  1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f},
    {-1.0f,  1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
    {-1.0f,  1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    // Bottom (-Y)
    {-1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    { 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f},
    { 1.0f, -1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f},
    { 1.0f, -1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f},
    {-1.0f, -1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
    {-1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}
};

static GLFWwindow* initGLFWAndContext();
static GLuint compileShader(GLenum type, const GLchar* src, GLint len);
static GLuint createProgramFromFiles(const fs::path& vsPath, const fs::path& fsPath);
static GLuint uploadStandard2D(const glutil::TextureImage& image);
static GLuint uploadDDS2D(const glutil::TextureDDS& dds);
static float randf(float a, float b);
static std::vector<glm::vec3> generateSpherePositions(int num_cubes,
                                                      const glm::vec3& center,
                                                      float radius,
                                                      float padding);
static std::vector<float> buildColorDataFromFaces();
static std::vector<glm::vec3> computeNormals(const glutil::VertexPNT* vertices, size_t vertexCount);
static std::vector<glm::vec3> computeTangents(
    const glutil::VertexPNT* vertices,
    const std::vector<glm::vec3>& normals);
static std::vector<glm::vec3> computeBitangents(
    const std::vector<glm::vec3>& normals,
    const std::vector<glm::vec3>& tangents);
static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

struct InputState {
    bool w=false, s=false, a=false, d=false;
    bool space=false, shift=false;
    bool left=false, right=false, up=false, down=false;
} g_input;

static std::mt19937 g_rng(12345);

int main(int argc, char** argv) {
    int num_cubes = 150;
    if (argc == 2) {
        num_cubes = std::max(1, std::stoi(argv[1]));
    }

    GLFWwindow* window = initGLFWAndContext();
    if (!window) {
        return 1;
    }
    glfwSetKeyCallback(window, keyCallback);

    const fs::path shaderDir = glutil::EXAMPLE_ASSET_DIR / "shader";
    const fs::path vsPath = shaderDir / "manyCubes.vert";
    const fs::path fsPath = shaderDir / "manyCubes.frag";

    const GLuint program = createProgramFromFiles(vsPath, fsPath);
    if (program == 0) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    GLuint vao = 0;
    GLuint vboVertex = 0;
    GLuint vboNormal = 0;
    GLuint vboTangent = 0;
    GLuint vboBitangent = 0;
    GLuint vboColor = 0;

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vboVertex);
    glBindBuffer(GL_ARRAY_BUFFER, vboVertex);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)sizeof(kVertices), kVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glutil::VertexPNT),
                          reinterpret_cast<void*>(offsetof(glutil::VertexPNT, x)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(glutil::VertexPNT),
                          reinterpret_cast<void*>(offsetof(glutil::VertexPNT, u)));

    const size_t vertexCount = sizeof(kVertices) / sizeof(kVertices[0]);
    const std::vector<glm::vec3> normals = computeNormals(kVertices, vertexCount);
    const std::vector<glm::vec3> tangents = computeTangents(kVertices, normals);
    const std::vector<glm::vec3> bitangents = computeBitangents(normals, tangents);

    glGenBuffers(1, &vboNormal);
    glBindBuffer(GL_ARRAY_BUFFER, vboNormal);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(normals.size() * sizeof(glm::vec3)),
                 normals.data(),
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    glGenBuffers(1, &vboTangent);
    glBindBuffer(GL_ARRAY_BUFFER, vboTangent);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(tangents.size() * sizeof(glm::vec3)),
                 tangents.data(),
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    glGenBuffers(1, &vboBitangent);
    glBindBuffer(GL_ARRAY_BUFFER, vboBitangent);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(bitangents.size() * sizeof(glm::vec3)),
                 bitangents.data(),
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    const std::vector<float> colorData = buildColorDataFromFaces();
    glGenBuffers(1, &vboColor);
    glBindBuffer(GL_ARRAY_BUFFER, vboColor);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(colorData.size() * sizeof(float)),
                 colorData.data(),
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    const fs::path textureDir = glutil::EXAMPLE_ASSET_DIR / "texture";
    const fs::path diffusePath = textureDir / "diffuse.DDS";
    const fs::path normalPath = textureDir / "normal.bmp";
    const fs::path specularPath = textureDir / "specular.DDS";

    GLuint diffuseTex = 0;
    GLuint normalTex = 0;
    GLuint specularTex = 0;

    if (fs::exists(diffusePath)) {
        if (glutil::ImageLoader::isDDS(diffusePath.string().c_str())) {
            glutil::TextureDDS dds = glutil::ImageLoader::loadDDS(diffusePath.string().c_str());
            if (dds.ok) {
                diffuseTex = uploadDDS2D(dds);
            } else {
                std::cerr << "[ManyCubes] Diffuse DDS load failed: " << diffusePath
                          << "\n  reason: " << dds.error << std::endl;
            }
        } else {
            glutil::TextureImage img = glutil::ImageLoader::loadImage(diffusePath.string().c_str());
            if (img.ok) {
                diffuseTex = uploadStandard2D(img);
            } else {
                std::cerr << "[ManyCubes] Diffuse texture load failed: " << diffusePath
                          << "\n  reason: " << img.error << std::endl;
            }
        }
    } else {
        std::cerr << "[ManyCubes] Diffuse texture not found: " << diffusePath << std::endl;
    }

    if (fs::exists(normalPath)) {
        glutil::TextureImage img = glutil::ImageLoader::loadImage(normalPath.string().c_str());
        if (img.ok) {
            normalTex = uploadStandard2D(img);
        } else {
            std::cerr << "[ManyCubes] Normal texture load failed: " << normalPath
                      << "\n  reason: " << img.error << std::endl;
        }
    } else {
        std::cerr << "[ManyCubes] Normal texture not found: " << normalPath << std::endl;
    }

    if (fs::exists(specularPath)) {
        if (glutil::ImageLoader::isDDS(specularPath.string().c_str())) {
            glutil::TextureDDS dds = glutil::ImageLoader::loadDDS(specularPath.string().c_str());
            if (dds.ok) {
                specularTex = uploadDDS2D(dds);
            } else {
                std::cerr << "[ManyCubes] Specular DDS load failed: " << specularPath
                          << "\n  reason: " << dds.error << std::endl;
            }
        } else {
            glutil::TextureImage img = glutil::ImageLoader::loadImage(specularPath.string().c_str());
            if (img.ok) {
                specularTex = uploadStandard2D(img);
            } else {
                std::cerr << "[ManyCubes] Specular texture load failed: " << specularPath
                          << "\n  reason: " << img.error << std::endl;
            }
        }
    } else {
        std::cerr << "[ManyCubes] Specular texture not found: " << specularPath << std::endl;
    }

    const bool normalMapReady = (diffuseTex != 0 && normalTex != 0 && specularTex != 0);
    if (!normalMapReady) {
        std::cerr << "[ManyCubes] Normal mapping textures incomplete. Falling back to vertex colors."
                  << std::endl;
    }

    std::vector<int> cubeUseNormalMap(num_cubes, 0);
    if (normalMapReady) {
        std::bernoulli_distribution useNormalMapDist(kNormalMapRatio);
        for (int i = 0; i < num_cubes; ++i) {
            cubeUseNormalMap[i] = useNormalMapDist(g_rng) ? 1 : 0;
        }
    }

    const GLint modelLoc = glGetUniformLocation(program, "uModel");
    const GLint viewLoc = glGetUniformLocation(program, "uView");
    const GLint projLoc = glGetUniformLocation(program, "uProj");
    const GLint viewPosLoc = glGetUniformLocation(program, "uViewPos");
    const GLint lightPosLoc = glGetUniformLocation(program, "uLightPos");
    const GLint diffuseTexLoc = glGetUniformLocation(program, "uDiffuseTex");
    const GLint normalTexLoc = glGetUniformLocation(program, "uNormalTex");
    const GLint specularTexLoc = glGetUniformLocation(program, "uSpecularTex");
    const GLint useNormalMapLoc = glGetUniformLocation(program, "uUseNormalMap");

    glUseProgram(program);
    if (diffuseTexLoc >= 0) glUniform1i(diffuseTexLoc, 0);
    if (normalTexLoc >= 0) glUniform1i(normalTexLoc, 1);
    if (specularTexLoc >= 0) glUniform1i(specularTexLoc, 2);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    glm::vec3 camPos = kBoundaryRadius * glm::vec3(1.0f) + glm::vec3(1.0f, kBoundaryRadius, 1.0f);
    glm::vec3 camTarget(0.0f, kBoundaryRadius, 0.0f);
    glm::vec3 camUp(0.0f, 1.0f, 0.0f);
    const glm::vec3 initialDir = glm::normalize(camTarget - camPos);
    float cameraYaw = glm::degrees(std::atan2(initialDir.z, initialDir.x));
    float cameraPitch = glm::degrees(std::asin(initialDir.y));
    const glm::vec3 lightPos = kBoundaryRadius * glm::vec3(1.1f);
    float fovY = 45.0f;
    float zNear = 0.1f;
    float zFar = 100.0f;

    int fbW = 0;
    int fbH = 0;
    glfwGetFramebufferSize(window, &fbW, &fbH);
    float aspect = (fbH > 0) ? static_cast<float>(fbW) / static_cast<float>(fbH) : 1.0f;

    const glm::vec3 boundaryCenter(0.0f, kBoundaryRadius, 0.0f);
    auto positions = generateSpherePositions(num_cubes, boundaryCenter, kBoundaryRadius, kCollisionRadius);
    std::vector<glm::vec3> rotations;
    rotations.reserve(num_cubes);
    std::uniform_real_distribution<float> rotDist(0.0f, 3.14159f);
    for (int i = 0; i < num_cubes; ++i) {
        rotations.emplace_back(rotDist(g_rng), rotDist(g_rng), rotDist(g_rng));
    }

    std::vector<glm::vec3> velocities;
    velocities.reserve(num_cubes);
    std::uniform_real_distribution<float> dirDist(-1.0f, 1.0f);
    std::uniform_real_distribution<float> speedDist(kInitialSpeedMin, kInitialSpeedMax);
    for (int i = 0; i < num_cubes; ++i) {
        glm::vec3 dir(dirDist(g_rng), dirDist(g_rng), dirDist(g_rng));
        if (glm::dot(dir, dir) < 1e-6f) {
            dir = glm::vec3(1.0f, 0.0f, 0.0f);
        }
        dir = glm::normalize(dir);
        velocities.emplace_back(dir * speedDist(g_rng));
    }

    auto lastPrint = std::chrono::steady_clock::now();
    unsigned int frameCount = 0;
    double renderTime = 0.0;
    double cpuTime = 0.0;
    double gpuTime = 0.0;
    auto simLastTime = std::chrono::steady_clock::now();

    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && !glfwWindowShouldClose(window)) {
        glfwPollEvents();

        const auto frameBegin = std::chrono::steady_clock::now();

        const auto simNow = frameBegin;
        const double dt = std::chrono::duration<double>(simNow - simLastTime).count();
        const float fdt = static_cast<float>(std::min(0.05, dt * 1.0));
        simLastTime = simNow;

        if (g_input.left) cameraYaw -= kRotateSpeed * fdt;
        if (g_input.right) cameraYaw += kRotateSpeed * fdt;
        if (g_input.up) cameraPitch += kRotateSpeed * fdt;
        if (g_input.down) cameraPitch -= kRotateSpeed * fdt;
        cameraPitch = glm::clamp(cameraPitch, -89.0f, 89.0f);

        glm::vec3 cameraFront;
        cameraFront.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
        cameraFront.y = sin(glm::radians(cameraPitch));
        cameraFront.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
        cameraFront = glm::normalize(cameraFront);
        glm::vec3 cameraRight = glm::normalize(glm::cross(cameraFront, camUp));

        if (g_input.w) camPos += cameraFront * kMoveSpeed * fdt;
        if (g_input.s) camPos -= cameraFront * kMoveSpeed * fdt;
        if (g_input.a) camPos -= cameraRight * kMoveSpeed * fdt;
        if (g_input.d) camPos += cameraRight * kMoveSpeed * fdt;
        if (g_input.space) camPos += glm::vec3(0.0f, 1.0f, 0.0f) * kMoveSpeed * fdt;
        if (g_input.shift) camPos -= glm::vec3(0.0f, 1.0f, 0.0f) * kMoveSpeed * fdt;

        for (int i = 0; i < num_cubes; ++i) {
            positions[i] += velocities[i] * fdt;
        }

        const float minDist = kCollisionRadius;
        const float minDist2 = minDist * minDist;
        for (int i = 0; i < num_cubes; ++i) {
            for (int j = i + 1; j < num_cubes; ++j) {
                glm::vec3 d = positions[j] - positions[i];
                float dist2 = glm::dot(d, d);
                if (dist2 < minDist2) {
                    float dist = std::sqrt(dist2) + 1e-6f;
                    glm::vec3 dir = d / dist;
                    float overlap = 0.5f * (minDist - dist);
                    positions[i] -= dir * overlap;
                    positions[j] += dir * overlap;
                    const float impulse = 20.0f;
                    velocities[i] -= dir * impulse * fdt;
                    velocities[j] += dir * impulse * fdt;
                }
            }
        }

        for (int i = 0; i < num_cubes; ++i) {
            const float effectiveRadius = kBoundaryRadius - kCollisionRadius;
            glm::vec3 toCenter = positions[i] - boundaryCenter;
            const float dist2 = glm::dot(toCenter, toCenter);
            if (dist2 > effectiveRadius * effectiveRadius) {
                float dist = std::sqrt(dist2);
                if (dist < 1e-6f) {
                    toCenter = glm::vec3(1.0f, 0.0f, 0.0f);
                    dist = 1.0f;
                }
                glm::vec3 n = toCenter / dist;
                positions[i] = boundaryCenter + n * effectiveRadius;
                velocities[i] = (velocities[i] - 2.0f * glm::dot(velocities[i], n) * n) * kBoundaryBounce;
            }
        }

        for (int i = 0; i < num_cubes; ++i) {
            if (i & 1) rotations[i].y += randf(0.007f, 0.013f);
            else rotations[i].y -= randf(0.007f, 0.013f);
            rotations[i].x += randf(0.012f, 0.018f);
        }

        const auto cpuEnd = std::chrono::steady_clock::now();

        glfwGetFramebufferSize(window, &fbW, &fbH);
        if (fbW > 0 && fbH > 0) {
            glViewport(0, 0, fbW, fbH);
            aspect = static_cast<float>(fbW) / static_cast<float>(fbH);
        }

        const glm::mat4 proj = glm::perspective(glm::radians(fovY), aspect, zNear, zFar);
        if (projLoc >= 0) {
            glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));
        }

        glm::mat4 view = glm::lookAt(camPos, camPos + cameraFront, camUp);
        if (viewLoc >= 0) {
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        }

        glClearColor(0.0f, 0.0f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program);
        if (viewPosLoc >= 0) glUniform3fv(viewPosLoc, 1, glm::value_ptr(camPos));
        if (lightPosLoc >= 0) glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));

        if (normalMapReady) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, diffuseTex);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, normalTex);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, specularTex);
        }

        for (int i = 0; i < num_cubes; ++i) {
            glm::mat4 model(1.0f);
            model = glm::translate(model, positions[i]);
            model = glm::rotate(model, rotations[i].x, glm::vec3(1, 0, 0));
            model = glm::rotate(model, rotations[i].y, glm::vec3(0, 1, 0));
            model = glm::rotate(model, rotations[i].z, glm::vec3(0, 0, 1));
            model = glm::scale(model, glm::vec3(0.5f));

            if (modelLoc >= 0) {
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            }

            if (useNormalMapLoc >= 0) {
                const int useNormal = (normalMapReady && cubeUseNormalMap[i] != 0) ? 1 : 0;
                glUniform1i(useNormalMapLoc, useNormal);
            }

            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        const auto renderEnd = std::chrono::steady_clock::now();

        glfwSwapBuffers(window);

        const std::chrono::duration<double> cpuElapsed = cpuEnd - frameBegin;
        const std::chrono::duration<double> gpuElapsed = renderEnd - cpuEnd;
        const std::chrono::duration<double> renderElapsed = cpuElapsed + gpuElapsed;

        renderTime += renderElapsed.count();
        cpuTime += cpuElapsed.count();
        gpuTime += gpuElapsed.count();
        frameCount++;

        const auto printNow = std::chrono::steady_clock::now();
        const std::chrono::duration<double> printElapsed = printNow - lastPrint;
        if (printElapsed.count() >= 5.0) {
            const double frameTimeAvgMs = (printElapsed.count() / static_cast<double>(frameCount)) * 1000.0;
            const double renderTimeAvgMs = (renderTime / static_cast<double>(frameCount)) * 1000.0;
            const double cpuTimeAvgMs = (cpuTime / static_cast<double>(frameCount)) * 1000.0;
            const double gpuTimeAvgMs = (gpuTime / static_cast<double>(frameCount)) * 1000.0;

            std::cout << "[FPS] " << static_cast<int>(frameCount / printElapsed.count())
                      << ", [Render Time / Frame Time] "
                      << std::fixed << std::setprecision(4)
                      << renderTimeAvgMs << " / " << frameTimeAvgMs << " ms ("
                      << std::setprecision(2)
                      << (100.0 * renderTimeAvgMs / frameTimeAvgMs) << "%), [CPU / GPU] "
                      << std::setprecision(4)
                      << cpuTimeAvgMs << " / " << gpuTimeAvgMs << " ms"
                      << std::endl;

            lastPrint = printNow;
            frameCount = 0;
            renderTime = 0.0;
            cpuTime = 0.0;
            gpuTime = 0.0;
        }
    }

    glDeleteBuffers(1, &vboBitangent);
    glDeleteBuffers(1, &vboTangent);
    glDeleteBuffers(1, &vboNormal);
    glDeleteBuffers(1, &vboColor);
    glDeleteBuffers(1, &vboVertex);
    glDeleteVertexArrays(1, &vao);

    if (diffuseTex != 0) glDeleteTextures(1, &diffuseTex);
    if (normalTex != 0) glDeleteTextures(1, &normalTex);
    if (specularTex != 0) glDeleteTextures(1, &specularTex);

    glDeleteProgram(program);
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
    glfwWindowHint(GLFW_SAMPLES, 4);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* ret = glfwCreateWindow(1280, 720, "glutil many cubes example", nullptr, nullptr);
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

    glfwSwapInterval(1);
    glfwSetInputMode(ret, GLFW_STICKY_KEYS, GL_TRUE);
    return ret;
}

static GLuint compileShader(GLenum type, const GLchar* src, GLint len) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, &len);
    glCompileShader(shader);

    const auto r = glutil::Inspector::shaderCompileResult(shader);
    if (r.ok) {
        return shader;
    }

    std::cerr << "Shader compile failed:\n" << r.message << std::endl;
    glDeleteShader(shader);
    return 0;
}

static GLuint createProgramFromFiles(const fs::path& vsPath, const fs::path& fsPath) {
    glutil::ShaderLoadResult vsSrc = glutil::ShaderLoader::loadFile(vsPath.string().c_str());
    if (!vsSrc.ok) {
        std::cerr << "Vertex shader load failed: " << vsPath << "\n  reason: " << vsSrc.error << std::endl;
        return 0;
    }

    glutil::ShaderLoadResult fsSrc = glutil::ShaderLoader::loadFile(fsPath.string().c_str());
    if (!fsSrc.ok) {
        std::cerr << "Fragment shader load failed: " << fsPath << "\n  reason: " << fsSrc.error << std::endl;
        return 0;
    }

    const GLuint vs = compileShader(GL_VERTEX_SHADER, *vsSrc.string(), vsSrc.length());
    const GLuint fs = compileShader(GL_FRAGMENT_SHADER, *fsSrc.string(), fsSrc.length());
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
    if (r.ok) {
        return program;
    }

    std::cerr << "Program link failed:\n" << r.message << std::endl;
    glDeleteProgram(program);
    return 0;
}

static GLuint uploadStandard2D(const glutil::TextureImage& image) {
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
                 image.internalFormat(),
                 image.width(),
                 image.height(),
                 0,
                 image.format(),
                 GL_UNSIGNED_BYTE,
                 image.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

static GLuint uploadDDS2D(const glutil::TextureDDS& dds) {
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
        glCompressedTexImage2D(GL_TEXTURE_2D,
                               (GLint)level,
                               dds.format(),
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

static float randf(float a, float b) {
    std::uniform_real_distribution<float> dist(a, b);
    return dist(g_rng);
}

static std::vector<glm::vec3> generateSpherePositions(int num_cubes,
                                                      const glm::vec3& center,
                                                      float radius,
                                                      float padding) {
    std::vector<glm::vec3> positions;
    positions.reserve(num_cubes);

    const float usableRadius = std::max(0.0f, radius - padding);
    for (int i = 0; i < num_cubes; ++i) {
        glm::vec3 dir(randf(-1.0f, 1.0f), randf(-1.0f, 1.0f), randf(-1.0f, 1.0f));
        if (glm::dot(dir, dir) < 1e-6f) {
            dir = glm::vec3(1.0f, 0.0f, 0.0f);
        }
        dir = glm::normalize(dir);
        const float r = usableRadius * std::cbrt(randf(0.0f, 1.0f));
        positions.emplace_back(center + dir * r);
    }

    return positions;
}

static std::vector<float> buildColorDataFromFaces() {
    std::vector<float> colors;
    colors.resize(36 * 3);

    auto setColor = [&colors](int idx, float r, float g, float b) {
        const int base = idx * 3;
        colors[base + 0] = r;
        colors[base + 1] = g;
        colors[base + 2] = b;
    };

    auto pickColor = [](float x, float y, float z, char axis) -> glm::vec3 {
        if (axis == 'x' && x < 0.0f) {
            if (y < 0.0f && z < 0.0f) return {0.0f, 0.0f, 0.0f};
            if (y < 0.0f && z > 0.0f) return {0.0f, 1.0f, 0.0f};
            if (y > 0.0f && z > 0.0f) return {1.0f, 1.0f, 0.0f};
            return {1.0f, 0.0f, 0.0f};
        }
        if (axis == 'x' && x > 0.0f) {
            if (y < 0.0f && z < 0.0f) return {0.0f, 0.0f, 1.0f};
            if (y > 0.0f && z < 0.0f) return {1.0f, 0.0f, 1.0f};
            if (y > 0.0f && z > 0.0f) return {1.0f, 1.0f, 1.0f};
            return {0.0f, 1.0f, 0.0f};
        }
        if (axis == 'y' && y > 0.0f) {
            if (x < 0.0f && z < 0.0f) return {1.0f, 0.0f, 0.0f};
            if (x < 0.0f && z > 0.0f) return {1.0f, 1.0f, 0.0f};
            if (x > 0.0f && z > 0.0f) return {1.0f, 1.0f, 1.0f};
            return {1.0f, 0.0f, 1.0f};
        }
        if (axis == 'y' && y < 0.0f) {
            if (x < 0.0f && z < 0.0f) return {0.0f, 0.0f, 0.0f};
            if (x > 0.0f && z < 0.0f) return {0.0f, 0.0f, 1.0f};
            if (x > 0.0f && z > 0.0f) return {0.0f, 1.0f, 0.0f};
            return {0.0f, 1.0f, 0.0f};
        }
        if (axis == 'z' && z > 0.0f) {
            if (x < 0.0f && y < 0.0f) return {0.0f, 1.0f, 0.0f};
            if (x > 0.0f && y < 0.0f) return {0.0f, 1.0f, 0.0f};
            if (x > 0.0f && y > 0.0f) return {1.0f, 1.0f, 1.0f};
            return {1.0f, 1.0f, 0.0f};
        }
        // z < 0.0f
        if (x < 0.0f && y < 0.0f) return {0.0f, 0.0f, 0.0f};
        if (x < 0.0f && y > 0.0f) return {1.0f, 0.0f, 0.0f};
        if (x > 0.0f && y > 0.0f) return {1.0f, 0.0f, 1.0f};
        return {0.0f, 0.0f, 1.0f};
    };

    for (int tri = 0; tri < 12; ++tri) {
        const int vtx = tri * 3;
        const glutil::VertexPNT& v0 = kVertices[vtx + 0];
        const glutil::VertexPNT& v1 = kVertices[vtx + 1];
        const glutil::VertexPNT& v2 = kVertices[vtx + 2];

        const float x0 = v0.x;
        const float y0 = v0.y;
        const float z0 = v0.z;
        const float x1 = v1.x;
        const float y1 = v1.y;
        const float z1 = v1.z;
        const float x2 = v2.x;
        const float y2 = v2.y;
        const float z2 = v2.z;

        char axis = 'z';
        if (x0 == x1 && x1 == x2) axis = 'x';
        else if (y0 == y1 && y1 == y2) axis = 'y';

        const glm::vec3 c0 = pickColor(x0, y0, z0, axis);
        const glm::vec3 c1 = pickColor(x1, y1, z1, axis);
        const glm::vec3 c2 = pickColor(x2, y2, z2, axis);

        setColor(vtx + 0, c0.r, c0.g, c0.b);
        setColor(vtx + 1, c1.r, c1.g, c1.b);
        setColor(vtx + 2, c2.r, c2.g, c2.b);
    }

    return colors;
}

static std::vector<glm::vec3> computeNormals(const glutil::VertexPNT* vertices, size_t vertexCount) {
    std::vector<glm::vec3> normals(vertexCount, glm::vec3(0.0f));
    for (size_t i = 0; i < vertexCount; i += 3) {
        glm::vec3 p0 = glutil::position(vertices[i + 0]);
        glm::vec3 p1 = glutil::position(vertices[i + 1]);
        glm::vec3 p2 = glutil::position(vertices[i + 2]);

        glm::vec3 edge1 = p1 - p0;
        glm::vec3 edge2 = p2 - p0;
        glm::vec3 faceNormal = glm::normalize(glm::cross(edge1, edge2));

        normals[i + 0] = faceNormal;
        normals[i + 1] = faceNormal;
        normals[i + 2] = faceNormal;
    }
    return normals;
}

static std::vector<glm::vec3> computeTangents(
    const glutil::VertexPNT* vertices,
    const std::vector<glm::vec3>& normals) {
    std::vector<glm::vec3> tangents(normals.size(), glm::vec3(0.0f));
    for (size_t i = 0; i < normals.size(); i += 3) {
        const glutil::VertexPNT& v0 = vertices[i + 0];
        const glutil::VertexPNT& v1 = vertices[i + 1];
        const glutil::VertexPNT& v2 = vertices[i + 2];

        glm::vec3 p0 = glutil::position(v0);
        glm::vec3 p1 = glutil::position(v1);
        glm::vec3 p2 = glutil::position(v2);

        glm::vec2 uv0 = glutil::uv(v0);
        glm::vec2 uv1 = glutil::uv(v1);
        glm::vec2 uv2 = glutil::uv(v2);

        glm::vec3 edge1 = p1 - p0;
        glm::vec3 edge2 = p2 - p0;
        glm::vec2 dUV1 = uv1 - uv0;
        glm::vec2 dUV2 = uv2 - uv0;

        float r = 1.0f / (dUV1.x * dUV2.y - dUV1.y * dUV2.x + 1e-6f);
        glm::vec3 tangent = (edge1 * dUV2.y - edge2 * dUV1.y) * r;

        tangents[i + 0] = glm::normalize(tangent);
        tangents[i + 1] = glm::normalize(tangent);
        tangents[i + 2] = glm::normalize(tangent);
    }
    return tangents;
}

static std::vector<glm::vec3> computeBitangents(
    const std::vector<glm::vec3>& normals,
    const std::vector<glm::vec3>& tangents) {
    std::vector<glm::vec3> bitangents(normals.size());
    for (size_t i = 0; i < normals.size(); ++i) {
        bitangents[i] = glm::normalize(glm::cross(normals[i], tangents[i]));
    }
    return bitangents;
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
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
