#ifndef GLUTIL_MODEL_HPP
#define GLUTIL_MODEL_HPP

#include <glutil/gl.hpp>
#include <glutil/math.hpp>

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace glutil {

/**
 * CPU-side mesh data container.
 * Stores vertex and index data along with basic metadata and load status.
 *
 * DiffuseTexturePath is resolved per mesh but only a single diffuse texture is selected 
 * if multiple materials exist in the source shape.
 */
struct MeshData {
    bool ok = false;   // Indicates whether loading succeeded
    std::string error; // Error message if loading failed

    std::string name;// Mesh name
    std::string diffuseTexturePath; // Path to selected diffuse texture (if any)

    std::vector<VertexPNT> vertices;   // Vertex buffer data
    std::vector<unsigned int> indices; // Index buffer data

    const VertexPNT* vertexData() const { return vertices.data(); }
    const unsigned int* indexData() const { return indices.data(); }
    size_t vertexCount() const { return vertices.size(); }
    size_t indexCount() const { return indices.size(); }
};

/**
 * CPU-side model data container.
 * Contains multiple meshes and global load status information.
 */
struct ModelData {
    bool ok = false;   // Indicates whether loading succeeded
    std::string error; // Error message if loading failed
    std::string warn;  // Accumulated warnings from loader

    std::vector<MeshData> meshes; // List of meshes in the model
};

/**
 * GPU-side mesh data container.
 * Manages OpenGL resources (VAO, VBO, EBO) and their lifecycle via RAII.
 */
struct GLMeshData {
    bool ok = false, resetInDtor = true; // Status and destructor cleanup flag
    std::string error;// Error message

    std::string name;       // Mesh name
    GLuint vao = 0;         // Vertex Array Object
    GLuint vbo = 0;         // Vertex Buffer Object
    GLuint ebo = 0;         // Element Buffer Object
    GLsizei indexCount = 0; // Number of indices

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

    /**
     * Releases OpenGL resources (VAO/VBO/EBO).
     * Safe to call multiple times.
     */
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
    /**
     * Moves OpenGL resource ownership from another instance.
     *
     * After move, other is reset to null state, and ownership of VAO/VBO/EBO is transferred.
     */
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

/**
 * GPU-side model container.
 * Holds multiple GL meshes and manages their lifecycle.
 */
struct GLModelData {
    bool ok = false, resetInDtor = true; // Status and destructor cleanup flag
    std::string error;                   // Error message
    std::string warn;                    // Warning messages from CPU loading stage

    std::vector<GLMeshData> meshes; // GPU mesh list

    GLModelData() = default;
    ~GLModelData() { if (resetInDtor) reset();};

    GLModelData(const GLModelData&) = delete;
    GLModelData& operator=(const GLModelData&) = delete;
    GLModelData(GLModelData&&) noexcept = default;
    GLModelData& operator=(GLModelData&&) noexcept = default;

    /**
     * Releases all GPU mesh resources by clearing the mesh container.
     */
    void reset() noexcept { meshes.clear(); }
};

/**
 * Model loading utility.
 * Provides functions to load OBJ files into CPU or GPU representations.
 *
 * loadOBJ performs parsing, mesh construction, deduplication (optional),
 * and material interpretation. loadOBJtoGL first builds CPU representation, then uploads to GPU.
 */
class ModelLoader {
public:
    /**
     * Loads an OBJ file into CPU-side model data.
     *
     * @param path File path to OBJ model
     * @param deduplicate If true, identical vertices are merged
     */
    static ModelData loadOBJ(const std::filesystem::path& path, bool deduplicate = false);

    /**
     * Loads an OBJ file and uploads it into GPU-side model data.
     *
     * This performs:
     * - CPU loading (loadOBJ)
     * - GPU buffer creation (VAO/VBO/EBO)
     * - fixed vertex attribute layout (pos/normal/uv)
     *
     * @param path File path to OBJ model
     * @param deduplicate If true, identical vertices are merged before upload
     */
    static GLModelData loadOBJtoGL(const std::filesystem::path& path, bool deduplicate = true);
};

} // namespace glutil

#endif // GLUTIL_MODEL_HPP
