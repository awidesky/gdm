#ifndef GLUTIL_TEXTURE_HPP
#define GLUTIL_TEXTURE_HPP

#include <glutil/gl.hpp>
#include <string>
#include <vector>
#include <cstddef>
#include <filesystem>
#include <utility>

namespace glutil {

/**
 * Represents a single mipmap level inside a texture (typically DDS/BC formats).
 * Offset is relative to the start of the raw file buffer (fileData).
 * Size is the byte size of the mip level.
 */
struct MipLevel {
    // Offset from base file buffer (used for pointer arithmetic: fileData + offset)
    size_t  offset;
    // Size in bytes of this mip level (used for GL compressed upload)
    GLsizei size;
    GLsizei width;
    GLsizei height;
};

/**
 * CPU-side uncompressed texture image.
 *
 * Ownership:
 * - pixels is heap allocated and owned by this object
 * - freed on destroy() or move
 */
struct TextureImage {
    bool ok = false;
    std::string error;

    /**
     * Raw pixel pointer (owned).
     * Format depends on loader (stb_image).
     */
    const void* data() const { return pixels; }
    /** Image width in pixels. */
    GLsizei width() const { return w; }
    /** Image height in pixels. */
    GLsizei height() const { return h; }
    /**
     * Pixel format for glTexImage2D.
     *
     * Encodes channel count is typically determined by stb_image like:
     * - GL_RED   (1 channel)
     * - GL_RG    (2 channels)
     * - GL_RGB   (3 channels)
     * - GL_RGBA  (4 channels)
     */
    GLenum format() const { return fmt; }
    /** OpenGL internal format used for texture storage. */
    GLint internalFormat() const { return internalFmt; }

    ~TextureImage() { destroy(); }
    TextureImage() = default;
    TextureImage(const TextureImage&)            = delete;
    TextureImage& operator=(const TextureImage&) = delete;
    TextureImage(TextureImage&& o) noexcept            { moveFrom(std::move(o)); }
    TextureImage& operator=(TextureImage&& o) noexcept {
        if (this != &o) { destroy(); moveFrom(std::move(o)); }
        return *this;
    }

private:
    friend class ImageLoader;

    /** Raw pixel buffer allocated by image loader (stb_image). */
    unsigned char* pixels = nullptr;
    GLsizei w = 0, h = 0;
    GLenum fmt = 0;
    GLint internalFmt = 0;

    /** Releases owned pixel memory and resets state. */
    void destroy() noexcept {
        delete[] pixels; pixels = nullptr;
        w = h = 0; fmt = 0; internalFmt = 0;
        ok = false; error.clear();
    }
    /** Moves ownership of pixel buffer and metadata. */
    void moveFrom(TextureImage&& o) noexcept {
        ok = o.ok; error = std::move(o.error);
        pixels = o.pixels; w = o.w; h = o.h;
        fmt = o.fmt; internalFmt = o.internalFmt;
        o.pixels = nullptr; o.ok = false;
        o.w = o.h = 0; o.fmt = 0; o.internalFmt = 0;
    }
};

/**
 * CPU-side DDS/KTX compressed texture container.
 *
 * Important ownership rule:
 * - fileData must remain valid while mipLevels reference its memory.
 * - mipLevels store offsets into fileData (no separate allocation per mip).
 */
struct TextureDDS {
    bool ok = false;
    std::string error;

    /** Raw DDS/KTX file buffer (owned). */
    const unsigned char* data() const { return fileData; }
    /** Base texture width. */
    GLsizei width() const { return w; }
    /** Base texture height. */
    GLsizei height() const { return h; }
    /**
     * Compressed internal format (BC1/BC2/BC3 only).
     *
     * Used directly with glCompressedTexImage2D.
     */
    GLenum format() const { return fmt; }
    /** Mipmap chain description. */
    const std::vector<MipLevel>& mips() const { return mipLevels; }

    ~TextureDDS() { destroy(); }
    TextureDDS() = default;
    TextureDDS(const TextureDDS&) = delete;
    TextureDDS& operator=(const TextureDDS&) = delete;
    TextureDDS(TextureDDS&& o) noexcept { moveFrom(std::move(o)); }
    TextureDDS& operator=(TextureDDS&& o) noexcept {
        if (this != &o) { destroy(); moveFrom(std::move(o)); }
        return *this;
    }

private:
    friend class ImageLoader;

    /**
     * Raw DDS file buffer.
     * All mip data pointers are offsets into this buffer.
     */
    unsigned char* fileData  = nullptr;
    GLsizei w = 0, h = 0;
    GLenum fmt = 0;
    /** Mipmap levels referencing fileData via offset. */
    std::vector<MipLevel> mipLevels;

    /** Frees file buffer and clears mip metadata. */
    void destroy() noexcept {
        delete[] fileData; fileData = nullptr;
        w = h = 0; fmt = 0;
        mipLevels.clear();
        ok = false; error.clear();
    }
    /** Moves ownership of DDS buffer and mip metadata. */
    void moveFrom(TextureDDS&& o) noexcept {
        ok = o.ok; error = std::move(o.error);
        fileData = o.fileData; w = o.w; h = o.h;
        fmt = o.fmt; mipLevels = std::move(o.mipLevels);
        o.fileData = nullptr; o.ok = false;
        o.w = o.h = 0; o.fmt = 0;
    }
};

/**
 * GPU-side 2D texture wrapper.
 *
 * Manages OpenGL texture object lifetime (RAII).
 */
struct GLTexture2D {
    bool ok = false, resetInDtor = true;
    std::string error;

    GLuint id = 0;       // OpenGL texture object
    GLsizei w = 0;       // width
    GLsizei h = 0;       // height
    GLenum format = 0;   // pixel/internal format

    GLTexture2D() = default;
    ~GLTexture2D() { if (resetInDtor) reset(); }

    GLTexture2D(const GLTexture2D&) = delete;
    GLTexture2D& operator=(const GLTexture2D&) = delete;

    GLTexture2D(GLTexture2D&& other) noexcept { moveFrom(std::move(other)); }
    GLTexture2D& operator=(GLTexture2D&& other) noexcept {
        if (this != &other) {
            reset();
            moveFrom(std::move(other));
        }
        return *this;
    }

    /** Deletes OpenGL texture object. */
    void reset() noexcept {
        if (id != 0) {
            glDeleteTextures(1, &id);
            id = 0;
        }
        w = 0;
        h = 0;
        format = 0;
    }

private:
    /** Moves OpenGL texture ownership. */
    void moveFrom(GLTexture2D&& other) noexcept {
        ok = other.ok;
        error = std::move(other.error);
        id = other.id;
        w = other.w;
        h = other.h;
        format = other.format;

        other.ok = false;
        other.error.clear();
        other.id = 0;
        other.w = 0;
        other.h = 0;
        other.format = 0;
    }
};

/**
 * Image loading utility.
 *
 * Supports:
 * - standard image formats (stb_image)
 * - DDS compressed textures (BC formats)
 */
class ImageLoader {
public:
    static bool isDDS(const std::filesystem::path& path);
    static TextureImage loadImage(const std::filesystem::path& path, bool flipV = true, int desiredChannels = 0);
    static TextureDDS loadDDS(const std::filesystem::path& path, bool flipV = true);

    static GLTexture2D loadImageToGL(const std::filesystem::path& path, bool flipV = true, int desiredChannels = 0);
    static GLTexture2D loadDDSToGL(const std::filesystem::path& path, bool flipV = true);
};

} // namespace glutil

#endif // GLUTIL_TEXTURE_HPP
