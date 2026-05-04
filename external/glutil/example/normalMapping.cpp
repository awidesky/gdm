/** 
 * Normal Mapping Example
 * 
 * This example demonstrates tangent-space normal mapping with specular lighting.
 * It uses three textures: diffuse color, normal map, and specular intensity.
 * 
 * The normal map encodes surface details in tangent space, allowing high-frequency
 * detail without increasing mesh complexity. TBN (tangent-bitangent-normal) basis
 * is computed from mesh UV gradients and used to transform normals to world space.
 */

#include <glutil/gl.hpp>
#include <glutil/inspector.hpp>
#include <glutil/math.hpp>
#include <glutil/texture.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "config.hpp"

namespace fs = std::filesystem;

// Forward declarations
static GLFWwindow* initGLFWAndContext();
static GLuint compileShader(GLenum type, const char* source);
static GLuint createProgram(const char* vertexSource, const char* fragmentSource);
static GLuint uploadStandard2D(const glutil::TextureImage& image);
static GLuint uploadDDS2D(const glutil::TextureDDS& dds);
static glm::mat4 makeMVP(float t, float aspect);
// Input handling (portable via GLFW callback)
static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

struct InputState {
    bool w=false, s=false, a=false, d=false;
    bool space=false, shift=false;
    bool left=false, right=false, up=false, down=false;
} g_input;

// Cube mesh vertex data - grouped by face, whole image per face
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

// Compute vertex normals for a cube mesh
std::vector<glm::vec3> computeNormals(const glutil::VertexPNT* vertices, size_t vertexCount) {
    std::vector<glm::vec3> normals(vertexCount, glm::vec3(0.0f));
    // 36 vertices = 12 triangles
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

// Compute tangent and bitangent vectors from position and UV data using Lengyel's method
std::vector<glm::vec3> computeTangents(
    const glutil::VertexPNT* vertices, const std::vector<glm::vec3>& normals) {
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

// Compute bitangent from normal and tangent
std::vector<glm::vec3> computeBitangents(
    const std::vector<glm::vec3>& normals, const std::vector<glm::vec3>& tangents) {
    std::vector<glm::vec3> bitangents(normals.size());
    for (size_t i = 0; i < normals.size(); ++i) {
        bitangents[i] = glm::normalize(glm::cross(normals[i], tangents[i]));
    }
    return bitangents;
}

// Vertex shader: transforms to world space, computes TBN basis in camera space
const char* kVS = R"(
#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

uniform mat4 uMVP;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat3 uNormalMat;  // inverse(transpose(mat3(uModel)))
uniform vec3 uLightPos;

out vec2 vUV;
out vec3 vPos_world;
out vec3 vLightDir_tangent;
out vec3 vEyeDir_tangent;

void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vUV = aUV;
    
    // Position in world space
    vec3 pos_world = (uModel * vec4(aPos, 1.0)).xyz;
    vPos_world = pos_world;
    
    // Transform TBN to world space
    vec3 normal_world = normalize(uNormalMat * aNormal);
    vec3 tangent_world = normalize(uNormalMat * aTangent);
    vec3 bitangent_world = normalize(uNormalMat * aBitangent);
    
    // Build TBN matrix (transforms from world to tangent space)
    mat3 TBN = transpose(mat3(tangent_world, bitangent_world, normal_world));
    
    // Light direction in tangent space
    vec3 lightDir = normalize(uLightPos - pos_world);
    vLightDir_tangent = TBN * lightDir;
    
    // Eye direction in tangent space (camera is at origin in view space)
    vec3 eyeDir = normalize(-pos_world);  // Approximate: camera far away
    vEyeDir_tangent = TBN * eyeDir;
}
)";

// Fragment shader: samples normal map, applies Phong lighting with specular map
const char* kFS = R"(
#version 330 core

in vec2 vUV;
in vec3 vPos_world;
in vec3 vLightDir_tangent;
in vec3 vEyeDir_tangent;

uniform sampler2D uDiffuseTex;
uniform sampler2D uNormalTex;
uniform sampler2D uSpecularTex;
uniform vec3 uLightPos;
uniform vec3 uViewPos;
uniform vec3 uLightColor;
uniform float uLightPower;

out vec4 FragColor;

void main() {
    // Sample textures
    vec3 diffuseColor = texture(uDiffuseTex, vUV).rgb;
    vec3 normalMap = texture(uNormalTex, vUV).rgb;
    vec3 specularColor = texture(uSpecularTex, vUV).rgb;
    
    // Decode normal from map (convert from [0,1] to [-1,1])
    vec3 normal_tangent = normalize(normalMap * 2.0 - 1.0);
    
    // Normalize interpolated directions
    vec3 lightDir = normalize(vLightDir_tangent);
    vec3 eyeDir = normalize(vEyeDir_tangent);
    
    // Diffuse component (Lambertian)
    float diffuse = max(dot(normal_tangent, lightDir), 0.0);
    
    // Specular component (Blinn-Phong)
    vec3 halfDir = normalize(lightDir + eyeDir);
    float specular = pow(max(dot(normal_tangent, halfDir), 0.0), 32.0);
    
    // Distance attenuation (approximate)
    float dist = length(uLightPos - vPos_world);
    float attenuation = 1.0 / (1.0 + 0.1 * dist + 0.02 * dist * dist);
    
    // Combine lighting with light color and power
    vec3 ambient = 0.1 * diffuseColor * uLightColor;
    vec3 diffuseLight = diffuseColor * diffuse * attenuation * uLightPower * uLightColor;
    vec3 specularLight = specularColor * specular * attenuation * uLightPower * uLightColor;
    
    FragColor = vec4(ambient + diffuseLight + specularLight, 1.0);
}
)";

int main() {
    GLFWwindow* window = initGLFWAndContext();
    if (!window) return 1;
    // register portable input callback
    glfwSetKeyCallback(window, keyCallback);

    const fs::path textureDir = glutil::EXAMPLE_ASSET_DIR / "texture";
    const fs::path diffusePath = textureDir / "diffuse.DDS";
    const fs::path normalPath = textureDir / "normal.bmp";
    const fs::path specularPath = textureDir / "specular.DDS";

    // Load diffuse texture
    GLuint diffuseTex = 0;
    if (fs::exists(diffusePath)) {
        if (glutil::ImageLoader::isDDS(diffusePath.string().c_str())) {
            glutil::TextureDDS dds = glutil::ImageLoader::loadDDS(diffusePath.string().c_str());
            if (dds.ok) {
                diffuseTex = uploadDDS2D(dds);
            } else {
                std::cerr << "Diffuse DDS load failed: " << diffusePath << "\n  reason: " << dds.error << std::endl;
            }
        } else {
            glutil::TextureImage img = glutil::ImageLoader::loadImage(diffusePath.string().c_str());
            if (img.ok) {
                diffuseTex = uploadStandard2D(img);
            } else {
                std::cerr << "Diffuse texture load failed: " << diffusePath << "\n  reason: " << img.error << std::endl;
            }
        }
    } else {
        std::cerr << "Diffuse texture not found: " << diffusePath << std::endl;
    }

    // Load normal texture
    GLuint normalTex = 0;
    if (fs::exists(normalPath)) {
        glutil::TextureImage img = glutil::ImageLoader::loadImage(normalPath.string().c_str());
        if (img.ok) {
            normalTex = uploadStandard2D(img);
        } else {
            std::cerr << "Normal texture load failed: " << normalPath << "\n  reason: " << img.error << std::endl;
        }
    } else {
        std::cerr << "Normal texture not found: " << normalPath << std::endl;
    }

    // Load specular texture
    GLuint specularTex = 0;
    if (fs::exists(specularPath)) {
        if (glutil::ImageLoader::isDDS(specularPath.string().c_str())) {
            glutil::TextureDDS dds = glutil::ImageLoader::loadDDS(specularPath.string().c_str());
            if (dds.ok) {
                specularTex = uploadDDS2D(dds);
            } else {
                std::cerr << "Specular DDS load failed: " << specularPath << "\n  reason: " << dds.error << std::endl;
            }
        } else {
            glutil::TextureImage img = glutil::ImageLoader::loadImage(specularPath.string().c_str());
            if (img.ok) {
                specularTex = uploadStandard2D(img);
            } else {
                std::cerr << "Specular texture load failed: " << specularPath << "\n  reason: " << img.error << std::endl;
            }
        }
    } else {
        std::cerr << "Specular texture not found: " << specularPath << std::endl;
    }

    if (diffuseTex == 0 || normalTex == 0 || specularTex == 0) {
        std::cerr << "Some textures failed to load (diffuse=" << diffuseTex << ", normal=" << normalTex
                  << ", specular=" << specularTex << ")" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    // Create shader program
    const GLuint program = createProgram(kVS, kFS);
    if (program == 0) {
        glDeleteTextures(1, &diffuseTex);
        glDeleteTextures(1, &normalTex);
        glDeleteTextures(1, &specularTex);
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    // Compute normals, tangents, bitangents
    const size_t vertexCount = sizeof(kVertices) / sizeof(kVertices[0]);
    std::vector<glm::vec3> normals = computeNormals(kVertices, vertexCount);
    std::vector<glm::vec3> tangents = computeTangents(kVertices, normals);
    std::vector<glm::vec3> bitangents = computeBitangents(normals, tangents);

    // Create VAO and VBOs
    GLuint vao = 0;
    GLuint vboVertex = 0;
    GLuint vboNormal = 0;
    GLuint vboTangent = 0;
    GLuint vboBitangent = 0;

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Position
    glGenBuffers(1, &vboVertex);
    glBindBuffer(GL_ARRAY_BUFFER, vboVertex);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kVertices), kVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glutil::VertexPNT),
                          reinterpret_cast<void*>(offsetof(glutil::VertexPNT, x)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(glutil::VertexPNT),
                          reinterpret_cast<void*>(offsetof(glutil::VertexPNT, u)));

    // Normal
    glGenBuffers(1, &vboNormal);
    glBindBuffer(GL_ARRAY_BUFFER, vboNormal);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(normals.size() * sizeof(glm::vec3)), normals.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Tangent
    glGenBuffers(1, &vboTangent);
    glBindBuffer(GL_ARRAY_BUFFER, vboTangent);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(tangents.size() * sizeof(glm::vec3)), tangents.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Bitangent
    glGenBuffers(1, &vboBitangent);
    glBindBuffer(GL_ARRAY_BUFFER, vboBitangent);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(bitangents.size() * sizeof(glm::vec3)), bitangents.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Get uniform locations
    const GLint mvpLoc = glGetUniformLocation(program, "uMVP");
    const GLint modelLoc = glGetUniformLocation(program, "uModel");
    const GLint viewLoc = glGetUniformLocation(program, "uView");
    const GLint normalMatLoc = glGetUniformLocation(program, "uNormalMat");
    const GLint lightPosLoc = glGetUniformLocation(program, "uLightPos");
    const GLint viewPosLoc = glGetUniformLocation(program, "uViewPos");
    const GLint diffuseTexLoc = glGetUniformLocation(program, "uDiffuseTex");
    const GLint normalTexLoc = glGetUniformLocation(program, "uNormalTex");
    const GLint specularTexLoc = glGetUniformLocation(program, "uSpecularTex");
    const GLint lightColorLoc = glGetUniformLocation(program, "uLightColor");
    const GLint lightPowerLoc = glGetUniformLocation(program, "uLightPower");

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Set a brighter light color and power for visibility
    glUseProgram(program);
    if (lightColorLoc >= 0) glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);
    if (lightPowerLoc >= 0) glUniform1f(lightPowerLoc, 4.0f);

    glm::vec3 cameraPos(3.0f, 3.0f, 3.0f);
    glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);
    glm::vec3 lightPos = cameraPos * 1.1f;
    float cameraYaw = -135.0f;
    float cameraPitch = -35.2643897f;
    const float moveSpeed = 2.5f;
    const float rotateSpeed = 90.0f;

    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && !glfwWindowShouldClose(window)) {
        glfwPollEvents();

        int fbW = 0;
        int fbH = 0;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        if (fbW > 0 && fbH > 0) {
            glViewport(0, 0, fbW, fbH);
        }

        const float currentTime = (float)glfwGetTime();
        static float lastTime = currentTime;
        const float deltaTime = glm::clamp(currentTime - lastTime, 0.0f, 0.1f);
        lastTime = currentTime;
        const float t = currentTime;
        const float aspect = (fbH > 0) ? ((float)fbW / (float)fbH) : 1.0f;

        // compute orientation from yaw/pitch
        if (g_input.left) cameraYaw -= rotateSpeed * deltaTime;
        if (g_input.right) cameraYaw += rotateSpeed * deltaTime;
        if (g_input.up) cameraPitch += rotateSpeed * deltaTime;
        if (g_input.down) cameraPitch -= rotateSpeed * deltaTime;
        cameraPitch = glm::clamp(cameraPitch, -89.0f, 89.0f);

        glm::vec3 cameraFront;
        cameraFront.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
        cameraFront.y = sin(glm::radians(cameraPitch));
        cameraFront.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
        cameraFront = glm::normalize(cameraFront);
        glm::vec3 cameraRight = glm::normalize(glm::cross(cameraFront, cameraUp));

        // apply movement along camera axes
        if (g_input.w) cameraPos += cameraFront * moveSpeed * deltaTime;
        if (g_input.s) cameraPos -= cameraFront * moveSpeed * deltaTime;
        if (g_input.a) cameraPos -= cameraRight * moveSpeed * deltaTime;
        if (g_input.d) cameraPos += cameraRight * moveSpeed * deltaTime;
        if (g_input.space) cameraPos += glm::vec3(0.0f, 1.0f, 0.0f) * moveSpeed * deltaTime;
        if (g_input.shift) cameraPos -= glm::vec3(0.0f, 1.0f, 0.0f) * moveSpeed * deltaTime;


        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);

        glUseProgram(program);

        // Set up matrices
        glm::mat4 model(1.0f);
        model = glm::rotate(model, t * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, t * 0.3f, glm::vec3(1.0f, 0.0f, 0.0f));

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
        glm::mat4 mvp = proj * view * model;

        glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(model)));

        // Upload matrices
        if (mvpLoc >= 0) glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
        if (modelLoc >= 0) glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        if (viewLoc >= 0) glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        if (normalMatLoc >= 0) glUniformMatrix3fv(normalMatLoc, 1, GL_FALSE, glm::value_ptr(normalMat));

        // Upload light and view positions
        if (lightPosLoc >= 0) glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
        if (viewPosLoc >= 0) glUniform3fv(viewPosLoc, 1, glm::value_ptr(cameraPos));

        // Bind textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseTex);
        if (diffuseTexLoc >= 0) glUniform1i(diffuseTexLoc, 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normalTex);
        if (normalTexLoc >= 0) glUniform1i(normalTexLoc, 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, specularTex);
        if (specularTexLoc >= 0) glUniform1i(specularTexLoc, 2);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glfwSwapBuffers(window);
    }

    // Cleanup
    glDeleteBuffers(1, &vboBitangent);
    glDeleteBuffers(1, &vboTangent);
    glDeleteBuffers(1, &vboNormal);
    glDeleteBuffers(1, &vboVertex);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);
    glDeleteTextures(1, &specularTex);
    glDeleteTextures(1, &normalTex);
    glDeleteTextures(1, &diffuseTex);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

// ===== Helper Functions =====

GLFWwindow* initGLFWAndContext() {
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

    GLFWwindow* ret = glfwCreateWindow(1024, 768, "glutil normal mapping example", nullptr, nullptr);
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

GLuint compileShader(GLenum type, const char* source) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    const auto r = glutil::Inspector::shaderCompileResult(shader);
    if (r.ok) return shader;

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

    const auto r = glutil::Inspector::programLinkResult(program);
    if (r.ok) return program;

    std::cerr << "Program link failed:\n" << r.message << std::endl;
    glDeleteProgram(program);
    return 0;
}

// Key callback stores compact input state for portability and decoupled handling
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

GLuint uploadStandard2D(const glutil::TextureImage& image) {
    if (!image.ok || image.data() == nullptr) return 0;

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, image.internalFormat(), image.width(), image.height(), 0,
                 image.format(), GL_UNSIGNED_BYTE, image.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

GLuint uploadDDS2D(const glutil::TextureDDS& dds) {
    if (!dds.ok || dds.data() == nullptr || dds.mips().empty()) return 0;

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, (GLint)(dds.mips().size() - 1));

    for (size_t level = 0; level < dds.mips().size(); ++level) {
        const glutil::MipLevel& m = dds.mips()[level];
        glCompressedTexImage2D(GL_TEXTURE_2D, (GLint)level, dds.format(), m.width, m.height, 0,
                               m.size, dds.data() + m.offset);

        const GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "DDS glCompressedTexImage2D failed at mip " << level << " (glError=0x"
                      << std::hex << err << std::dec << ")" << std::endl;
            glBindTexture(GL_TEXTURE_2D, 0);
            glDeleteTextures(1, &tex);
            return 0;
        }
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}
