#ifndef GLUTIL_GLTOSTRING_HPP
#define GLUTIL_GLTOSTRING_HPP

#include <glutil/gl.hpp>
#include <cstddef>

namespace glutil {

constexpr inline const char* glTypeToString(GLint type) {
    switch (type) {
        case GL_FLOAT: return "GL_FLOAT";
        case GL_DOUBLE: return "GL_DOUBLE";
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

constexpr inline const char* glTextureInternalFormatToString(GLenum fmt) {
    switch (fmt) {
        // base formats
        case GL_DEPTH_COMPONENT: return "GL_DEPTH_COMPONENT";
        case GL_DEPTH_STENCIL: return "GL_DEPTH_STENCIL";
        case GL_RED: return "GL_RED";
        case GL_RG: return "GL_RG";
        case GL_RGB: return "GL_RGB";
        case GL_RGBA: return "GL_RGBA";

        // R
        case GL_R8: return "GL_R8";
        case GL_R8_SNORM: return "GL_R8_SNORM";
        case GL_R16: return "GL_R16";
        case GL_R16_SNORM: return "GL_R16_SNORM";
        case GL_R16F: return "GL_R16F";
        case GL_R32F: return "GL_R32F";
        case GL_R8I: return "GL_R8I";
        case GL_R8UI: return "GL_R8UI";
        case GL_R16I: return "GL_R16I";
        case GL_R16UI: return "GL_R16UI";
        case GL_R32I: return "GL_R32I";
        case GL_R32UI: return "GL_R32UI";

        // RG
        case GL_RG8: return "GL_RG8";
        case GL_RG8_SNORM: return "GL_RG8_SNORM";
        case GL_RG16: return "GL_RG16";
        case GL_RG16_SNORM: return "GL_RG16_SNORM";
        case GL_RG16F: return "GL_RG16F";
        case GL_RG32F: return "GL_RG32F";
        case GL_RG8I: return "GL_RG8I";
        case GL_RG8UI: return "GL_RG8UI";
        case GL_RG16I: return "GL_RG16I";
        case GL_RG16UI: return "GL_RG16UI";
        case GL_RG32I: return "GL_RG32I";
        case GL_RG32UI: return "GL_RG32UI";

        // RGB
        case GL_RGB8: return "GL_RGB8";
        case GL_RGB8_SNORM: return "GL_RGB8_SNORM";
        case GL_RGB10: return "GL_RGB10";
        case GL_RGB12: return "GL_RGB12";
        case GL_RGB16: return "GL_RGB16";
        case GL_RGB16_SNORM: return "GL_RGB16_SNORM";
        case GL_RGB16F: return "GL_RGB16F";
        case GL_RGB32F: return "GL_RGB32F";
        case GL_R11F_G11F_B10F: return "GL_R11F_G11F_B10F";
        case GL_RGB9_E5: return "GL_RGB9_E5";
        case GL_SRGB8: return "GL_SRGB8";
        case GL_RGB8I: return "GL_RGB8I";
        case GL_RGB8UI: return "GL_RGB8UI";
        case GL_RGB16I: return "GL_RGB16I";
        case GL_RGB16UI: return "GL_RGB16UI";
        case GL_RGB32I: return "GL_RGB32I";
        case GL_RGB32UI: return "GL_RGB32UI";

        // RGBA
        case GL_RGBA2: return "GL_RGBA2";
        case GL_RGBA4: return "GL_RGBA4";
        case GL_RGB5_A1: return "GL_RGB5_A1";
        case GL_RGBA8: return "GL_RGBA8";
        case GL_RGBA8_SNORM: return "GL_RGBA8_SNORM";
        case GL_RGB10_A2: return "GL_RGB10_A2";
        case GL_RGB10_A2UI: return "GL_RGB10_A2UI";
        case GL_RGBA12: return "GL_RGBA12";
        case GL_RGBA16: return "GL_RGBA16";
        case GL_RGBA16F: return "GL_RGBA16F";
        case GL_RGBA32F: return "GL_RGBA32F";
        case GL_RGBA8I: return "GL_RGBA8I";
        case GL_RGBA8UI: return "GL_RGBA8UI";
        case GL_RGBA16I: return "GL_RGBA16I";
        case GL_RGBA16UI: return "GL_RGBA16UI";
        case GL_RGBA32I: return "GL_RGBA32I";
        case GL_RGBA32UI: return "GL_RGBA32UI";
        case GL_SRGB8_ALPHA8: return "GL_SRGB8_ALPHA8";

        // depth
        case GL_DEPTH_COMPONENT16: return "GL_DEPTH_COMPONENT16";
        case GL_DEPTH_COMPONENT24: return "GL_DEPTH_COMPONENT24";
        case GL_DEPTH_COMPONENT32: return "GL_DEPTH_COMPONENT32";
        case GL_DEPTH_COMPONENT32F: return "GL_DEPTH_COMPONENT32F";
        case GL_DEPTH24_STENCIL8: return "GL_DEPTH24_STENCIL8";
        case GL_DEPTH32F_STENCIL8: return "GL_DEPTH32F_STENCIL8";

        // compressed S3TC (EXT)
#ifdef GL_COMPRESSED_RGB_S3TC_DXT1_EXT
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT: return "GL_COMPRESSED_RGB_S3TC_DXT1";
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT: return "GL_COMPRESSED_RGBA_S3TC_DXT1";
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT: return "GL_COMPRESSED_RGBA_S3TC_DXT3";
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT: return "GL_COMPRESSED_RGBA_S3TC_DXT5";
#endif
        // RGTC
#if defined(GL_ARB_texture_compression_rgtc) || defined(GL_VERSION_3_0)
        case GL_COMPRESSED_RED_RGTC1: return "GL_COMPRESSED_RED_RGTC1";
        case GL_COMPRESSED_SIGNED_RED_RGTC1: return "GL_COMPRESSED_SIGNED_RED_RGTC1";
        case GL_COMPRESSED_RG_RGTC2: return "GL_COMPRESSED_RG_RGTC2";
        case GL_COMPRESSED_SIGNED_RG_RGTC2: return "GL_COMPRESSED_SIGNED_RG_RGTC2";
#endif
        // BPTC (OpenGL 4.2+, optional in 3.3 via extension)
#if defined(GL_ARB_texture_compression_bptc) || defined(GL_VERSION_4_2)
        case GL_COMPRESSED_RGBA_BPTC_UNORM: return "GL_COMPRESSED_RGBA_BPTC_UNORM";
        case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM: return "GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM";
        case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT: return "GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT";
        case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT: return "GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT";
#endif
        // generic compressed
        case GL_COMPRESSED_RED: return "GL_COMPRESSED_RED";
        case GL_COMPRESSED_RG: return "GL_COMPRESSED_RG";
        case GL_COMPRESSED_RGB: return "GL_COMPRESSED_RGB";
        case GL_COMPRESSED_RGBA: return "GL_COMPRESSED_RGBA";
        case GL_COMPRESSED_SRGB: return "GL_COMPRESSED_SRGB";
        case GL_COMPRESSED_SRGB_ALPHA: return "GL_COMPRESSED_SRGB_ALPHA";

        default: return "UNKNOWN_INTERNAL_FORMAT";
    }
}

constexpr inline const char* glTextureFilterToString(GLenum v) {
    switch (v) {
        case GL_NEAREST: return "GL_NEAREST";
        case GL_LINEAR: return "GL_LINEAR";

        case GL_NEAREST_MIPMAP_NEAREST: return "GL_NEAREST_MIPMAP_NEAREST";
        case GL_LINEAR_MIPMAP_NEAREST: return "GL_LINEAR_MIPMAP_NEAREST";
        case GL_NEAREST_MIPMAP_LINEAR: return "GL_NEAREST_MIPMAP_LINEAR";
        case GL_LINEAR_MIPMAP_LINEAR: return "GL_LINEAR_MIPMAP_LINEAR";

        default: return "UNKNOWN";
    }
}

constexpr inline const char* glTextureWrapModeToString(GLenum v) {
    switch (v) {
        case GL_REPEAT: return "GL_REPEAT";
        case GL_MIRRORED_REPEAT: return "GL_MIRRORED_REPEAT";
        case GL_CLAMP_TO_EDGE: return "GL_CLAMP_TO_EDGE";
        case GL_CLAMP_TO_BORDER: return "GL_CLAMP_TO_BORDER";

#ifdef GL_MIRROR_CLAMP_TO_EDGE
        case GL_MIRROR_CLAMP_TO_EDGE: return "GL_MIRROR_CLAMP_TO_EDGE";
#endif
#ifdef GL_MIRROR_CLAMP_TO_EDGE_EXT
        case GL_MIRROR_CLAMP_TO_EDGE_EXT: return "GL_MIRROR_CLAMP_TO_EDGE_EXT";
#endif

        default: return "UNKNOWN";
    }
}

constexpr inline const char* glCompareFuncToString(GLenum v) {
    switch (v) {
        case GL_NEVER: return "GL_NEVER";
        case GL_LESS: return "GL_LESS";
        case GL_EQUAL: return "GL_EQUAL";
        case GL_LEQUAL: return "GL_LEQUAL";
        case GL_GREATER: return "GL_GREATER";
        case GL_NOTEQUAL: return "GL_NOTEQUAL";
        case GL_GEQUAL: return "GL_GEQUAL";
        case GL_ALWAYS: return "GL_ALWAYS";

        default: return "UNKNOWN";
    }
}

constexpr inline const char* glBlendFactorToString(GLenum v) {
    switch (v) {
        case GL_ZERO: return "GL_ZERO";
        case GL_ONE: return "GL_ONE";

        case GL_SRC_COLOR: return "GL_SRC_COLOR";
        case GL_ONE_MINUS_SRC_COLOR: return "GL_ONE_MINUS_SRC_COLOR";
        case GL_DST_COLOR: return "GL_DST_COLOR";
        case GL_ONE_MINUS_DST_COLOR: return "GL_ONE_MINUS_DST_COLOR";

        case GL_SRC_ALPHA: return "GL_SRC_ALPHA";
        case GL_ONE_MINUS_SRC_ALPHA: return "GL_ONE_MINUS_SRC_ALPHA";
        case GL_DST_ALPHA: return "GL_DST_ALPHA";
        case GL_ONE_MINUS_DST_ALPHA: return "GL_ONE_MINUS_DST_ALPHA";

        case GL_CONSTANT_COLOR: return "GL_CONSTANT_COLOR";
        case GL_ONE_MINUS_CONSTANT_COLOR: return "GL_ONE_MINUS_CONSTANT_COLOR";
        case GL_CONSTANT_ALPHA: return "GL_CONSTANT_ALPHA";
        case GL_ONE_MINUS_CONSTANT_ALPHA: return "GL_ONE_MINUS_CONSTANT_ALPHA";

        case GL_SRC_ALPHA_SATURATE: return "GL_SRC_ALPHA_SATURATE";

#ifdef GL_SRC1_COLOR
        case GL_SRC1_COLOR: return "GL_SRC1_COLOR";
        case GL_ONE_MINUS_SRC1_COLOR: return "GL_ONE_MINUS_SRC1_COLOR";
        case GL_SRC1_ALPHA: return "GL_SRC1_ALPHA";
        case GL_ONE_MINUS_SRC1_ALPHA: return "GL_ONE_MINUS_SRC1_ALPHA";
#endif

        default: return "UNKNOWN";
    }
}

constexpr inline const char* glCullFaceModeToString(GLenum v) {
    switch (v) {
        case GL_FRONT: return "GL_FRONT";
        case GL_BACK: return "GL_BACK";
        case GL_FRONT_AND_BACK: return "GL_FRONT_AND_BACK";

        default: return "UNKNOWN";
    }
}

constexpr inline const char* glTextureTargetToShortString(GLenum target) {
    switch (target){
        case GL_TEXTURE_1D: return "Tex1D";
        case GL_TEXTURE_2D: return "Tex2D";
        case GL_TEXTURE_3D: return "Tex3D";
        case GL_TEXTURE_CUBE_MAP: return "TexCubemap";
#ifdef GL_TEXTURE_1D_ARRAY
        case GL_TEXTURE_1D_ARRAY: return "Tex1DArr";
        case GL_TEXTURE_2D_ARRAY: return "Tex2DArr";
        case GL_TEXTURE_RECTANGLE: return "TexRect";
#endif
#ifdef GL_TEXTURE_CUBE_MAP_ARRAY
        case GL_TEXTURE_CUBE_MAP_ARRAY: return "TexCubeArr";
        case GL_TEXTURE_2D_MULTISAMPLE: return "Tex2DMS";
        case GL_TEXTURE_2D_MULTISAMPLE_ARRAY: return "Tex2DMSArr";
        case GL_TEXTURE_BUFFER: return "TexBuffer";
#endif
        default: return "UNKNOWN";
    }
}

constexpr inline const char* glTextureTargetToString(GLenum target) {
    switch (target){
        case GL_TEXTURE_1D: return "GL_TEXTURE_1D";
        case GL_TEXTURE_2D: return "GL_TEXTURE_2D";
        case GL_TEXTURE_3D: return "GL_TEXTURE_3D";
        case GL_TEXTURE_CUBE_MAP: return "GL_TEXTURE_CUBE_MAP";
#ifdef GL_TEXTURE_1D_ARRAY
        case GL_TEXTURE_1D_ARRAY: return "GL_TEXTURE_1D_ARRAY";
        case GL_TEXTURE_2D_ARRAY: return "GL_TEXTURE_2D_ARRAY";
        case GL_TEXTURE_RECTANGLE: return "GL_TEXTURE_RECTANGLE";
#endif
#ifdef GL_TEXTURE_CUBE_MAP_ARRAY
        case GL_TEXTURE_CUBE_MAP_ARRAY: return "GL_TEXTURE_CUBE_MAP_ARRAY";
        case GL_TEXTURE_2D_MULTISAMPLE: return "GL_TEXTURE_2D_MULTISAMPLE";
        case GL_TEXTURE_2D_MULTISAMPLE_ARRAY: return "GL_TEXTURE_2D_MULTISAMPLE_ARRAY";
        case GL_TEXTURE_BUFFER: return "GL_TEXTURE_BUFFER";
#endif
        default: return "UNKNOWN";
    }
}

constexpr inline const char* glShaderTypeToShortString(GLenum type) {
    switch (type) {
    case GL_VERTEX_SHADER: return "vs";
    case GL_FRAGMENT_SHADER: return "fs";
#ifdef GL_GEOMETRY_SHADER
    case GL_GEOMETRY_SHADER: return "gs";
#endif
#ifdef GL_TESS_CONTROL_SHADER
    case GL_TESS_CONTROL_SHADER: return "tcs";
#endif
#ifdef GL_TESS_EVALUATION_SHADER
    case GL_TESS_EVALUATION_SHADER: return "tes";
#endif
#ifdef GL_COMPUTE_SHADER
    case GL_COMPUTE_SHADER: return "cs";
#endif
    default: return "UNKNOWN_SHADER_TYPE";
    }
}
constexpr inline const char* glShaderTypeToString(GLenum type) {
    switch (type) {
    case GL_VERTEX_SHADER: return "GL_VERTEX_SHADER";
    case GL_FRAGMENT_SHADER: return "GL_FRAGMENT_SHADER";
#ifdef GL_GEOMETRY_SHADER
    case GL_GEOMETRY_SHADER: return "GL_GEOMETRY_SHADER";
#endif
#ifdef GL_TESS_CONTROL_SHADER
    case GL_TESS_CONTROL_SHADER: return "GL_TESS_CONTROL_SHADER";
#endif
#ifdef GL_TESS_EVALUATION_SHADER
    case GL_TESS_EVALUATION_SHADER: return "GL_TESS_EVALUATION_SHADER";
#endif
#ifdef GL_COMPUTE_SHADER
    case GL_COMPUTE_SHADER: return "GL_COMPUTE_SHADER";
#endif
    default: return "UNKNOWN_SHADER_TYPE";
    }
}

constexpr inline const char* glErrorToString(GLenum err) {
    switch (err) {
        case GL_NO_ERROR: return "GL_NO_ERROR";
        case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
        case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
        case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
        case GL_STACK_OVERFLOW: return "GL_STACK_OVERFLOW";
        case GL_STACK_UNDERFLOW: return "GL_STACK_UNDERFLOW";
        default: return "UNKNOWN_ERROR";
    }
}

constexpr inline const char* glFramebufferStatusToString(GLenum status) {
    switch (status) {
        case GL_FRAMEBUFFER_COMPLETE: return "GL_FRAMEBUFFER_COMPLETE";
#ifdef GL_FRAMEBUFFER_UNDEFINED
        case GL_FRAMEBUFFER_UNDEFINED: return "GL_FRAMEBUFFER_UNDEFINED";
#endif
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: return "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: return "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
        case GL_FRAMEBUFFER_UNSUPPORTED: return "GL_FRAMEBUFFER_UNSUPPORTED";
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: return "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
#ifdef GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS
        case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS: return "GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS";
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_FORMATS
        case GL_FRAMEBUFFER_INCOMPLETE_FORMATS: return "GL_FRAMEBUFFER_INCOMPLETE_FORMATS";
#endif
        default: return "UNKNOWN_FRAMEBUFFER_STATUS";
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

inline const char* glLabelObjectTypeToString(GLenum type) {
    switch (type) {
        case GL_BUFFER: return "GL_BUFFER";
        case GL_SHADER: return "GL_SHADER";
        case GL_PROGRAM: return "GL_PROGRAM";
        case GL_VERTEX_ARRAY: return "GL_VERTEX_ARRAY";
        case GL_QUERY: return "GL_QUERY";
        case GL_TEXTURE: return "GL_TEXTURE";
        case GL_RENDERBUFFER: return "GL_RENDERBUFFER";
        case GL_FRAMEBUFFER: return "GL_FRAMEBUFFER";
        case GL_SAMPLER: return "GL_SAMPLER";
#ifdef GL_PROGRAM_PIPELINE
        case GL_PROGRAM_PIPELINE: return "GL_PROGRAM_PIPELINE";
#endif
#ifdef GL_TRANSFORM_FEEDBACK
        case GL_TRANSFORM_FEEDBACK: return "GL_TRANSFORM_FEEDBACK";
#endif

        default: return "(UNKNOWN_OBJECT_TYPE)";
    }
}

constexpr inline const char* glBufferTypeToString(GLenum target) {
    switch (target) {
        case GL_ARRAY_BUFFER: return "GL_ARRAY_BUFFER";
        case GL_ATOMIC_COUNTER_BUFFER: return "GL_ATOMIC_COUNTER_BUFFER";
        case GL_COPY_READ_BUFFER: return "GL_COPY_READ_BUFFER";
        case GL_COPY_WRITE_BUFFER: return "GL_COPY_WRITE_BUFFER";
#ifdef GL_DISPATCH_INDIRECT_BUFFER
        case GL_DISPATCH_INDIRECT_BUFFER: return "GL_DISPATCH_INDIRECT_BUFFER";
        case GL_QUERY_BUFFER: return "GL_QUERY_BUFFER";
        case GL_SHADER_STORAGE_BUFFER: return "GL_SHADER_STORAGE_BUFFER";
#endif
        case GL_DRAW_INDIRECT_BUFFER: return "GL_DRAW_INDIRECT_BUFFER";
        case GL_ELEMENT_ARRAY_BUFFER: return "GL_ELEMENT_ARRAY_BUFFER";
        case GL_PIXEL_PACK_BUFFER: return "GL_PIXEL_PACK_BUFFER";
        case GL_PIXEL_UNPACK_BUFFER: return "GL_PIXEL_UNPACK_BUFFER";
        case GL_TEXTURE_BUFFER: return "GL_TEXTURE_BUFFER";
        case GL_TRANSFORM_FEEDBACK_BUFFER: return "GL_TRANSFORM_FEEDBACK_BUFFER";
        case GL_UNIFORM_BUFFER: return "GL_UNIFORM_BUFFER";
        default: return "UNKNOWN_BUFFER";
    }
}

constexpr inline const char* glBufferTypeToShortString(GLenum target) {
    switch (target) {
        case GL_ARRAY_BUFFER: return "VBO";
        case GL_ATOMIC_COUNTER_BUFFER: return "ACB";
        case GL_COPY_READ_BUFFER: return "CRB";
        case GL_COPY_WRITE_BUFFER: return "CWB";
#ifdef GL_DISPATCH_INDIRECT_BUFFER
        case GL_DISPATCH_INDIRECT_BUFFER: return "DIB";
        case GL_QUERY_BUFFER: return "QB";
        case GL_SHADER_STORAGE_BUFFER: return "SSBO";
#endif
        case GL_DRAW_INDIRECT_BUFFER: return "DrIB";
        case GL_ELEMENT_ARRAY_BUFFER: return "EBO";
        case GL_PIXEL_PACK_BUFFER: return "PPB";
        case GL_PIXEL_UNPACK_BUFFER: return "PUB";
        case GL_TEXTURE_BUFFER: return "TBO";
        case GL_TRANSFORM_FEEDBACK_BUFFER: return "TFB";
        case GL_UNIFORM_BUFFER: return "UBO";
        default: return "UNKNOWN";
    }
}
} // namespace glutil

#endif // GLUTIL_GLTOSTRING_HPP