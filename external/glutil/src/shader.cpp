#include <glutil/shader.hpp>
#include <glutil/logging.hpp>

#include <filesystem>
#include <fstream>
#include <cstring>
#include <cstdint>
#include <system_error>

namespace glutil {

namespace fs = std::filesystem;

static bool isGLSLSupportUTF8() {
    const GLubyte* glslVersionRaw = glGetString(GL_SHADING_LANGUAGE_VERSION);
    if (glslVersionRaw == 0)
        return false;

    const char* version = reinterpret_cast<const char*>(glslVersionRaw);
    if (version[0] < '0' || version[0] > '9' || version[2] < '0' || version[2] > '9')
        return false;

    const int major = version[0] - '0';
    const int minor = version[2] - '0';

    return (major > 4) || (major == 4 && minor >= 2);;
}

static void utf8_to_ascii_replace(GLchar* data, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        unsigned char ch = static_cast<unsigned char>(data[i]);
        if (ch & 0x80) data[i] = ' ';
    }
}

// TODO : instead of re-writing, in-place substitution to space?
// TODO : possible memory leak senario?
static GLchar* utf16_to_ascii(const GLchar* data, size_t size, bool littleEndian, size_t& outSize) {
    GLchar* out = new GLchar[(size / 2) + 1];
    size_t out_i = 0;

    for (size_t i = 0; i + 1 < size;) {
        uint16_t u;
        if (littleEndian)
            u = static_cast<uint8_t>(data[i]) | (static_cast<uint16_t>(static_cast<uint8_t>(data[i + 1])) << 8);
        else
            u = (static_cast<uint16_t>(static_cast<uint8_t>(data[i])) << 8) | static_cast<uint8_t>(data[i + 1]);

        i += 2;

        if (u >= 0xD800 && u <= 0xDFFF) {
            if (i + 1 < size) i += 2;
            continue;
        }

        if (u <= 0x7F) out[out_i++] = static_cast<GLchar>(u);
    }

    outSize = out_i;
    out[out_i] = '\0';
    return out;
}

static GLchar* utf32_to_ascii(const GLchar* data, size_t size, bool littleEndian, size_t& outSize) {
    GLchar* out = new GLchar[(size / 4) + 1];
    size_t out_i = 0;

    for (size_t i = 0; i + 3 < size; i += 4) {
        uint32_t u;
        if (littleEndian)
            u = static_cast<uint8_t>(data[i]) |
                (static_cast<uint32_t>(static_cast<uint8_t>(data[i + 1])) << 8) |
                (static_cast<uint32_t>(static_cast<uint8_t>(data[i + 2])) << 16) |
                (static_cast<uint32_t>(static_cast<uint8_t>(data[i + 3])) << 24);
        else
            u = (static_cast<uint32_t>(static_cast<uint8_t>(data[i])) << 24) |
                (static_cast<uint32_t>(static_cast<uint8_t>(data[i + 1])) << 16) |
                (static_cast<uint32_t>(static_cast<uint8_t>(data[i + 2])) << 8) |
                static_cast<uint8_t>(data[i + 3]);

        if (u <= 0x7F) out[out_i++] = static_cast<GLchar>(u);
    }

    outSize = out_i;
    out[out_i] = '\0';
    return out;
}

static bool hasNonASCII(const char* data, size_t size) {
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



bool ShaderLoader::checkEncoding = true;

ShaderLoadResult ShaderLoader::loadFile(const char* inputPath) {
    ShaderLoadResult result;

    if (inputPath == nullptr) {
        result.error = "inputPath is nullptr";
        return result;
    }

    std::string resolvedPath(inputPath);

    std::error_code ec;
    const auto fileSize = fs::file_size(resolvedPath, ec);
    if (ec) {
        result.error = "failed to get file size: " + resolvedPath + " (" + ec.message() + ")";
        return result;
    }

    std::ifstream file(resolvedPath, std::ios::binary);
    if (!file.is_open()) {
        result.error = "failed to open file: " + resolvedPath;
        return result;
    }

    GLchar* buffer = new GLchar[static_cast<size_t>(fileSize) + 1];
    if (!file.read(buffer, static_cast<std::streamsize>(fileSize))) {
        delete[] buffer;
        result.error = "failed to read file: " + resolvedPath;
        return result;
    }

    buffer[static_cast<size_t>(fileSize)] = '\0';

    GLchar* converted = buffer;
    size_t convertedSize = static_cast<size_t>(fileSize);

    if (checkEncoding) {
        LOG_INFO() << "Checking text encoding of file: " << resolvedPath;
        if (fileSize >= 3 &&
                static_cast<unsigned char>(buffer[0]) == 0xEF &&
                static_cast<unsigned char>(buffer[1]) == 0xBB &&
                static_cast<unsigned char>(buffer[2]) == 0xBF) {
            buffer[0] = buffer[1] = buffer[2] = ' ';
            LOG_WARNING() << "Detected charset: UTF-8 BOM (replacing BOM to space character...)";
            // In GLSL 4.20+, noo-ASCII character is allowed in comment,
            // as long as the source is UTF-8. So, we don't replace non-ASCII characters.
            // NOTE : When non-ASCII character is used outside a comment,
            //        a compile-time error is clarified in GLSL 4.30 spec.
            //        But 4.20 spec does not clarify it.
            if (isGLSLSupportUTF8()) {
                LOG_WARNING() << "Current GLSL version allows non-ASCII character in UTF-8 in comments.";
                LOG_WARNING() << "Make sure the non-ASCII chracter(s) is NOT outside of comments.";
            } else {
                LOG_WARNING() << "Current GLSL version allows ASCII only (replacing all non-ASCII to space character...)";
                utf8_to_ascii_replace(buffer + 3, fileSize - 3);
            }
        } else if (fileSize >= 4 &&
                   static_cast<unsigned char>(buffer[0]) == 0xFF &&
                   static_cast<unsigned char>(buffer[1]) == 0xFE &&
                   static_cast<unsigned char>(buffer[2]) == 0x00 &&
                   static_cast<unsigned char>(buffer[3]) == 0x00) {
            LOG_WARNING() << "Detected charset: UTF-32 LE (replacing all non-ASCII to space character...)";
            converted = utf32_to_ascii(buffer + 4, static_cast<size_t>(fileSize) - 4, true, convertedSize);
            delete[] buffer;
        } else if (fileSize >= 4 &&
                   static_cast<unsigned char>(buffer[0]) == 0x00 &&
                   static_cast<unsigned char>(buffer[1]) == 0x00 &&
                   static_cast<unsigned char>(buffer[2]) == 0xFE &&
                   static_cast<unsigned char>(buffer[3]) == 0xFF) {
            LOG_WARNING() << "Detected charset: UTF-32 BE (replacing all non-ASCII to space character...)";
            converted = utf32_to_ascii(buffer + 4, static_cast<size_t>(fileSize) - 4, false, convertedSize);
            delete[] buffer;
        } else if (fileSize >= 2 &&
                   static_cast<unsigned char>(buffer[0]) == 0xFF &&
                   static_cast<unsigned char>(buffer[1]) == 0xFE) {
            LOG_WARNING() << "Detected charset: UTF-16 LE (replacing all non-ASCII to space character...)";
            converted = utf16_to_ascii(buffer + 2, static_cast<size_t>(fileSize) - 2, true, convertedSize);
            delete[] buffer;
        } else if (fileSize >= 2 &&
                   static_cast<unsigned char>(buffer[0]) == 0xFE &&
                   static_cast<unsigned char>(buffer[1]) == 0xFF) {
            LOG_WARNING() << "Detected charset: UTF-16 BE (replacing all non-ASCII to space character...)";
            converted = utf16_to_ascii(buffer + 2, static_cast<size_t>(fileSize) - 2, false, convertedSize);
            delete[] buffer;
        } else {
            if (hasNonASCII(buffer, static_cast<size_t>(fileSize))) {
                LOG_WARNING() << "Detected charset: unknown (non-ASCII bytes found!)";
                LOG_WARNING() << "Remove non-ASCII text, or compile error might occur!";
            }  else {
                LOG_INFO() << "Source is ASCII-only content, no conversion required";
            }
        }
    }

    result.source = converted;
    result.len = static_cast<GLint>(convertedSize);
    result.ok = true;
    return result;
}

} // namespace glutil
