// glutil/texture.cpp
//
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define DDSKTX_IMPLEMENT
#include "dds-ktx.h"

#include <glutil/glutil.hpp>
#include <glutil/logging.hpp>
#include <glutil/texture.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <system_error>
#include <vector>

namespace glutil {

namespace fs = std::filesystem;

bool ImageLoader::isDDS(const char* path) {
    if (!path) return false;

    PathResolveResult pr = pathResolve(path);
    std::string targetPath = pr.success ? pr.resolvedPath : path;

    std::ifstream file(targetPath, std::ios::binary);
    if (file.is_open()) {
        char magic[4] = {0};
        if (file.read(magic, 4)) {
            return (magic[0] == 'D' && magic[1] == 'D' && magic[2] == 'S' && magic[3] == ' ');
        }
    }

    std::string ext = fs::path(targetPath).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
    return ext == ".dds";
}

TextureImage ImageLoader::loadImage(const char* path, bool flipV) {
    TextureImage result;

    // [PathResolve]
    PathResolveResult pr = pathResolve(path);
    if (!pr.success) {
        result.error = "path resolve failed: " + pr.message;
        LOG_ERROR() << "[TextureImage] " << result.error;
        return result;
    }

    stbi_set_flip_vertically_on_load(flipV ? 1 : 0);

    int w = 0, h = 0, c = 0;
    unsigned char* raw = stbi_load(pr.resolvedPath.c_str(), &w, &h, &c, 0);
    if (!raw) {
        result.error = stbi_failure_reason() ? stbi_failure_reason() : "unknown stb_image error";
        LOG_ERROR() << "[TextureImage] Load failed: " << pr.resolvedPath << " (" << result.error << ")";
        return result;
    }

    // TODO : there are many other types like RG, BRG, etc. let user choose? or remove fmt parameter.
    GLenum fmt = 0;
    GLint internalFmt = 0;
    switch (c) {
        case 1:
            fmt = GL_RED;
            internalFmt = GL_R8;
            break;
        case 3:
            fmt = GL_RGB;
            internalFmt = GL_RGB8;
            break;
        case 4:
            fmt = GL_RGBA;
            internalFmt = GL_RGBA8;
            break;
        default:
            result.error = "Unsupported number of channels: " + std::to_string(c);
            LOG_ERROR() << "[TextureImage] " << result.error;
            stbi_image_free(raw);
            return result;
    }

    // stbi_load internally uses malloc.
    // Copy to new memory and then immediately free with stbi_image_free. Destructor deletes new memory with delete[].
    const size_t sz = static_cast<size_t>(w * h * c);
    result.pixels = new unsigned char[sz];
    std::memcpy(result.pixels, raw, sz);
    stbi_image_free(raw);

    result.w = static_cast<GLsizei>(w);
    result.h = static_cast<GLsizei>(h);
    result.fmt = fmt;
    result.internalFmt = internalFmt;
    result.ok = true;

    LOG_INFO() << "[TextureImage] Load succeeded: " << pr.resolvedPath;
    LOG_INFO() << "[TextureImage]                 (" << w << "x" << h << ", flipV=" << flipV << ")";
    return result;
}

// [toGLFormat design reason]
static GLenum toGLFormat(ddsktx_format fmt, unsigned int flags) {
    switch (fmt) {
        // BC1(DXT1) alpha bit usage determines RGB vs RGBA.
        // If alpha bit is not actually used (XRGB/alpha_x), should be RGB.
        case DDSKTX_FORMAT_BC1:
#if defined(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) && defined(GL_COMPRESSED_RGB_S3TC_DXT1_EXT)
            return ((flags & DDSKTX_TEXTURE_FLAG_ALPHA) && !(flags & DDSKTX_TEXTURE_FLAG_ALPHA_X))
                 ? GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
                 : GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
#else
            LOG_WARNING() << "[TextureDDS] S3TC symbola are missing!"
                            << "Verify GL_EXT_texture_compression_s3tc is loaded correctly.\n"
                            << "GL_EXT_texture_compression_s3tc is generally ubiquitous, "
                            << "so using hardcoded enum values for now.";
            return ((flags & DDSKTX_TEXTURE_FLAG_ALPHA) && !(flags & DDSKTX_TEXTURE_FLAG_ALPHA_X))
                 ? 0x83F1
                 : 0x83F0;
#endif
        case DDSKTX_FORMAT_BC2:
#if defined(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT)
            return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
#else
            return 0x83F2;
#endif
        case DDSKTX_FORMAT_BC3:
#if defined(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
            return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
#else
            return 0x83F3;
#endif
        default: return 0;
    }
}

static GLsizei compressedBlockSize(ddsktx_format fmt) {
    switch (fmt) {
        case DDSKTX_FORMAT_BC1: return 8;
        case DDSKTX_FORMAT_BC2:
        case DDSKTX_FORMAT_BC3: return 16;
        default: return 0;
    }
}

static void flipBC1BlockVertical(unsigned char* block) {
    std::uint32_t indices = 0;
    std::memcpy(&indices, block + 4, sizeof(indices));

    const std::uint32_t row0 = (indices & 0x000000FFu) << 24;
    const std::uint32_t row1 = (indices & 0x0000FF00u) << 8;
    const std::uint32_t row2 = (indices & 0x00FF0000u) >> 8;
    const std::uint32_t row3 = (indices & 0xFF000000u) >> 24;

    indices = row0 | row1 | row2 | row3;
    std::memcpy(block + 4, &indices, sizeof(indices));
}

static void flipBC2BlockVertical(unsigned char* block) {
    unsigned char row[2];

    std::memcpy(row, block + 0, sizeof(row));
    std::memcpy(block + 0, block + 6, sizeof(row));
    std::memcpy(block + 6, row, sizeof(row));

    std::memcpy(row, block + 2, sizeof(row));
    std::memcpy(block + 2, block + 4, sizeof(row));
    std::memcpy(block + 4, row, sizeof(row));

    flipBC1BlockVertical(block + 8);
}

static void flipBC3AlphaVertical(unsigned char* block) {
    std::uint64_t alphaBits = 0;
    // BC3 alpha block layout: 2 bytes alpha0/alpha1, then 6 bytes of 3-bit indices x16
    std::memcpy(&alphaBits, block + 2, 6);

    std::uint8_t alphaValues[16] = {};
    for (int i = 0; i < 16; ++i) {
        alphaValues[i] = static_cast<std::uint8_t>((alphaBits >> (3 * i)) & 0x7u);
    }

    std::uint8_t flipped[16] = {};
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            flipped[(3 - row) * 4 + col] = alphaValues[row * 4 + col];
        }
    }

    alphaBits = 0;
    for (int i = 0; i < 16; ++i) {
        alphaBits |= static_cast<std::uint64_t>(flipped[i] & 0x7u) << (3 * i);
    }

    std::memcpy(block + 2, &alphaBits, 6);
}

static void flipBC3BlockVertical(unsigned char* block) {
    flipBC3AlphaVertical(block);
    flipBC1BlockVertical(block + 8);
}

static void flipCompressedMipVertical(unsigned char* mipData, GLsizei width, GLsizei height, ddsktx_format fmt) {
    const GLsizei blockSize = compressedBlockSize(fmt);
    if (blockSize == 0) {
        return;
    }

    const GLsizei blocksWide = std::max<GLsizei>(1, (width + 3) / 4);
    const GLsizei blocksHigh = std::max<GLsizei>(1, (height + 3) / 4);
    const size_t rowBytes = static_cast<size_t>(blocksWide) * static_cast<size_t>(blockSize);

    std::vector<unsigned char> flipped(rowBytes * static_cast<size_t>(blocksHigh));

    for (GLsizei row = 0; row < blocksHigh; ++row) {
        unsigned char* dstRow = flipped.data() + static_cast<size_t>(blocksHigh - 1 - row) * rowBytes;
        const unsigned char* srcRow = mipData + static_cast<size_t>(row) * rowBytes;
        std::memcpy(dstRow, srcRow, rowBytes);

        for (GLsizei col = 0; col < blocksWide; ++col) {
            unsigned char* block = dstRow + static_cast<size_t>(col) * static_cast<size_t>(blockSize);
            switch (fmt) {
                case DDSKTX_FORMAT_BC1: flipBC1BlockVertical(block); break;
                case DDSKTX_FORMAT_BC2: flipBC2BlockVertical(block); break;
                case DDSKTX_FORMAT_BC3: flipBC3BlockVertical(block); break;
                default: break;
            }
        }
    }

    std::memcpy(mipData, flipped.data(), flipped.size());
}

TextureDDS ImageLoader::loadDDS(const char* path, bool flipV) {
    TextureDDS result;

    if (!path) {
        result.error = "path is nullptr";
        return result;
    }

    PathResolveResult pr = pathResolve(path);
    if (!pr.success) {
        result.error = "Path resolution failed: " + pr.message;
        LOG_ERROR() << "[TextureDDS] " << result.error;
        return result;
    }

    std::error_code ec;
    const auto fileSize = fs::file_size(pr.resolvedPath, ec);
    if (ec) {
        result.error = "Failed to get file size(" + ec.message() + "): " + pr.resolvedPath;
        LOG_ERROR() << "[TextureDDS] " << result.error;
        return result;
    }

    std::ifstream file(pr.resolvedPath, std::ios::binary);
    if (!file.is_open()) {
        result.error = "Failed to open file: " + pr.resolvedPath;
        LOG_ERROR() << "[TextureDDS] " << result.error;
        return result;
    }

    result.fileData = new unsigned char[fileSize];
    if (!file.read(reinterpret_cast<char*>(result.fileData), static_cast<std::streamsize>(fileSize))) {
        result.error = "Failed to read file: " + pr.resolvedPath;
        LOG_ERROR() << "[TextureDDS] " << result.error;
        delete[] result.fileData;
        result.fileData = nullptr;
        return result;
    }

    ddsktx_texture_info tc = {0};
    ddsktx_error err = {};
    if (!ddsktx_parse(&tc, result.fileData, static_cast<int>(fileSize), &err)) {
        result.error = std::string("ddsktx_parse failed: ") + err.msg;
        LOG_ERROR() << "[TextureDDS] " << result.error << ": " << pr.resolvedPath;
        delete[] result.fileData;
        result.fileData = nullptr;
        return result;
    }

    const GLenum glFmt = toGLFormat(tc.format, tc.flags);
    if (glFmt == 0) {
        result.error = "Unsupported DDS format (only BC1/BC2/BC3 supported)";
        LOG_ERROR() << "[TextureDDS] " << result.error << ": " << pr.resolvedPath;
        delete[] result.fileData;
        result.fileData = nullptr;
        return result;
    }

    result.mipLevels.reserve(tc.num_mips);
    for (int i = 0; i < tc.num_mips; ++i) {
        ddsktx_sub_data sub;
        ddsktx_get_sub(&tc, &sub, result.fileData, static_cast<int>(fileSize), 0, 0, i);

        // If sub.buff < fileData then negative ptrdiff_t - casting to size_t is UB.
        // Can occur with corrupted DDS files.
        const auto* ptr = reinterpret_cast<const unsigned char*>(sub.buff);
        if (ptr < result.fileData) {
            result.error = "DDS mip offset out of range: level=" + std::to_string(i);
            LOG_ERROR() << "[TextureDDS] " << result.error;
            delete[] result.fileData;
            result.fileData = nullptr;
            return result;
        }

        result.mipLevels.push_back({static_cast<size_t>(ptr - result.fileData), static_cast<GLsizei>(sub.size_bytes),
                                    static_cast<GLsizei>(sub.width), static_cast<GLsizei>(sub.height)});
    }

    if (flipV) {
        for (const MipLevel& mip : result.mipLevels) {
            flipCompressedMipVertical(result.fileData + mip.offset, mip.width, mip.height, tc.format);
        }
    }

    result.w = static_cast<GLsizei>(tc.width);
    result.h = static_cast<GLsizei>(tc.height);
    result.fmt = glFmt;
    result.ok = true;

    LOG_INFO() << "[TextureDDS] Load succeeded: " << pr.resolvedPath;
    LOG_INFO() << "[TextureDDS]                 (" << tc.width << "x" << tc.height
               << ", mips=" << tc.num_mips << ", flipV=" << flipV << ")";
    return result;
}

} // namespace glutil