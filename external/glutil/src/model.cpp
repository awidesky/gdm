#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <glutil/path.hpp>
#include <glutil/model.hpp>
#include <glutil/logging.hpp>
#include <glutil/debug.hpp>

#include <algorithm>
#include <filesystem>
#include <unordered_map>
#include <functional>

namespace glutil {

// ── helpers ───────────────────────────────────────────────
template <size_t Stride> static float safeGet(const std::vector<float>& arr, int idx, int component) {
    if (idx < 0)
        return 0.0f;
    const auto base = static_cast<size_t>(Stride * static_cast<size_t>(idx) + static_cast<size_t>(component));
    return (base < arr.size()) ? arr[base] : 0.0f;
}
static std::string pathToUtf8(const std::filesystem::path& path) {
#ifdef _WIN32
    const std::wstring ws = path.native();
    if (ws.empty()) return {};

    int size = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), static_cast<int>(ws.size()), nullptr, 0, nullptr, nullptr);
    std::string out(size, '\0');

    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), static_cast<int>(ws.size()), out.data(), size, nullptr, nullptr);
    return out;
#else
    return path.string();
#endif
}

ModelData ModelLoader::loadOBJ(const std::filesystem::path& path, bool deduplicate) {
    ModelData result;

    const PathResolveResult pathResult = pathResolve(path);
    if (!pathResult.success) {
        result.error = "loadOBJ: path resolve failed: " + pathResult.message;
        LOG_ERROR() << "[Model] " << result.error;
        return result;
    }

    const std::filesystem::path objPath(pathResult.resolvedPath);
    const std::string objPathStr = pathToUtf8(pathResult.resolvedPath);
    const std::string mtlDir = pathToUtf8(objPath.parent_path());

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    const bool loaded = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, objPathStr.c_str(),
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

        // TODO_later(3): shape에 여러 material이 섞인 경우 material 단위로 MeshData를 분할(submesh)하는 구조로 확장 검토.
        for (int matId : shape.mesh.material_ids) {
            // no material (NORMAL CASE)
            if (matId < 0) continue;
            // invalid index (ERROR CASE)
            if (static_cast<size_t>(matId) >= materials.size()) {
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

        // TODO_later: attrib.colors도 있는 경우?
        auto hasher = [](const VertexPNT& vert) {
            // combine std::hash<float> results
            size_t h = std::hash<float>{}(vert.x);
            auto mix = [](size_t& seed, size_t v) { seed ^= v + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2); };
            mix(h, std::hash<float>{}(vert.y));
            mix(h, std::hash<float>{}(vert.z));
            mix(h, std::hash<float>{}(vert.nx));
            mix(h, std::hash<float>{}(vert.ny));
            mix(h, std::hash<float>{}(vert.nz));
            mix(h, std::hash<float>{}(vert.u));
            mix(h, std::hash<float>{}(vert.v));
            return h;
        };
        std::unordered_map<VertexPNT, unsigned int, decltype(hasher)> map(deduplicate ? numIdx : 0, hasher);

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

            if (deduplicate) {
                auto it = map.find(v);
                if (it != map.end()) {
                    mesh.indices.push_back(it->second);
                } else {
                    unsigned int newIndex = static_cast<unsigned int>(mesh.vertices.size());
                    mesh.vertices.push_back(v);
                    mesh.indices.push_back(newIndex);
                    map.emplace(v, newIndex);
                }
            } else {
                mesh.vertices.push_back(v);
                mesh.indices.push_back(static_cast<unsigned int>(i));
            }
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

static GLMeshData uploadMeshToGL(const MeshData& mesh, std::string baseName) {
    GLMeshData gm;
    gm.name = mesh.name;
    gm.indexCount = static_cast<GLsizei>(mesh.indexCount());

    glGenVertexArrays(1, &gm.vao);
    glBindVertexArray(gm.vao);

    glGenBuffers(1, &gm.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, gm.vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(mesh.vertexCount() * sizeof(VertexPNT)),
                 mesh.vertexData(),
                 GL_STATIC_DRAW);

    glGenBuffers(1, &gm.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gm.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(mesh.indexCount() * sizeof(unsigned int)),
                 mesh.indexData(),
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          sizeof(VertexPNT),
                          (void*)offsetof(VertexPNT, x));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                          sizeof(VertexPNT),
                          (void*)offsetof(VertexPNT, nx));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
                          sizeof(VertexPNT),
                          (void*)offsetof(VertexPNT, u));

    glBindVertexArray(0);

    glutil::debug::labelGLobject(GL_VERTEX_ARRAY, gm.vao, "VAO" + baseName);
    glutil::debug::labelGLobject(GL_BUFFER, gm.vbo, "VBO" + baseName);
    glutil::debug::labelGLobject(GL_BUFFER, gm.ebo, "EBO" + baseName);

    gm.ok = true;
    return gm;
}

GLModelData ModelLoader::loadOBJtoGL(const std::filesystem::path& path, bool deduplicate) {
    GLModelData result;

    const ModelData cpuModel = loadOBJ(path, deduplicate);
    result.warn = cpuModel.warn;
    if (!cpuModel.ok) {
        result.error = cpuModel.error;
        return result;
    }

    result.meshes.reserve(cpuModel.meshes.size());
    int i = 0;
    for (const MeshData& mesh : cpuModel.meshes) {
        result.meshes.push_back(
          uploadMeshToGL(mesh, ("(path=" +  path.filename().string() + ", object=" + 
                (mesh.name.empty() ? ("mesh #" + std::to_string(i)) : mesh.name) + ")")));
        i++;
    }

    result.ok = true;
    return result;
}

} // namespace glutil