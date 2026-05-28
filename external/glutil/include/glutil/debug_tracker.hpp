#ifndef GLUTIL_DEBUG_TRACKER_HPP
#define GLUTIL_DEBUG_TRACKER_HPP

#include <glutil/gl.hpp>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace glutil::debug {

// ──────────────────────────────────────────────
// Buffer (VBO / EBO)
// ──────────────────────────────────────────────

enum class BufferRole {
    Unknown,
    VBO,
    EBO,
};

struct BufferInfo {
    BufferRole role = BufferRole::Unknown;
    GLenum dataType = 0; 
    GLsizeiptr size = 0; 
    std::unordered_set<GLuint> associatedVaos;
    std::string label;
};

class BufferRegistry {
public:
    void create(GLuint id) { buffers[id] = {}; }
    void destroy(GLuint id) { buffers.erase(id); }
    BufferInfo* get(GLuint id) {
        auto it = buffers.find(id);
        if (it == buffers.end())
            return nullptr;
        return &it->second;
    }
    const std::unordered_map<GLuint, BufferInfo>& getAll() const { return buffers; }

private:
    std::unordered_map<GLuint, BufferInfo> buffers;
};

 
// "VAO", "Texture", "Shader", "Program", "FBO",

struct ObjectInfo {
    std::string label;
};

class GLObjectRegistry {
public:
    void create(const std::string& type, GLuint id) { objects[type][id] = {}; }
    void destroy(const std::string& type, GLuint id) { objects[type].erase(id); }
    ObjectInfo* get(const std::string& type, GLuint id) {
        auto typeIt = objects.find(type);
        if (typeIt == objects.end())
            return nullptr;
        auto idIt = typeIt->second.find(id);
        if (idIt == typeIt->second.end())
            return nullptr;
        return &idIt->second;
    }
    const std::unordered_map<std::string, std::unordered_map<GLuint, ObjectInfo>>& getAll() const { return objects; }

private:
    std::unordered_map<std::string, std::unordered_map<GLuint, ObjectInfo>> objects;
};

class GLStateTracker {
public:
    static GLStateTracker& instance() {
        static GLStateTracker tracker;
        return tracker;
    }

    BufferRegistry buffers;
    GLObjectRegistry objects;

    GLuint boundVAO = 0;
    GLuint boundArrayBuffer = 0;
    GLuint boundElementArrayBuffer = 0;
    std::unordered_map<GLenum, GLuint> boundTextures;

private:
    GLStateTracker() = default;
    ~GLStateTracker()
    {
        bool hasLeak = false;

        for (auto& [id, info] : buffers.getAll()) {
            if (!hasLeak) {
                fprintf(stderr, "\n=== Leak Check ===\n");
                hasLeak = true;
            }
            const char* role = (info.role == BufferRole::VBO)   ? "VBO"
                               : (info.role == BufferRole::EBO) ? "EBO"
                                                                : "Unknown";

            const char* label = info.label.c_str();

            fprintf(stderr, "[LEAK] Buffer id=%u role=%s label=%s\n", id, role, label);
        }

       for (auto& [type, ids] : objects.getAll()) {
            for (auto& [id, info] : ids) {
                if (!hasLeak) {
                    fprintf(stderr, "\n=== Leak Check ===\n");
                    hasLeak = true;
                }
                fprintf(stderr, "[LEAK] Object id=%u type=%s label=%s\n", id, type.c_str(),
                        info.label.empty() ? "(none)" : info.label.c_str());
            }
        }

        if (!hasLeak)
            fprintf(stderr, "\n=== No Leaks Detected ===\n");
    }
};

} // namespace glutil::debug
#endif // GLUTIL_DEBUG_TRACKER_HPP