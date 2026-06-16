#ifndef GLUTIL_DEBUG_TRACKER_HPP
#define GLUTIL_DEBUG_TRACKER_HPP

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include <glutil/gl.hpp>
#include <glutil/glToString.hpp>
#include <glutil/logging.hpp>

namespace glutil::debug {

/**
 * Buffer classification used by debug tracking.
 */
enum class BufferRole {
    Unknown,
    VBO,
    EBO,
};

/**
 * Debug-tracked metadata for a GL buffer object.
 *
 * This structure does not own GPU memory.
 * It mirrors runtime state for debugging and leak detection.
 */
struct BufferInfo {
    BufferRole role = BufferRole::Unknown;
    /** Last recorded GL data type for this buffer. */
    GLenum dataType = 0;
    /** Last recorded buffer size in bytes. */
    GLsizeiptr size = 0;
    /** VAO IDs associated with this buffer. */
    std::unordered_set<GLuint> associatedVaos;
    /** Debug label assigned to this buffer. */
    std::string label;
    // Not just "has a non-empty label": this tracks whether auto-labeling was already attempted,
    // so we avoid repeated attempts even when labeling is unsupported and the attempt fails.
    bool autoLabeled = false;
};

/**
 * Registry of buffer objects for debug tracking.
 */
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

/**
 * Debug metadata for a generic GL object.
 */
struct ObjectInfo {
    std::string label;
    // Not just "has a non-empty label": this tracks whether auto-labeling was already attempted,
    // so we avoid repeated attempts even when labeling is unsupported and the attempt fails.
    bool autoLabeled = false;
};

/** Key type for GL object registry (type + id). */
using ObjectKey = std::pair<GLenum, GLuint>;
/** Hash function for GL object keys. */
struct ObjectKeyHash {
    std::size_t operator()(const ObjectKey& key) const noexcept {
        const std::size_t h1 = std::hash<GLenum>{}(key.first);
        const std::size_t h2 = std::hash<GLuint>{}(key.second);
        return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    }
};

/** Registry of all tracked GL objects (debug-only metadata). */
class GLObjectRegistry {
public:
    void create(GLenum type, GLuint id) { objects[{type, id}] = {}; }
    void destroy(GLenum type, GLuint id) { objects.erase({type, id}); }
    ObjectInfo* get(GLenum type, GLuint id) {
        auto it = objects.find({type, id});
        if (it == objects.end())
            return nullptr;
        return &it->second;
    }
    const std::unordered_map<ObjectKey, ObjectInfo, ObjectKeyHash>& getAll() const { return objects; }

private:
    std::unordered_map<ObjectKey, ObjectInfo, ObjectKeyHash> objects;
};

/**
 * Global OpenGL state + object tracker (debug singleton).
 *
 * Stores mirrored binding state and registered GL objects.
 */
class GLStateTracker {
public:
    static GLStateTracker& instance() {
        static GLStateTracker tracker;
        return tracker;
    }

    BufferRegistry buffers;
    GLObjectRegistry objects;

    /** Currently bound VAO. */
    GLuint boundVAO = 0;
    /** Currently bound GL_ARRAY_BUFFER. */
    GLuint boundArrayBuffer = 0;
    /** Currently bound GL_ELEMENT_ARRAY_BUFFER. */
    GLuint boundElementArrayBuffer = 0;
    /** Currently bound textures by target. */
    std::unordered_map<GLenum, GLuint> boundTextures;

private:
    GLStateTracker() = default;
#if GDM_DEBUG
    /** Debug-only destructor that reports remaining tracked GL objects. */
    ~GLStateTracker() {
        bool hasLeak = false;

        for (auto& [id, info] : buffers.getAll()) {
            if (!hasLeak) {
                LOG_INFO() << "=== Leak Check ===";
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
                LOG_INFO() << "=== Leak Check ===";
                hasLeak = true;
            }
            LOG_ERROR() << "[LEAK] Object id=" << id << " type=" << glLabelObjectTypeToString(type)
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