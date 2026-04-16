#ifndef GLUTIL_TEXTURE_HPP
#define GLUTIL_TEXTURE_HPP

#include <glutil/gl.hpp>
#include <string>
#include <vector>
#include <cstddef>

namespace glutil {

struct MipLevel {
    // offset : size_t - for fileData pointer arithmetic (data() + offset). platform's pointer size
    size_t  offset;
    // size --> glCompressedTexImage2D(GLsizei imageSize)
    GLsizei size;
    // width --> glCompressedTexImage2D(GLsizei width)
    GLsizei width;
    // height --> glCompressedTexImage2D(GLsizei height)
    GLsizei height;
};

struct TextureImage {
    bool ok = false;
    std::string error;

    const void* data() const { return pixels; }
    // width(), height() --> glTexImage2D(GLsizei width/height)
    GLsizei width() const { return w; }
    GLsizei height() const { return h; }
    // format() --> glTexImage2D(GLenum format)
    // format() already encodes channel information
    // GL_RED=1ch, GL_RGB=3ch, GL_RGBA=4ch
    GLenum format() const { return fmt; }
    // internalFormat() --> glTexImage2D(GLint internalFormat) 
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

    // stb_image parses file to pixel data only.
    unsigned char* pixels = nullptr; // new[], delete[] in destroy()
    GLsizei w = 0, h = 0;
    GLenum fmt = 0;
    GLint internalFmt = 0;

    void destroy() noexcept {
        delete[] pixels; pixels = nullptr;
        w = h = 0; fmt = 0; internalFmt = 0;
        ok = false; error.clear();
    }
    void moveFrom(TextureImage&& o) noexcept {
        ok = o.ok; error = std::move(o.error);
        pixels = o.pixels; w = o.w; h = o.h;
        fmt = o.fmt; internalFmt = o.internalFmt;
        o.pixels = nullptr; o.ok = false;
        o.w = o.h = 0; o.fmt = 0; o.internalFmt = 0;
    }
};

// dds-ktx parses DDS file to compressed block data.
// toGLFormat() only allows BC1/BC2/BC3 (all compressed) and returns error for others.
// Since, format is always compressed.
struct TextureDDS {
    bool ok = false;
    std::string error;

    const unsigned char* data() const { return fileData; }
    // width(), height() return type --> glCompressedTexImage2D(GLsizei width/height)
    GLsizei width() const { return w; }
    GLsizei height() const { return h; }
    // format() --> glCompressedTexImage2D(GLenum internalformat)
    GLenum format() const { return fmt; }
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

    // dds-ktx is a no-allocation library.
    // sub_data.buff returned by ddsktx_get_sub() is a pointer inside original buffer (fileData).
    // MipLevel.offset is calculated as sub_data.buff - fileData.
    // fileData must be kept until destructor so that dds.data() + mip.offset remains valid.
    unsigned char* fileData  = nullptr; // new[], delete[] in destroy()
    GLsizei w = 0, h = 0;
    GLenum fmt = 0;
    std::vector<MipLevel> mipLevels;

    void destroy() noexcept {
        delete[] fileData; fileData = nullptr;
        w = h = 0; fmt = 0;
        mipLevels.clear();
        ok = false; error.clear();
    }
    void moveFrom(TextureDDS&& o) noexcept {
        ok = o.ok; error = std::move(o.error);
        fileData = o.fileData; w = o.w; h = o.h;
        fmt = o.fmt; mipLevels = std::move(o.mipLevels);
        o.fileData = nullptr; o.ok = false;
        o.w = o.h = 0; o.fmt = 0;
    }
};

class ImageLoader {
public:
    static TextureImage loadImage(const char* path, bool flipV = true);
    static TextureDDS loadDDS(const char* path, bool flipV = true);
};

} // namespace glutil

#endif // GLUTIL_TEXTURE_HPP
