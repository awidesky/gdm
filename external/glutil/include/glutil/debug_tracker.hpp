#pragma once
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

class GLObjectRegistry {
public:
    void create(const std::string& type, GLuint id) { objects[type].insert(id); }
    void destroy(const std::string& type, GLuint id) { objects[type].erase(id); }
    const std::unordered_map<std::string, std::unordered_set<GLuint>>& getAll() const { return objects; }

private:
    std::unordered_map<std::string, std::unordered_set<GLuint>> objects;
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

private:
    GLStateTracker() = default;
};

} // namespace glutil::debug