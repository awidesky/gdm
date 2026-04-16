#pragma once
#include <string>
#include <vector>

namespace glutil {

// ── Vertex ────────────────────────────────────────────────

struct Vertex {
    float px, py, pz;  // position  
    float nx, ny, nz;  // normal    
    float u,  v;       // texcoord  
};


struct MeshData {
    bool        ok    = false;
    std::string error;

    std::string name;                
    std::string diffuseTexturePath;  
                                     

    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;

    const Vertex*       vertexData()  const { return vertices.data(); }
    const unsigned int* indexData()   const { return indices.data(); }
    size_t              vertexCount() const { return vertices.size(); }
    size_t              indexCount()  const { return indices.size(); }
};

// ── ModelData ─────────────────────────────────────────────
struct ModelData {
    bool        ok    = false;
    std::string error;
    std::string warn;  // forwarded tinyobjloader warnings
                       // may be non-empty even when ok == true

    std::vector<MeshData> meshes;
};

// ── ModelLoader ───────────────────────────────────────────
class ModelLoader {
public:
    static ModelData loadOBJ(const char* path);
};

} 
