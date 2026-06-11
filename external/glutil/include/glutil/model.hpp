#ifndef GLUTIL_MODEL_HPP
#define GLUTIL_MODEL_HPP

#include <glutil/gl.hpp>
#include <glutil/math.hpp>

#include <string>
#include <vector>
#include <filesystem>
#include <utility>

namespace glutil {

struct MeshData {
    bool ok = false;
    std::string error;

    std::string name;
    std::string diffuseTexturePath;

    std::vector<VertexPNT> vertices;
    std::vector<unsigned int> indices;

    const VertexPNT* vertexData() const { return vertices.data(); }
    const unsigned int* indexData() const { return indices.data(); }
    size_t vertexCount() const { return vertices.size(); }
    size_t indexCount() const { return indices.size(); }
};

struct ModelData {
    bool ok = false;
    std::string error;
    std::string warn;

    std::vector<MeshData> meshes;
};

struct GLMeshData {
    bool ok = false, resetInDtor = true;
    std::string error;

    std::string name;
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLsizei indexCount = 0;

    GLMeshData() = default;
    ~GLMeshData() { if(resetInDtor) reset(); }

    GLMeshData(const GLMeshData&) = delete;
    GLMeshData& operator=(const GLMeshData&) = delete;

    GLMeshData(GLMeshData&& other) noexcept { moveFrom(std::move(other)); }
    GLMeshData& operator=(GLMeshData&& other) noexcept {
        if (this != &other) {
            reset();
            moveFrom(std::move(other));
        }
        return *this;
    }

    void reset() noexcept {
        if (ebo != 0) {
            glDeleteBuffers(1, &ebo);
            ebo = 0;
        }
        if (vbo != 0) {
            glDeleteBuffers(1, &vbo);
            vbo = 0;
        }
        if (vao != 0) {
            glDeleteVertexArrays(1, &vao);
            vao = 0;
        }
        indexCount = 0;
    }

private:
    void moveFrom(GLMeshData&& other) noexcept {
        ok = other.ok;
        error = std::move(other.error);
        name = std::move(other.name);
        vao = other.vao;
        vbo = other.vbo;
        ebo = other.ebo;
        indexCount = other.indexCount;

        other.ok = false;
        other.error.clear();
        other.name.clear();
        other.vao = 0;
        other.vbo = 0;
        other.ebo = 0;
        other.indexCount = 0;
    }
};

struct GLModelData {
    bool ok = false, resetInDtor = true;
    std::string error;
    std::string warn;

    std::vector<GLMeshData> meshes;

    GLModelData() = default;
    ~GLModelData() { if (resetInDtor) reset();};

    GLModelData(const GLModelData&) = delete;
    GLModelData& operator=(const GLModelData&) = delete;
    GLModelData(GLModelData&&) noexcept = default;
    GLModelData& operator=(GLModelData&&) noexcept = default;

    void reset() noexcept { meshes.clear(); }
};

class ModelLoader {
public:
    static ModelData loadOBJ(const std::filesystem::path& path, bool deduplicate = false);
    static GLModelData loadOBJtoGL(const std::filesystem::path& path, bool deduplicate = true);
};

} // namespace glutil

#endif // GLUTIL_MODEL_HPP
