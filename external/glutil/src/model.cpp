
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <glutil/model.hpp>

#include <filesystem>

namespace glutil {

// ── helpers ───────────────────────────────────────────────
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
        mtlDir.empty() ? nullptr : mtlDir.c_str(),true);   

    if (!warn.empty())
        result.warn = warn;

    if (!loaded) {
        result.error = err.empty()
            ? "tinyobj::LoadObj failed (unknown reason)"
            : err;
        return result;
    }

    for (const tinyobj::shape_t& shape : shapes) {
        MeshData mesh;
        mesh.ok   = true;
        mesh.name = shape.name;

        
        for (int matId : shape.mesh.material_ids) {
            if (matId < 0 ||
                static_cast<size_t>(matId) >= materials.size())
                continue;

            const std::string& kd =
                materials[static_cast<size_t>(matId)].diffuse_texname;
            if (kd.empty()) continue;

           
            mesh.diffuseTexturePath =
                (objPath.parent_path() / kd).string();
            break;
        }

       

        const size_t numIdx = shape.mesh.indices.size();
        mesh.vertices.reserve(numIdx);
        mesh.indices.reserve(numIdx);

        for (size_t i = 0; i < numIdx; ++i) {
            const tinyobj::index_t& idx = shape.mesh.indices[i];

            Vertex v{};

            v.px = safeGet3(attrib.vertices, idx.vertex_index,   0);
            v.py = safeGet3(attrib.vertices, idx.vertex_index,   1);
            v.pz = safeGet3(attrib.vertices, idx.vertex_index,   2);

            v.nx = safeGet3(attrib.normals,  idx.normal_index,   0);
            v.ny = safeGet3(attrib.normals,  idx.normal_index,   1);
            v.nz = safeGet3(attrib.normals,  idx.normal_index,   2);

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

} 
