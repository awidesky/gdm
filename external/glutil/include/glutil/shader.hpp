#ifndef GLUTIL_SHADER_HPP
#define GLUTIL_SHADER_HPP

#include <glutil/gl.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>

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

class ShaderLoader {
public:
    static bool checkEncoding;
    static bool replaceUnknownCharsetNonASCII;
    static ShaderLoadResult loadFile(const char* inputPath);
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
