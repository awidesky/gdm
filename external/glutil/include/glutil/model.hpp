#ifndef GLUTIL_MODEL_HPP
#define GLUTIL_MODEL_HPP

#include <glutil/math.hpp>

#include <string>
#include <vector>

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

class ModelLoader {
public:
    static ModelData loadOBJ(const char* path);
};

} // namespace glutil

#endif // GLUTIL_MODEL_HPP
