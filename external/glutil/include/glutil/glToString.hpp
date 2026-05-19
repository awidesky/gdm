#ifndef GLUTIL_GLTOSTRING_HPP
#define GLUTIL_GLTOSTRING_HPP

#include <glutil/gl.hpp>
#include <cstddef>

namespace glutil {

constexpr inline const char* glTypeToString(GLint type) {
    switch (type) {
        case GL_FLOAT: return "GL_FLOAT";
        case GL_UNSIGNED_BYTE: return "GL_UNSIGNED_BYTE";
        case GL_UNSIGNED_SHORT: return "GL_UNSIGNED_SHORT";
        case GL_UNSIGNED_INT: return "GL_UNSIGNED_INT";
        case GL_INT: return "GL_INT";
        case GL_SHORT: return "GL_SHORT";
        case GL_BYTE: return "GL_BYTE";
        default: return "UNKNOWN";
    }
}

constexpr inline std::size_t glTypeSize(GLenum type) {
    switch (type) {
        case GL_FLOAT: return sizeof(GLfloat);
        case GL_DOUBLE: return sizeof(GLdouble);
        case GL_INT: return sizeof(GLint);
        case GL_UNSIGNED_INT: return sizeof(GLuint);
        case GL_SHORT: return sizeof(GLshort);
        case GL_UNSIGNED_SHORT: return sizeof(GLushort);
        case GL_BYTE: return sizeof(GLbyte);
        case GL_UNSIGNED_BYTE: return sizeof(GLubyte);
        default: return sizeof(unsigned char);
    }
}

constexpr inline const char* glTextureFormatToString(GLint fmt) {
    switch (fmt) {
        case GL_RGB8: return "GL_RGB8";
        case GL_RGBA8: return "GL_RGBA8";
        case GL_R8: return "GL_R8";
        case GL_RG8: return "GL_RG8";
        case GL_RGB16F: return "GL_RGB16F";
        case GL_RGBA16F: return "GL_RGBA16F";
        case GL_RGB32F: return "GL_RGB32F";
        case GL_RGBA32F: return "GL_RGBA32F";

        case GL_DEPTH_COMPONENT: return "GL_DEPTH_COMPONENT";
        case GL_DEPTH_COMPONENT16: return "GL_DEPTH_COMPONENT16";
        case GL_DEPTH_COMPONENT24: return "GL_DEPTH_COMPONENT24";
        case GL_DEPTH_COMPONENT32F: return "GL_DEPTH_COMPONENT32F";
        case GL_DEPTH_STENCIL: return "GL_DEPTH_STENCIL";

        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT: return "GL_COMPRESSED_RGB_S3TC_DXT1";
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT: return "GL_COMPRESSED_RGBA_S3TC_DXT1";
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT: return "GL_COMPRESSED_RGBA_S3TC_DXT3";
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT: return "GL_COMPRESSED_RGBA_S3TC_DXT5";

        case GL_LINEAR: return "GL_LINEAR";
        case GL_NEAREST: return "GL_NEAREST";
        case GL_LINEAR_MIPMAP_LINEAR: return "GL_LINEAR_MIPMAP_LINEAR";
        case GL_LINEAR_MIPMAP_NEAREST: return "GL_LINEAR_MIPMAP_NEAREST";
        case GL_NEAREST_MIPMAP_LINEAR: return "GL_NEAREST_MIPMAP_LINEAR";
        case GL_NEAREST_MIPMAP_NEAREST: return "GL_NEAREST_MIPMAP_NEAREST";
        case GL_REPEAT: return "GL_REPEAT";
        case GL_CLAMP_TO_EDGE: return "GL_CLAMP_TO_EDGE";
        case GL_MIRRORED_REPEAT: return "GL_MIRRORED_REPEAT";

        case GL_LEQUAL: return "GL_LEQUAL";
        case GL_GEQUAL: return "GL_GEQUAL";
        case GL_LESS: return "GL_LESS";
        case GL_GREATER: return "GL_GREATER";
        case GL_EQUAL: return "GL_EQUAL";
        case GL_NOTEQUAL: return "GL_NOTEQUAL";
        case GL_ALWAYS: return "GL_ALWAYS";
        case GL_NEVER: return "GL_NEVER";

        case GL_ZERO: return "GL_ZERO";
        case GL_ONE: return "GL_ONE";
        case GL_SRC_ALPHA: return "GL_SRC_ALPHA";
        case GL_ONE_MINUS_SRC_ALPHA: return "GL_ONE_MINUS_SRC_ALPHA";
        case GL_DST_ALPHA: return "GL_DST_ALPHA";
        case GL_ONE_MINUS_DST_ALPHA: return "GL_ONE_MINUS_DST_ALPHA";
        case GL_SRC_COLOR: return "GL_SRC_COLOR";
        case GL_ONE_MINUS_SRC_COLOR: return "GL_ONE_MINUS_SRC_COLOR";
        case GL_DST_COLOR: return "GL_DST_COLOR";
        case GL_ONE_MINUS_DST_COLOR: return "GL_ONE_MINUS_DST_COLOR";
        case GL_BACK: return "GL_BACK";
        case GL_FRONT: return "GL_FRONT";
        case GL_FRONT_AND_BACK: return "GL_FRONT_AND_BACK";


        default: return "UNKNOWN";
    }
}

constexpr inline const char* glErrorToString(GLenum err) {
    switch (err) {
        case GL_NO_ERROR: return "GL_NO_ERROR";
        case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
        case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
        case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
        case GL_FRAMEBUFFER_COMPLETE: return "GL_FRAMEBUFFER_COMPLETE";
        case GL_FRAMEBUFFER_UNDEFINED: return "GL_FRAMEBUFFER_UNDEFINED";
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: return "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: return "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
        case GL_FRAMEBUFFER_UNSUPPORTED: return "GL_FRAMEBUFFER_UNSUPPORTED";
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: return "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
        default: return "UNKNOWN_ERROR";
    }
}

constexpr inline const char* usageToString(GLint usage) {
    switch (usage) {
        case GL_STATIC_DRAW: return "GL_STATIC_DRAW";
        case GL_DYNAMIC_DRAW: return "GL_DYNAMIC_DRAW";
        case GL_STREAM_DRAW: return "GL_STREAM_DRAW";
        case GL_STATIC_READ: return "GL_STATIC_READ";
        case GL_DYNAMIC_READ: return "GL_DYNAMIC_READ";
        case GL_STREAM_READ: return "GL_STREAM_READ";
        case GL_STATIC_COPY: return "GL_STATIC_COPY";
        case GL_DYNAMIC_COPY: return "GL_DYNAMIC_COPY";
        case GL_STREAM_COPY: return "GL_STREAM_COPY";
        default: return "UNKNOWN";
    }
}

constexpr inline const char* glBufferTypeToString(GLenum target) {
    switch (target) {
        case GL_ARRAY_BUFFER: return "GL_ARRAY_BUFFER";
        case GL_ATOMIC_COUNTER_BUFFER: return "GL_ATOMIC_COUNTER_BUFFER";
        case GL_COPY_READ_BUFFER: return "GL_COPY_READ_BUFFER";
        case GL_COPY_WRITE_BUFFER: return "GL_COPY_WRITE_BUFFER";
        case GL_DISPATCH_INDIRECT_BUFFER: return "GL_DISPATCH_INDIRECT_BUFFER";
        case GL_DRAW_INDIRECT_BUFFER: return "GL_DRAW_INDIRECT_BUFFER";
        case GL_ELEMENT_ARRAY_BUFFER: return "GL_ELEMENT_ARRAY_BUFFER";
        case GL_PIXEL_PACK_BUFFER: return "GL_PIXEL_PACK_BUFFER";
        case GL_PIXEL_UNPACK_BUFFER: return "GL_PIXEL_UNPACK_BUFFER";
        case GL_QUERY_BUFFER: return "GL_QUERY_BUFFER";
        case GL_SHADER_STORAGE_BUFFER: return "GL_SHADER_STORAGE_BUFFER";
        case GL_TEXTURE_BUFFER: return "GL_TEXTURE_BUFFER";
        case GL_TRANSFORM_FEEDBACK_BUFFER: return "GL_TRANSFORM_FEEDBACK_BUFFER";
        case GL_UNIFORM_BUFFER: return "GL_UNIFORM_BUFFER";
        default: return "UNKNOWN_BUFFER";
    }
}

} // namespace glutil

#endif // GLUTIL_GLTOSTRING_HPP