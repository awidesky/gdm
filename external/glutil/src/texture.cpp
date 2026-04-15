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

namespace glutil {

// ══════════════════════════════════════════════════════════
// ImageLoader::loadImage
// ══════════════════════════════════════════════════════════
TextureImage ImageLoader::loadImage(const char* path, bool flipV) {
    TextureImage result;

    // [PathResolve]
    PathResolveResult pr = pathResolve(path);
    if (!pr.success) {
        result.error = "path resolve failed: " + pr.message;
        LOG_WARNING() << "[TextureImage] " << result.error;
        return result;
    }

    // 천지뒤집기
    stbi_set_flip_vertically_on_load(flipV ? 1 : 0);

    int w = 0, h = 0, c = 0;
    unsigned char* raw = stbi_load(pr.resolvedPath.c_str(), &w, &h, &c, 0);
    if (!raw) {
        result.error = stbi_failure_reason() ? stbi_failure_reason() : "unknown stb_image error";
        LOG_WARNING() << "[TextureImage] load failed: " << pr.resolvedPath << " (" << result.error << ")";
        return result;
    }

    // [GLenum/GLint 변환을 파싱 시점에 수행하는 이유]
    // 사용자가 glTexImage2D에 바로 꽂을 수 있도록.
    // channels 숫자를 노출하지 않고 GL 타입으로 캡슐화.
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
            result.error = "unsupported channel count: " + std::to_string(c);
            LOG_WARNING() << "[TextureImage] " << result.error;
            stbi_image_free(raw);
            return result;
    }

    // [stbi → new[] 복사 이유]
    // stbi_load는 내부적으로 malloc 사용.
    // new[]로 복사 후 stbi_image_free로 즉시 원본 해제. 소멸자에서 delete[]로 해제.
    const size_t sz = static_cast<size_t>(w * h * c);
    result.pixels = new unsigned char[sz];
    std::memcpy(result.pixels, raw, sz);
    stbi_image_free(raw);

    result.w = static_cast<GLsizei>(w);
    result.h = static_cast<GLsizei>(h);
    result.fmt = fmt;
    result.internalFmt = internalFmt;
    result.ok = true;

    LOG_INFO() << "[TextureImage] load succeeded: " << pr.resolvedPath << " (" << w << "x" << h << ", flipV=" << flipV
               << ")";
    return result;
}

// ══════════════════════════════════════════════════════════
// ImageLoader::loadDDS
// ══════════════════════════════════════════════════════════

// [toGLFormat 설계 이유]
// ddsktx_format → GLenum 변환을 파싱 시점에 완료.
// BC1/BC2/BC3만 지원. 그 외는 0 반환 → LoadDDS에서 에러 처리.
// GL_COMPRESSED_RGBA_S3TC_DXT*_EXT는 GL_EXT_texture_compression_s3tc 확장.
// 현대 데스크톱 GPU는 거의 지원하나, 사용 전 확장 지원 여부 확인 권장 (사용자 책임).
static GLenum toGLFormat(ddsktx_format fmt, unsigned int flags) {
    switch (fmt) {
        // TODO : 이건 확장 표준이라 opengl 표준에서 지원하지 않음
        // glew의 경우 glewInit()하면 모든 확장을 가져오기 때문에 실행되는 것
        // glad 생성 시 GL_EXT_texture_compression_s3tc표준 추가해야 하고,
        // #ifdef GLAD_GL_EXT_texture_compression_s3tc 체크하고,
        //  if(!GLAD_GL_EXT_texture_compression_s3tc) { /* fallback */ } 추가해야 함
        // glew로 하는 경우
        // #ifdef GL_EXT_texture_compression_s3tc 체크하고
        //  if (!GLEW_EXT_texture_compression_s3tc) { /* fallback */ } 추가해야 함
        //
        // 사용자가 뭐를 선택했을지 알 수 없기 때문에, cmakelists.txt에서 add_compile_definition해서 GDM_USE_GLEW같은
        // 매크로 상수 만들어줘야함
        // case DDSKTX_FORMAT_BC1: return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        // case DDSKTX_FORMAT_BC2: return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        // case DDSKTX_FORMAT_BC3: return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;

        // BC1(DXT1)은 alpha 비트 사용 여부에 따라 RGB/RGBA가 갈림.
        // alpha 비트가 실제로 사용되지 않는 XRGB(alpha_x) 경우는 RGB로 올려야 함.
        case DDSKTX_FORMAT_BC1:
            return ((flags & DDSKTX_TEXTURE_FLAG_ALPHA) && !(flags & DDSKTX_TEXTURE_FLAG_ALPHA_X))
                     ? 0x83F1                  // GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
                     : 0x83F0;                 // GL_COMPRESSED_RGB_S3TC_DXT1_EXT
        case DDSKTX_FORMAT_BC2: return 0x83F2; // GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
        case DDSKTX_FORMAT_BC3: return 0x83F3; // GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
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
        result.error = "path resolve failed: " + pr.message;
        LOG_WARNING() << "[TextureDDS] " << result.error;
        return result;
    }

    // [파일 전체를 new[]로 읽는 이유]
    // dds-ktx는 no-allocation 라이브러리.
    // ddsktx_parse/ddsktx_get_sub 모두 외부 버퍼의 포인터를 그대로 사용.
    // fileData를 소멸자까지 유지해야 mip.offset 기반 접근이 유효함.
    // fopen/malloc 대신 ifstream + new[] 사용
    std::ifstream file(pr.resolvedPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        result.error = "failed to open file";
        LOG_WARNING() << "[TextureDDS] " << result.error << ": " << pr.resolvedPath;
        return result;
    }

    const auto pos = file.tellg();
    if (pos == std::streampos(-1)) {
        result.error = "failed to get file size";
        LOG_WARNING() << "[TextureDDS] " << result.error << ": " << pr.resolvedPath;
        return result;
    }

    const size_t fileSize = static_cast<size_t>(pos);
    file.seekg(0);
    result.fileData = new unsigned char[fileSize];

    if (!file.read(reinterpret_cast<char*>(result.fileData), fileSize)) {
        result.error = "failed to read file";
        LOG_WARNING() << "[TextureDDS] " << result.error << ": " << pr.resolvedPath;
        delete[] result.fileData;
        result.fileData = nullptr;
        return result;
    }

    ddsktx_texture_info tc = {0};
    ddsktx_error err = {};
    if (!ddsktx_parse(&tc, result.fileData, static_cast<int>(fileSize), &err)) {
        // [ddsktx_error.msg 근거]
        // dds-ktx.h: typedef struct ddsktx_error { char msg[256]; } ddsktx_error;
        result.error = std::string("ddsktx_parse failed: ") + err.msg;
        LOG_WARNING() << "[TextureDDS] " << result.error << ": " << pr.resolvedPath;
        delete[] result.fileData;
        result.fileData = nullptr;
        return result;
    }

    const GLenum glFmt = toGLFormat(tc.format, tc.flags);
    if (glFmt == 0) {
        result.error = "unsupported DDS format (only BC1/BC2/BC3 supported)";
        LOG_WARNING() << "[TextureDDS] " << result.error << ": " << pr.resolvedPath;
        delete[] result.fileData;
        result.fileData = nullptr;
        return result;
    }

    // [MipLevel 파싱 시점 계산 이유]
    // ShaderLoadResult의 lengthPtr()처럼 GL 함수에 바로 꽂히는 형태로 준비.
    // 사용자가 mip 크기/오프셋 계산 공식을 알 필요 없음.
    result.mipLevels.reserve(tc.num_mips);
    for (int i = 0; i < tc.num_mips; ++i) {
        ddsktx_sub_data sub;
        ddsktx_get_sub(&tc, &sub, result.fileData, static_cast<int>(fileSize), 0, 0, i);

        // sub.buff < fileData 이면 ptrdiff_t 음수 → size_t 캐스팅 시 UB.
        // 손상된 DDS 파일에서 발생 가능.
        const auto* ptr = reinterpret_cast<const unsigned char*>(sub.buff);
        if (ptr < result.fileData) {
            result.error = "DDS mip offset out of range: level=" + std::to_string(i);
            LOG_WARNING() << "[TextureDDS] " << result.error;
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

    LOG_INFO() << "[TextureDDS] load succeeded: " << pr.resolvedPath << " (" << tc.width << "x" << tc.height
               << ", mips=" << tc.num_mips << ")";
    return result;
}

} // namespace glutil