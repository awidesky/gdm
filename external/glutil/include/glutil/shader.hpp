#ifndef GLUTIL_SHADER_HPP
#define GLUTIL_SHADER_HPP

#include <glutil/gl.hpp>

#include <string>
#include <utility>

struct ShaderLoadResult {
    bool ok = false;
    std::string error;

    const GLchar** string() { return &source; }
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
    static ShaderLoadResult LoadFile(const char* inputPath);
};

#endif // GLUTIL_SHADER_HPP
