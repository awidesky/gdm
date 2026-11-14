#ifndef GLUTIL_DEBUG_TRACKER_HPP
#define GLUTIL_DEBUG_TRACKER_HPP

#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <glutil/gl.hpp>
#include <glutil/glToString.hpp>
#include <glutil/logging.hpp>

namespace glutil::debug {

/**
 * Runtime configuration for the debug system.
 *
 * This struct is not declared in debug.hpp to avoid a circular include.
 * debug.hpp includes debug_tracker.hpp, and GLStateTracker's destructor (below)
 * references config, so config must be visible to this header.
 * 
 */
struct DebugConfig {
    /** Disable automatic GL object labeling. */
    bool disableAutoLabelGLObjects = false;
    /** Disable automatic inspection hooks. */
    bool disableAutoInspcector = false;
    /** Disable the GL object leak check at program exit. */
    bool disableLeakCheck = false;
    /** Disable the "No OpenGL Object Leaks Detected" report when no leak is found. */
    bool disableNoLeakReport = true;
};

#if GDM_DEBUG
/** Global debug configuration instance. Defined in debug.cpp. */
extern DebugConfig config;
#else
// In release builds the config is unused; provide a default instance.
// Values mirror the old release-mode defaults: all debug features disabled.
inline DebugConfig config{/*disableAutoLabelGLObjects=*/true,
                          /*disableAutoInspcector=*/true,
                          /*disableLeakCheck=*/true,
                          /*disableNoLeakReport=*/true};
#endif

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

    /** Buffer ids in ascending order, for deterministic report output. */
    std::vector<GLuint> sortedIds() const {
        std::vector<GLuint> ids;
        ids.reserve(buffers.size());
        for (const auto& [id, info] : buffers) ids.push_back(id);
        std::sort(ids.begin(), ids.end());
        return ids;
    }

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

    /** (id, type) pairs sorted by id then type, for deterministic report output. */
    std::vector<std::pair<GLuint, GLenum>> sortedIdTypePairs() const {
        std::vector<std::pair<GLuint, GLenum>> pairs;
        pairs.reserve(objects.size());
        for (const auto& [key, info] : objects) pairs.emplace_back(key.second, key.first);
        std::sort(pairs.begin(), pairs.end());
        return pairs;
    }

private:
    std::unordered_map<ObjectKey, ObjectInfo, ObjectKeyHash> objects;
};

/**
 * Global OpenGL state + object tracker (debug singleton).
 *
 * Stores mirrored binding state and registered GL objects.
 */
class GLStateTracker;
void printLeakReport(const GLStateTracker& tracker, const char* title = "OpenGL Leak Check");

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
    /** Currently bound GL_ELEMENT_ARRAY_BUFFER by glBindBuffer. */
    GLuint boundElementArrayBuffer = 0;
    /** Currently bound textures by target. */
    std::unordered_map<GLenum, GLuint> boundTextures;
    /** VAO with connected EBO. */
    std::unordered_map<GLuint, GLuint> vaoToElementBuffer;

private:
    GLStateTracker() = default;
#if GDM_DEBUG
    /** Debug-only destructor that reports remaining tracked GL objects. */
    ~GLStateTracker() {
        if (config.disableLeakCheck) return;
        printLeakReport(*this);
    }
#else
    ~GLStateTracker() = default;
#endif // GDM_DEBUG
};

/**
 * Reports every tracked GL object still registered in the tracker (i.e. not released
 * via the corresponding glDelete* call) as a leak.
 *
 * Buffers are printed in ascending id order and objects in (id, type) order, so the
 * output is deterministic and roughly follows the allocation timeline.
 *
 * @param title Report title, without the surrounding "=== ... ===" decoration.
 */
inline void printLeakReport(const GLStateTracker& tracker, const char* title) {
    const auto& buffers = tracker.buffers.getAll();
    const auto& objects = tracker.objects.getAll();
    if (buffers.empty() && objects.empty()) {
        if (!config.disableNoLeakReport)
            LOG_INFO() << "=== No OpenGL Object Leaks Detected ===";
        return;
    }

    LOG_INFO() << "=== " << title << " (" << buffers.size() << " buffer(s), " << objects.size()
               << " object(s) not released) ===";

    for (GLuint id : tracker.buffers.sortedIds()) {
        const BufferInfo& info = buffers.at(id);
        const char* role = (info.role == BufferRole::VBO)   ? "VBO"
                           : (info.role == BufferRole::EBO) ? "EBO"
                                                            : "Unknown";
        LOG_ERROR() << "[LEAK] Buffer id=" << id << " role=" << role
                    << " label=" << (info.label.empty() ? "(none)" : info.label);
    }

    for (const auto& [id, type] : tracker.objects.sortedIdTypePairs()) {
        const ObjectInfo& info = objects.at({type, id});
        LOG_ERROR() << "[LEAK] Object id=" << id << " type=" << glLabelObjectTypeToString(type)
                    << " label=" << (info.label.empty() ? "(none)" : info.label);
    }
}

} // namespace glutil::debug

#endif // GLUTIL_DEBUG_TRACKER_HPP