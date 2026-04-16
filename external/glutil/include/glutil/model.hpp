#pragma once
#include <string>
#include <vector>

namespace glutil {

// ── Vertex ────────────────────────────────────────────────
// One vertex: position + normal + UV, interleaved.
// Fields are zeroed out if the OBJ does not provide them.
struct Vertex {
    float px, py, pz;  // position  (3D world space)
    float nx, ny, nz;  // normal    (unit vector, 0,0,0 if absent)
    float u,  v;       // texcoord  (0-1 range, 0,0 if absent)
};

// ── MeshData ──────────────────────────────────────────────
// Corresponds to one OBJ shape (the "o" keyword).
// No OpenGL types or calls — plain numeric data only.
//
// [why vector instead of new[]]
// Unlike TextureDDS (fixed-size blob), vertex/index counts are unknown
// until parsing is complete. vector handles dynamic growth safely.
//
// [why no private + friend pattern]
// MeshData is a plain data container assembled by ModelLoader.
// There is no invariant to enforce, so the texture-style
// private/friend overhead is unnecessary.
struct MeshData {
    bool        ok    = false;
    std::string error;

    std::string name;                // shape name from "o <n>" in OBJ
    std::string diffuseTexturePath;  // absolute path from map_Kd in .mtl
                                     // empty string if no material/texture

    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;

    // Read-only accessors (mirrors TextureImage/TextureDDS style)
    const Vertex*       vertexData()  const { return vertices.data(); }
    const unsigned int* indexData()   const { return indices.data(); }
    size_t              vertexCount() const { return vertices.size(); }
    size_t              indexCount()  const { return indices.size(); }
};

// ── ModelData ─────────────────────────────────────────────
// Corresponds to one OBJ file. May contain multiple meshes.
struct ModelData {
    bool        ok    = false;
    std::string error;
    std::string warn;  // forwarded tinyobjloader warnings
                       // may be non-empty even when ok == true

    std::vector<MeshData> meshes;
};

// ── ModelLoader ───────────────────────────────────────────
// Parses an OBJ file and returns a ModelData.
// No OpenGL calls. Responsibility ends at CPU-side data assembly.
//
// Usage:
//   glutil::ModelData m = glutil::ModelLoader::loadOBJ("room.obj");
//   if (!m.ok) { /* handle error */ }
//   for (const auto& mesh : m.meshes) {
//       // upload mesh.vertexData() / mesh.indexData() via glBufferData
//   }
class ModelLoader {
public:
    static ModelData loadOBJ(const char* path);
};

} // namespace glutil
