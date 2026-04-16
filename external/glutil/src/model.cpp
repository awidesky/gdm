// glutil/model.cpp
//
// [TINYOBJLOADER_IMPLEMENTATION]
// Must be defined in exactly one .cpp file.
// Same rule as STB_IMAGE_IMPLEMENTATION / DDSKTX_IMPLEMENT.
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <glutil/model.hpp>

#include <filesystem>

namespace glutil {

// ── helpers ───────────────────────────────────────────────

// tinyobjloader stores positions/normals as a flat float array:
//   [x0,y0,z0, x1,y1,z1, ...]
// index_t fields are signed int; negative value means "not present".
// safeGet3 / safeGet2 handle both the absent case and bounds check.

static float safeGet3(const std::vector<float>& arr, int idx, int component)
{
    if (idx < 0) return 0.0f;
    const auto base = static_cast<size_t>(3 * idx + component);
    return (base < arr.size()) ? arr[base] : 0.0f;
}

static float safeGet2(const std::vector<float>& arr, int idx, int component)
{
    if (idx < 0) return 0.0f;
    const auto base = static_cast<size_t>(2 * idx + component);
    return (base < arr.size()) ? arr[base] : 0.0f;
}

// ── ModelLoader::loadOBJ ──────────────────────────────────
ModelData ModelLoader::loadOBJ(const char* path)
{
    ModelData result;

    if (!path || path[0] == '\0') {
        result.error = "loadOBJ: path is empty";
        return result;
    }

    // tinyobjloader needs the OBJ directory to locate .mtl files.
    const std::filesystem::path objPath(path);
    const std::string mtlDir = objPath.parent_path().string();

    tinyobj::attrib_t                attrib;
    std::vector<tinyobj::shape_t>    shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    const bool loaded = tinyobj::LoadObj(
        &attrib, &shapes, &materials,
        &warn, &err,
        path,
        mtlDir.empty() ? nullptr : mtlDir.c_str(),
        /*triangulate=*/true);   // auto-convert quads → triangles

    if (!warn.empty())
        result.warn = warn;

    if (!loaded) {
        result.error = err.empty()
            ? "tinyobj::LoadObj failed (unknown reason)"
            : err;
        return result;
    }

    // ── one shape → one MeshData ──────────────────────────
    for (const tinyobj::shape_t& shape : shapes) {
        MeshData mesh;
        mesh.ok   = true;
        mesh.name = shape.name;

        // ── resolve diffuse texture path ──────────────────
        // Each face can reference a different material.
        // We take the first valid one for the whole mesh
        // (simple approach; multi-material meshes can be
        //  split at the OBJ level if needed).
        for (int matId : shape.mesh.material_ids) {
            if (matId < 0 ||
                static_cast<size_t>(matId) >= materials.size())
                continue;

            const std::string& kd =
                materials[static_cast<size_t>(matId)].diffuse_texname;
            if (kd.empty()) continue;

            // Build absolute path so callers don't need to know
            // the OBJ directory.
            mesh.diffuseTexturePath =
                (objPath.parent_path() / kd).string();
            break;
        }

        // ── assemble vertices from index triples ──────────
        //
        // tinyobjloader uses separate index spaces for position,
        // normal and texcoord (same as the OBJ spec).
        // We expand every index triple into a full Vertex and
        // build a simple sequential index buffer (0,1,2,3,...).
        //
        // [sequential indices]
        // Deduplication (merging identical Vertex values) would
        // reduce VRAM usage but adds complexity. Sequential indices
        // are correct and easy to reason about; optimize later if
        // needed.
        //
        // [UV V-flip]
        // OBJ uses a bottom-left origin for V (V=0 is the bottom of
        // the image). OpenGL's glTexImage2D expects a top-left origin.
        // Flipping V here avoids having to flip every texture on load.

        const size_t numIdx = shape.mesh.indices.size();
        mesh.vertices.reserve(numIdx);
        mesh.indices.reserve(numIdx);

        for (size_t i = 0; i < numIdx; ++i) {
            const tinyobj::index_t& idx = shape.mesh.indices[i];

            Vertex v{};

            // position — always present in a valid OBJ
            v.px = safeGet3(attrib.vertices, idx.vertex_index,   0);
            v.py = safeGet3(attrib.vertices, idx.vertex_index,   1);
            v.pz = safeGet3(attrib.vertices, idx.vertex_index,   2);

            // normal — optional (negative index = absent)
            v.nx = safeGet3(attrib.normals,  idx.normal_index,   0);
            v.ny = safeGet3(attrib.normals,  idx.normal_index,   1);
            v.nz = safeGet3(attrib.normals,  idx.normal_index,   2);

            // texcoord — optional, V flipped for OpenGL
            v.u  =        safeGet2(attrib.texcoords, idx.texcoord_index, 0);
            v.v  = 1.0f - safeGet2(attrib.texcoords, idx.texcoord_index, 1);

            mesh.vertices.push_back(v);
            mesh.indices.push_back(static_cast<unsigned int>(i));
        }

        result.meshes.push_back(std::move(mesh));
    }

    result.ok = true;
    return result;
}

} // namespace glutil
