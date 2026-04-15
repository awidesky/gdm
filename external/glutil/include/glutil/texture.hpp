#ifndef GLUTIL_TEXTURE_HPP
#define GLUTIL_TEXTURE_HPP

// glutil/texture.hpp
//
// [설계 원칙]
// - ImageLoader만 TextureImage/TextureDDS를 생성 가능 (friend class ImageLoader)
// - 데이터 멤버 전부 private, 접근자만 공개
// - 소멸자에서 delete[], 복사 금지, 이동만 허용 (RAII)
//   malloc/free 대신 new[]/delete[] 규칙 준수
// - OpenGL 함수 호출 없음. GLenum/GLint/GLsizei 타입만 사용
//   파싱만 담당
// - 접근자 반환 타입을 GL 함수 파라미터 타입과 정확히 일치
//   사용자가 캐스팅 없이 GL 함수에 바로 꽂을 수 있도록 << 이거 캐스팅 안해줘도 되려나?

#include <glutil/gl.hpp>
#include <string>
#include <vector>
#include <cstddef>

namespace glutil {

// ── MipLevel ──────────────────────────────────────────────
// offset  : size_t  → fileData 포인터 연산용 (data() + offset), 플랫폼 포인터 크기
// size    : GLsizei → glCompressedTexImage2D(imageSize) 파라미터가 GLsizei
// width   : GLsizei → glCompressedTexImage2D(width)     파라미터가 GLsizei
// height  : GLsizei → glCompressedTexImage2D(height)    파라미터가 GLsizei
struct MipLevel {
    size_t  offset;
    GLsizei size;
    GLsizei width;
    GLsizei height;
};

// ── TextureImage ──────────────────────────────────────────
// stb_image로 파일 → 픽셀 데이터까지만 파싱. 
// format()         → GLenum : glTexImage2D(format)         파라미터가 GLenum
// internalFormat() → GLint  : glTexImage2D(internalFormat) 파라미터가 GLint
// width(), height()  → GLsizei: glTexImage2D(width/height)   파라미터가 GLsizei
//
// [channels() 미제공 이유]
// format()이 이미 채널 정보를 인코딩함
// GL_RED=1ch, GL_RGB=3ch, GL_RGBA=4ch → 중복 노출 불필요
struct TextureImage {
    bool        ok = false;
    std::string error;

    const void* data() const { return pixels; }
    GLsizei     width() const { return w; }
    GLsizei     height() const { return h; }
    GLenum      format() const { return fmt; }
    GLint       internalFormat() const { return internalFmt; }

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

    unsigned char* pixels      = nullptr; // new[], destroy()에서 delete[]
    GLsizei        w = 0, h    = 0;
    GLenum         fmt         = 0;
    GLint          internalFmt = 0;

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

// ── TextureDDS ────────────────────────────────────────────
// dds-ktx로 DDS 파일 → 압축 블록 데이터까지만 파싱. OpenGL 함수 없음.
//
// [fileData 보관 이유]
// dds-ktx는 no-allocation 라이브러리.
// ddsktx_get_sub()이 반환하는 sub_data.buff는 원본 버퍼(fileData) 안의 포인터.
// MipLevel.offset = sub_data.buff - fileData 로 계산해 저장.
// fileData를 소멸자까지 유지해야 dds.data() + mip.offset 이 유효함.
//
// [format() 반환 타입]
// GLenum : glCompressedTexImage2D(internalformat) 파라미터가 GLenum 
//
// [width/height 반환 타입]
// GLsizei : glCompressedTexImage2D width/height 파라미터 타입 
//
// [compressed() 미제공 이유]
// toGLFormat()이 BC1/BC2/BC3(모두 압축)만 허용하고 나머지는 에러 반환.
// LoadDDS() 성공 시 항상 압축 포맷이므로 분기용 플래그가 의미 없음.
// 비압축 DDS 지원 시 그때 추가.
struct TextureDDS {
    bool        ok = false;
    std::string error;

    const unsigned char*         data()     const { return fileData; }
    GLsizei                      width()    const { return w; }
    GLsizei                      height()   const { return h; }
    GLenum                       format() const { return fmt; }
    const std::vector<MipLevel>& mips()     const { return mipLevels; }

    ~TextureDDS() { destroy(); }
    TextureDDS() = default;
    TextureDDS(const TextureDDS&)            = delete;
    TextureDDS& operator=(const TextureDDS&) = delete;
    TextureDDS(TextureDDS&& o) noexcept            { moveFrom(std::move(o)); }
    TextureDDS& operator=(TextureDDS&& o) noexcept {
        if (this != &o) { destroy(); moveFrom(std::move(o)); }
        return *this;
    }

private:
    friend class ImageLoader;

    unsigned char*        fileData  = nullptr; // new[], destroy()에서 delete[]
    GLsizei               w = 0, h  = 0;
    GLenum                fmt       = 0;
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

// ── ImageLoader ───────────────────────────────────────────
// ShaderLoader처럼 로딩 책임을 별도 클래스로 분리.
// TextureImage/TextureDDS는 ImageLoader를 friend로 선언해
// 외부에서 직접 생성/수정 불가.
class ImageLoader {
public:
    static TextureImage LoadImage(const char* path, bool flipV = true);
    static TextureDDS   LoadDDS(const char* path);
};

} // namespace glutil

#endif // GLUTIL_TEXTURE_HPP
