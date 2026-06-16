#ifndef GLUTIL_SHADER_HPP
#define GLUTIL_SHADER_HPP

#include <glutil/gl.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string>
#include <utility>

namespace glutil {

/**
 * Container for loaded shader source data.
 *
 * Owns dynamically allocated shader source buffer.
 * Move-only type; used as intermediate representation before OpenGL compilation.
 */
struct ShaderLoadResult {
    bool ok = false;
    std::string error;

    /** Pointer to internal GLchar* source buffer (OpenGL-compatible layout). */
    GLchar** string() { return &source; }
    /** Pointer to source length (used for glShaderSource). */
    const GLint* lengthPtr() const { return &len; }
    /** Returns source length in bytes. */
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

/**
 * RAII wrapper for an OpenGL shader object.
 *
 * Manages lifetime of GL shader handle.
 * Automatically deletes shader in destructor unless resetInDtor is false.
 */
struct GLShader {
    bool ok = false, resetInDtor = true;
    std::string error;

    /** OpenGL shader object handle (glCreateShader result). */
    GLuint id = 0;
    /** Shader stage type (e.g. GL_VERTEX_SHADER, GL_FRAGMENT_SHADER). */
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

    /** Deletes OpenGL shader object if valid. */
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

/**
 * RAII wrapper for an OpenGL program object.
 *
 * Manages lifecycle of linked shader program.
 */
struct GLProgram {
    bool ok = false, resetInDtor = true;
    std::string error;

    /** OpenGL program object handle (glCreateProgram result). */
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

    /** Deletes OpenGL program object if valid. */
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
/**
 * Shader loading, encoding processing, and OpenGL compilation utility API.
 */
class ShaderLoader {
public:
    static bool checkEncoding;
    static bool replaceUnknownNonASCII;

    /**
     * Loads a shader source file from disk and returns a heap-allocated source buffer.
     *
     * Behavior includes:
     * - Resolving input file path
     * - Reading file as binary
     * - Detecting and handling BOM (UTF-8 / UTF-16 / UTF-32)
     * - Converting non-ASCII encodings to ASCII where required
     * - Optionally replacing unknown non-ASCII bytes
     *
     * Ownership:
     * Returned ShaderLoadResult owns the allocated source buffer.
     */
    static ShaderLoadResult loadFile(const std::filesystem::path& inputPath);

    /**
     * Loads a shader source file and compiles it into an OpenGL shader object.
     *
     * Pipeline:
     * - loadFile() for source acquisition
     * - glCreateShader for object creation
     * - glShaderSource to upload source
     * - glCompileShader for compilation
     * - compilation validation via Inspector
     *
     * Ownership:
     * Returns GLShader which owns the OpenGL shader handle.
     */
    static GLShader loadShaderToGL(GLenum type, const std::filesystem::path& inputPath);

    /**
     * Loads vertex and fragment shaders, links them into an OpenGL program.
     *
     * Pipeline:
     * - loadShaderToGL(vertex)
     * - loadShaderToGL(fragment)
     * - glCreateProgram
     * - glAttachShader (vertex + fragment)
     * - glLinkProgram
     * - glDetachShader after linking
     * - validation via Inspector
     *
     * Ownership:
     * Returns GLProgram which owns the linked program object.
     */
    static GLProgram loadProgramToGL(const std::filesystem::path& vertexPath,
                                     const std::filesystem::path& fragmentPath);
};

/**
 * Returns whether current OpenGL context supports UTF-8 GLSL source strings.
 * Basically queries if the GLSL version is >= 4.2
 */
bool isGLSLSupportUTF8();

/**
 * Checks whether the input buffer contains any non-ASCII byte (MSB = 1).
 *
 * Uses 64-bit block scanning for performance optimization.
 *
 * @param data Input byte buffer
 * @param size Buffer size in bytes
 * @return true if any non-ASCII byte exists
 */
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

/**
 * Replaces all non-ASCII bytes in the buffer with space characters.
 *
 * Operates in 8-byte chunks for performance optimization.
 */
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