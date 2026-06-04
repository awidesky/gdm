#ifndef GLUTIL_DEBUG_TRACKER_HPP
#define GLUTIL_DEBUG_TRACKER_HPP

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include <glutil/gl.hpp>
#include <glutil/logging.hpp>

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
    // Not just "has a non-empty label": this tracks whether auto-labeling was already attempted,
    // so we avoid repeated attempts even when labeling is unsupported and the attempt fails.
    bool autoLabeled = false;
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
    // Not just "has a non-empty label": this tracks whether auto-labeling was already attempted,
    // so we avoid repeated attempts even when labeling is unsupported and the attempt fails.
    bool autoLabeled = false;
};

using ObjectKey = std::pair<std::string, GLuint>;
struct ObjectKeyHash {
    std::size_t operator()(const ObjectKey& key) const noexcept {
        const std::size_t h1 = std::hash<std::string>{}(key.first);
        const std::size_t h2 = std::hash<GLuint>{}(key.second);
        return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    }
};

class GLObjectRegistry {
public:
    // TODO_easy : make the type from string to glenum
    void create(const std::string& type, GLuint id) { objects[{type, id}] = {}; }
    void destroy(const std::string& type, GLuint id) { objects.erase({type, id}); }
    ObjectInfo* get(const std::string& type, GLuint id) {
        auto it = objects.find({type, id});
        if (it == objects.end())
            return nullptr;
        return &it->second;
    }
    const std::unordered_map<ObjectKey, ObjectInfo, ObjectKeyHash>& getAll() const { return objects; }

private:
    std::unordered_map<ObjectKey, ObjectInfo, ObjectKeyHash> objects;
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
#if GDM_DEBUG
    ~GLStateTracker() {
        bool hasLeak = false;

        for (auto& [id, info] : buffers.getAll()) {
            if (!hasLeak) {
                LOG_INFO() << "\n=== Leak Check ===";
                hasLeak = true;
            }
            const char* role = (info.role == BufferRole::VBO)   ? "VBO"
                               : (info.role == BufferRole::EBO) ? "EBO"
                                                                : "Unknown";

            LOG_ERROR() << "[LEAK] Buffer id=" << id << " role=" << role
                        << " label=" << (info.label.empty() ? "(none)" : info.label);
        }

       for (auto& [key, info] : objects.getAll()) {
            const auto& [type, id] = key;
            if (!hasLeak) {
                LOG_INFO() << "\n=== Leak Check ===";
                hasLeak = true;
            }
            LOG_ERROR() << "[LEAK] Object id=" << id << " type=" << type
                        << " label=" << (info.label.empty() ? "(none)" : info.label);
        }

        if (!hasLeak)
            LOG_INFO() << "=== No Leaks Detected ===";
    }
#else
        ~GLStateTracker() = default;
#endif // GDM_DEBUG
};

} // namespace glutil::debug

#endif // GLUTIL_DEBUG_TRACKER_HPP