#ifndef GLUTIL_SHADER_HPP
#define GLUTIL_SHADER_HPP

#include <glutil/gl.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <filesystem>

namespace glutil {

struct ShaderLoadResult {
    bool ok = false;
    std::string error;

    GLchar** string() { return &source; }
    const GLint* lengthPtr() const { return &len; }
    GLint length() const { return len; }

    void destroy() {
        delete[] source;
        source = nullptr;
        len = 0;
        ok = false;
        error.clear();
    }

    ~ShaderLoadResult() { destroy(); }

    ShaderLoadResult() = default;
    ShaderLoadResult(const ShaderLoadResult&) = delete;
    ShaderLoadResult& operator=(const ShaderLoadResult&) = delete;

    ShaderLoadResult(ShaderLoadResult&& other) noexcept { moveFrom(std::move(other)); }

    ShaderLoadResult& operator=(ShaderLoadResult&& other) noexcept {
        if (this != &other) {
            destroy();
            moveFrom(std::move(other));
        }
        return *this;
    }

private:
    friend class ShaderLoader;

    GLchar* source = nullptr;
    GLint len = 0;

    void moveFrom(ShaderLoadResult&& other) {
        ok = other.ok;
        error = std::move(other.error);
        source = other.source;
        len = other.len;

        other.ok = false;
        other.source = nullptr;
        other.len = 0;
        other.error.clear();
    }
};

struct GLShader {
    bool ok = false, resetInDtor = true;
    std::string error;

    GLuint id = 0;
    GLenum type = 0;

    GLShader() = default;
    ~GLShader() { if (resetInDtor) reset(); }

    GLShader(const GLShader&) = delete;
    GLShader& operator=(const GLShader&) = delete;

    GLShader(GLShader&& other) noexcept { moveFrom(std::move(other)); }
    GLShader& operator=(GLShader&& other) noexcept {
        if (this != &other) {
            reset();
            moveFrom(std::move(other));
        }
        return *this;
    }

    void reset() noexcept {
        if (id != 0) {
            glDeleteShader(id);
            id = 0;
        }
        type = 0;
    }

private:
    void moveFrom(GLShader&& other) noexcept {
        ok = other.ok;
        error = std::move(other.error);
        id = other.id;
        type = other.type;

        other.ok = false;
        other.error.clear();
        other.id = 0;
        other.type = 0;
    }
};

struct GLProgram {
    bool ok = false, resetInDtor = true;
    std::string error;

    GLuint id = 0;

    GLProgram() = default;
    ~GLProgram() { if (resetInDtor) reset(); }

    GLProgram(const GLProgram&) = delete;
    GLProgram& operator=(const GLProgram&) = delete;

    GLProgram(GLProgram&& other) noexcept { moveFrom(std::move(other)); }
    GLProgram& operator=(GLProgram&& other) noexcept {
        if (this != &other) {
            reset();
            moveFrom(std::move(other));
        }
        return *this;
    }

    void reset() noexcept {
        if (id != 0) {
            glDeleteProgram(id);
            id = 0;
        }
    }

private:
    void moveFrom(GLProgram&& other) noexcept {
        ok = other.ok;
        error = std::move(other.error);
        id = other.id;

        other.ok = false;
        other.error.clear();
        other.id = 0;
    }
};

class ShaderLoader {
public:
    static bool checkEncoding;
    static bool replaceUnknownNonASCII;
    static ShaderLoadResult loadFile(const std::filesystem::path& inputPath);

    static GLShader loadShaderToGL(GLenum type, const std::filesystem::path& inputPath);
    static GLProgram loadProgramToGL(const std::filesystem::path& vertexPath,
                                     const std::filesystem::path& fragmentPath);
};

inline static bool hasNonASCII(const char* data, size_t size) {
    constexpr uint64_t HIGH_BIT_MASK = 0x8080808080808080ULL;

    size_t i = 0;

    for (; i + 8 <= size; i += 8) {
        uint64_t word;
        std::memcpy(&word, data + i, sizeof(word));

        if ((word & HIGH_BIT_MASK) != 0)
            return true;
    }

    for (; i < size; ++i) {
        if ((static_cast<unsigned char>(data[i]) & 0x80) != 0)
            return true;
    }

    return false;
}

inline static void replaceNonASCIIWithSpace(char* data, size_t size) {
    constexpr uint64_t HIGH_BIT_MASK = 0x8080808080808080ULL;

    size_t i = 0;

    for (; i + 8 <= size; i += 8) {
        uint64_t word;
        std::memcpy(&word, data + i, sizeof(word));

        if ((word & HIGH_BIT_MASK) != 0) {
            unsigned char* bytes = reinterpret_cast<unsigned char*>(data + i);
            for (size_t j = 0; j < 8; ++j) {
                if (bytes[j] & 0x80)
                    bytes[j] = ' ';
            }
        }
    }

    for (; i < size; ++i) {
        if (static_cast<unsigned char>(data[i]) & 0x80)
            data[i] = ' ';
    }
}
} // namespace glutil

#endif // GLUTIL_SHADER_HPP
