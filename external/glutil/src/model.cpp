#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <glutil/path.hpp>
#include <glutil/model.hpp>
#include <glutil/logging.hpp>

#include <algorithm>
#include <filesystem>

namespace glutil {

// ── helpers ───────────────────────────────────────────────
template <size_t Stride> static float safeGet(const std::vector<float>& arr, int idx, int component) {
    if (idx < 0)
        return 0.0f;
    const auto base = static_cast<size_t>(Stride * static_cast<size_t>(idx) + static_cast<size_t>(component));
    return (base < arr.size()) ? arr[base] : 0.0f;
}

ModelData ModelLoader::loadOBJ(const char* path) {
    ModelData result;

    if (!path || path[0] == '\0') {
        result.error = "loadOBJ: path is empty";
        LOG_ERROR() << "[Model] " << result.error;
        return result;
    }

    const PathResolveResult pathResult = pathResolve(path);
    if (!pathResult.success) {
        result.error = "loadOBJ: path resolve failed: " + pathResult.message;
        LOG_ERROR() << "[Model] " << result.error;
        return result;
    }

    const std::filesystem::path objPath(pathResult.resolvedPath);
    const std::string mtlDir = objPath.parent_path().string();

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    const bool loaded = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, pathResult.resolvedPath.c_str(),
                                         mtlDir.empty() ? nullptr : mtlDir.c_str(), true);

    auto appendWarn = [&result](const std::string& msg) {
        if (!result.warn.empty())
            result.warn += '\n';
        result.warn += msg;
    };

    if (!warn.empty())
        appendWarn(warn);

    if (!loaded) {
        result.error = err.empty() ? "tinyobj::LoadObj failed (unknown reason)" : err;
        LOG_ERROR() << "[Model] load failed: " << pathResult.resolvedPath << " (" << result.error << ")";
        return result;
    }

    size_t totalVertices = 0;
    size_t totalIndices = 0;
    size_t texturedMeshCount = 0;
    size_t unnamedMeshCount = 0;

    for (size_t shapeIdx = 0; shapeIdx < shapes.size(); ++shapeIdx) {
        const tinyobj::shape_t& shape = shapes[shapeIdx];

        MeshData mesh;
        mesh.ok = true;
        mesh.name = shape.name.empty() ? (objPath.stem().string() + "_shape_" + std::to_string(shapeIdx)) : shape.name;

        int firstValidMatId = -1;
        int firstDiffuseMatId = -1;
        bool warnedInvalidMatId = false;
        bool warnedMixedMaterial = false;

        // TODO(3): shape에 여러 material이 섞인 경우 material 단위로 MeshData를 분할(submesh)하는 구조로 확장 검토.
        for (int matId : shape.mesh.material_ids) {
            if (matId < 0 || static_cast<size_t>(matId) >= materials.size()) {
                if (!warnedInvalidMatId) {
                    appendWarn("loadOBJ: invalid material ID in shape '" + shape.name + "': " + std::to_string(matId));
                    warnedInvalidMatId = true;
                }
                continue;
            }

            if (firstValidMatId < 0) {
                firstValidMatId = matId;
            } else if (matId != firstValidMatId && !warnedMixedMaterial) {
                appendWarn("loadOBJ: shape '" + (shape.name.empty() ? std::string("<unnamed>") : shape.name) +
                           "' has multiple materials. Only one diffuse texture path is stored.");
                warnedMixedMaterial = true;
            }

            const std::string& kd = materials[static_cast<size_t>(matId)].diffuse_texname;
            if (firstDiffuseMatId < 0 && !kd.empty()) {
                firstDiffuseMatId = matId;
            }
        }

        if (firstDiffuseMatId >= 0) {
            const std::string& kd = materials[static_cast<size_t>(firstDiffuseMatId)].diffuse_texname;
            mesh.diffuseTexturePath = (objPath.parent_path() / kd).string();
        }

        const size_t numIdx = shape.mesh.indices.size();
        mesh.vertices.reserve(numIdx);
        mesh.indices.reserve(numIdx);

        // TODO(4): (position, normal, uv) 인덱스 조합 기준으로 정점 dedup 적용해 정점/인덱스 버퍼 최적화.
        for (size_t i = 0; i < numIdx; ++i) {
            const tinyobj::index_t& idx = shape.mesh.indices[i];

            VertexPNT v{};

            v.x = safeGet<3>(attrib.vertices, idx.vertex_index, 0);
            v.y = safeGet<3>(attrib.vertices, idx.vertex_index, 1);
            v.z = safeGet<3>(attrib.vertices, idx.vertex_index, 2);

            v.nx = safeGet<3>(attrib.normals, idx.normal_index, 0);
            v.ny = safeGet<3>(attrib.normals, idx.normal_index, 1);
            v.nz = safeGet<3>(attrib.normals, idx.normal_index, 2);

            v.u = safeGet<2>(attrib.texcoords, idx.texcoord_index, 0);
            v.v = safeGet<2>(attrib.texcoords, idx.texcoord_index, 1);

            mesh.vertices.push_back(v);
            mesh.indices.push_back(static_cast<unsigned int>(i));
        }

        const std::string meshNameForLog = mesh.name.empty() ? "<unnamed>" : mesh.name;
        LOG_INFO() << "[Model] Mesh loaded: name=" << meshNameForLog << ", vertices=" << mesh.vertexCount()
                   << ", indices=" << mesh.indexCount() << ", hasDiffuse=" << (!mesh.diffuseTexturePath.empty());

        totalVertices += mesh.vertexCount();
        totalIndices += mesh.indexCount();
        if (!mesh.diffuseTexturePath.empty())
            ++texturedMeshCount;
        if (mesh.name.empty())
            ++unnamedMeshCount;

        result.meshes.push_back(std::move(mesh));
    }

    result.ok = true;

    const size_t warnLineCount =
      result.warn.empty() ? 0 : (std::count(result.warn.begin(), result.warn.end(), '\n') + 1);

    if (!result.warn.empty()) {
        LOG_WARNING() << "[Model] Load warnings (" << warnLineCount << "):\n" << result.warn;
    }

    LOG_INFO() << "[Model] Load succeeded: " << pathResult.resolvedPath;
    LOG_INFO() << "[Model]                 (meshes=" << result.meshes.size() << ", vertices=" << totalVertices
               << ", indices=" << totalIndices << ", texturedMeshes=" << texturedMeshCount
               << ", unnamedMeshes=" << unnamedMeshCount << ", warnings=" << warnLineCount << ")";

    return result;
}

} // namespace glutil