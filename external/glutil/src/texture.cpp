// glutil/texture.cpp
//
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define DDSKTX_IMPLEMENT
#include "dds-ktx.h"

#include <glutil/glutil.hpp>
#include <glutil/logging.hpp>
#include <glutil/texture.hpp>
#include <glutil/debug.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <system_error>
#include <vector>

namespace glutil {

namespace fs = std::filesystem;

bool ImageLoader::isDDS(const std::filesystem::path& path) {
    const PathResolveResult pr = pathResolve(path);
    if (!pr.success) {
        LOG_ERROR() << "[ImageLoader::isDDS] Path resolution failed: " + pr.message;;
        return false;
    }

    std::ifstream file(pr.resolvedPath, std::ios::binary);
    if (file.is_open()) {
        char magic[4] = {0};
        if (file.read(magic, 4)) {
            return (magic[0] == 'D' && magic[1] == 'D' && magic[2] == 'S' && magic[3] == ' ');
        }
    }

    std::string ext = fs::path(pr.resolvedPath).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
    return ext == ".dds";
}

TextureImage ImageLoader::loadImage(const std::filesystem::path& path, bool flipV, int desiredChannels) {
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
    unsigned char* raw = stbi_load(pr.resolvedPath.c_str(), &w, &h, &c, desiredChannels);
    if (!raw) {
        result.error = stbi_failure_reason() ? stbi_failure_reason() : "unknown stb_image error";
        LOG_ERROR() << "[TextureImage] Load failed: " << pr.resolvedPath << " (" << result.error << ")";
        return result;
    }
    
    const int channels = desiredChannels != 0 ? desiredChannels : c;
    switch (channels) {
        case 1:
            result.fmt = GL_RED;
            result.internalFmt = GL_R8;
            break;
        case 2:
            result.fmt = GL_RG;
            result.internalFmt = GL_RG8;
            break;
        case 3:
            result.fmt = GL_RGB;
            result.internalFmt = GL_RGB8;
            break;
        case 4:
            result.fmt = GL_RGBA;
            result.internalFmt = GL_RGBA8;
            break;
        default:
            result.error = "Unsupported number of channels: " + std::to_string(channels);
            LOG_ERROR() << "[TextureImage] " << result.error;
            stbi_image_free(raw);
            return result;
    }

    const int rowBytes = w * channels;
    GLint unpackAlignment = 0;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpackAlignment);
    if ((unpackAlignment > 0) && (rowBytes % unpackAlignment) != 0) {
        LOG_WARNING() << "[TextureImage] Potential GL_UNPACK_ALIGNMENT issue detected.";
        LOG_WARNING() << "[TextureImage] Texture row size is not 4-byte aligned: " << rowBytes << " bytes per row"
                      << " (width=" << w << ", channels=" << (channels) << ")";
        LOG_WARNING() << "[TextureImage] Current GL_UNPACK_ALIGNMENT = " << unpackAlignment;
        LOG_WARNING() << "[TextureImage] If texture appears diagonally skewed or torn,";
        LOG_WARNING() << "[TextureImage] ensure GL_UNPACK_ALIGNMENT is set to 1 before glTexImage2D:";
        LOG_WARNING() << "[TextureImage]     glBindTexture(GL_TEXTURE_2D, texID);";
        LOG_WARNING() << "[TextureImage]     glPixelStorei(GL_UNPACK_ALIGNMENT, 1);";
        LOG_WARNING() << "[TextureImage]     glTexImage2D(...);";
    }

    // stbi_load internally uses malloc.
    // Copy to new memory and then immediately free with stbi_image_free. Destructor deletes new memory with delete[].
    const size_t sz = static_cast<size_t>(w * h * channels);
    result.pixels = new unsigned char[sz];
    std::memcpy(result.pixels, raw, sz);
    stbi_image_free(raw);

    result.w = static_cast<GLsizei>(w);
    result.h = static_cast<GLsizei>(h);
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

TextureDDS ImageLoader::loadDDS(const std::filesystem::path& path, bool flipV) {
    TextureDDS result;
    const PathResolveResult pr = pathResolve(path);
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

GLTexture2D ImageLoader::loadImageToGL(const std::filesystem::path& path, bool flipV, int desiredChannels) {
    GLTexture2D out;

    TextureImage image = loadImage(path, flipV, desiredChannels);
    if (!image.ok) {
        out.error = image.error;
        return out;
    }

    GLint unpackAlignment = 0;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpackAlignment);
    int channels = 0;
    switch (image.format()) {
        case GL_RED: channels = 1; break;
        case GL_RG: channels = 2; break;
        case GL_RGB: channels = 3; break;
        case GL_RGBA: channels = 4; break;
        default: 
            out.error = std::string("Unexpected image format from stb_image : ") + glTextureFormatToString(image.format());
            return out;
    }
    const int rowBytes = image.width() * channels;
    const bool needsTightAlignment = (channels > 0) && (unpackAlignment > 0) && ((rowBytes % unpackAlignment) != 0);
    if (needsTightAlignment) {
        LOG_WARNING() << "[TextureImage] Temporarily setting GL_UNPACK_ALIGNMENT to 1 for this upload.";
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    }

    glGenTextures(1, &out.id);
    glBindTexture(GL_TEXTURE_2D, out.id);

    // Set reasonable default sampling/wrap parameters and generate mipmaps.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, image.internalFormat(), image.width(), image.height(),
                 0, image.format(), GL_UNSIGNED_BYTE, image.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    // Set max mip level to the computed number of levels to avoid undefined sampling
    // on drivers that query GL_TEXTURE_MAX_LEVEL.
    int maxLevel = 0;
    int larger = std::max(image.width(), image.height());
    while (larger > 1) { larger >>= 1; ++maxLevel; }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, maxLevel);

    if (needsTightAlignment) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, unpackAlignment);
    }

    const std::string label = "Texture2D:" + path.filename().string();
    glutil::debug::labelGLobject(GL_TEXTURE, out.id, label);

    out.w = image.width();
    out.h = image.height();
    out.format = image.format();
    out.ok = true;
    return out;
}

GLTexture2D ImageLoader::loadDDSToGL(const std::filesystem::path& path, bool flipV) {
    GLTexture2D out;

    TextureDDS dds = loadDDS(path, flipV);
    if (!dds.ok) {
        out.error = dds.error;
        return out;
    }


    glGenTextures(1, &out.id);
    glBindTexture(GL_TEXTURE_2D, out.id);

    // Set reasonable default sampling/wrap parameters. DDS contains its own mip levels,
    // so ensure the sampler expects mipmapped data.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    const GLenum internalFmt = dds.format();
    const auto& mips = dds.mips();
    for (size_t level = 0; level < mips.size(); ++level) {
        const MipLevel& mip = mips[level];
        glCompressedTexImage2D(GL_TEXTURE_2D, static_cast<GLint>(level), internalFmt,
                               mip.width, mip.height, 0, mip.size,
                               dds.data() + mip.offset);
    }

    // Explicitly set the base/max mip level based on the DDS mip count.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(mips.size() > 0 ? (mips.size() - 1) : 0));

    const std::string label = "TextureDDS:" + path.filename().string();
    glutil::debug::labelGLobject(GL_TEXTURE, out.id, label);

    out.w = dds.width();
    out.h = dds.height();
    out.format = internalFmt;
    out.ok = true;
    return out;
}

} // namespace glutil