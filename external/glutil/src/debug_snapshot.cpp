#include <glutil/gl.hpp>
#include <glutil/glToString.hpp>

#include <glutil/debug_snapshot.hpp>
#include <glutil/glutil.hpp>

#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#define LOG LOG_ERROR()

namespace glutil::debug {

static void printSeparator(std::ostream& out, const char* title) {
    out << "\n============================== " << title << " ==============================\n\n";
}

static void printSubSeparator(std::ostream& out, const std::string& title) {
    out << "\n  ---------- " << title << " ----------\n";
}

struct GLStateGuard {
    GLint vao, arrayBuffer, activeTexture, renderbuffer;

    GLStateGuard() {
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &arrayBuffer);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
        glGetIntegerv(GL_RENDERBUFFER_BINDING, &renderbuffer);
    }
    ~GLStateGuard() {
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, arrayBuffer);
        glActiveTexture(activeTexture);
        glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
    }
};

snapshot::snapshot(bool printAll)
    : m_shaderStatus(printAll), m_shaderUniform(printAll), m_textureInfo(printAll), m_textureIncludeSampler(printAll),
      m_bufferVAOInfo(printAll), m_bufferIncludeUnbound(false), m_bufferIncludeDisabled(false), m_allVBOInfo(printAll),
      m_bufferIncludeData(false), m_rendererState(printAll), m_framebufferInfo(printAll), m_boundInfo(printAll),
      m_Once(true), m_flag(false) {}

snapshot& snapshot::shaderStatus(bool v) {
    m_shaderStatus = v;
    return *this;
}
snapshot& snapshot::shaderUniform(bool v) {
    m_shaderUniform = v;
    return *this;
}

snapshot& snapshot::textureInfo(bool v, bool includeSampler) {
    m_textureInfo = v;
    m_textureIncludeSampler = includeSampler;
    return *this;
}

snapshot& snapshot::bufferVAOInfo(bool v, bool includeData, bool includeDisabled, bool includeUnbound) {
    m_bufferVAOInfo = v;
    m_bufferIncludeUnbound = includeUnbound;
    m_bufferIncludeDisabled = includeDisabled;
    m_bufferIncludeData = includeData;
    return *this;
}

snapshot& snapshot::allVBOInfo(bool v) {
    m_allVBOInfo = v;
    return *this;
}

snapshot& snapshot::rendererState(bool v) {
    m_rendererState = v;
    return *this;
}
snapshot& snapshot::framebufferInfo(bool v) {
    m_framebufferInfo = v;
    return *this;
}
snapshot& snapshot::boundInfo(bool v) {
    m_boundInfo = v;
    return *this;
}

snapshot& snapshot::printPerCall(bool v) {
    m_Once = !v;
    return *this;
}

void snapshot::captureFramebuffer(std::ostream& out) const {
    printSeparator(out, "Framebuffer");

    const GLenum fbStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    out << "  Status     : " << glutil::glErrorToString(fbStatus) << " (0x" << std::hex << fbStatus << std::dec
        << ")\n";

    GLint fbBinding = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbBinding);
    out << "  Bound FBO  : " << fbBinding << "\n";

    if (fbBinding != 0) {
        GLint objType = 0;
        glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                              GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &objType);

        if (objType == GL_TEXTURE) {
            GLint texId = 0, mipLevel = 0, internalFmt = 0;
            glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                                  GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &texId);
            glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                                  GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL, &mipLevel);
            glGetTexLevelParameteriv(GL_TEXTURE_2D, mipLevel, GL_TEXTURE_INTERNAL_FORMAT, &internalFmt);
            out << "  Color Att0 : Texture ID=" << texId << "  mip=" << mipLevel
                << "  Format=" << glTextureFormatToString(internalFmt) << "\n";

        } else if (objType == GL_RENDERBUFFER) {
            GLint rbId = 0;
            glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                                  GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &rbId);

            GLint samples = 0, internalFmt = 0, w = 0, h = 0;
            glBindRenderbuffer(GL_RENDERBUFFER, rbId);
            glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_SAMPLES, &samples);
            glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_INTERNAL_FORMAT, &internalFmt);
            glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &w);
            glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &h);

            out << "  Color Att0 : Renderbuffer ID=" << rbId << "  " << w << "x" << h << "  Format=0x" << std::hex
                << internalFmt << std::dec << "  Samples=" << samples << (samples > 0 ? " (MSAA)" : "") << "\n";
        }
    }
}

void snapshot::captureShaderStatus(std::ostream& out) const {
    printSeparator(out, "Shader Status");

    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    if (program == 0) {
        out << "  No shader program bound\n";
        return;
    }

    GLint linkStatus = 0, infoLogLen = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLen);

    out << "  Program ID  : " << program << "\n";
    out << "  Link Status : " << (linkStatus == GL_TRUE ? "OK" : "FAIL") << "\n";

    if (infoLogLen > 0) {
        std::string log(infoLogLen, '\0');
        glGetProgramInfoLog(program, infoLogLen, nullptr, log.data());
        out << "  InfoLog     :\n" << log << "\n";
    }
}

void snapshot::captureShaderUniforms(std::ostream& out) const {
    printSeparator(out, "Shader Uniforms");

    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    if (program == 0) {
        out << "  No shader program bound\n";
        return;
    }

    GLint count = 0, maxLen = 0;
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &count);
    glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLen);

    out << "  Program ID : " << program << "  Active uniforms : " << count << "\n\n";

    std::vector<char> name(maxLen);

    int maxNameWidth = 0;
    for (GLint i = 0; i < count; i++) {
        GLint sz = 0;
        GLenum tp = 0;
        glGetActiveUniform(program, i, maxLen, nullptr, &sz, &tp, name.data());
        maxNameWidth = std::max(maxNameWidth, (int)std::string(name.data()).size());
    }
    int maxTypeWidth = 20;
    int valueIndent = 2 + maxNameWidth + 2 + maxTypeWidth + 6;

    for (GLint i = 0; i < count; i++) {
        GLint size = 0;
        GLenum type = 0;
        glGetActiveUniform(program, i, maxLen, nullptr, &size, &type, name.data());

        GLint loc = glGetUniformLocation(program, name.data());
        if (loc == -1)
            continue;

        out << std::fixed << std::setprecision(4);
        out << "  " << std::left << std::setw(maxNameWidth) << name.data() << "  " << std::setw(maxTypeWidth);

        switch (type) {
            // ── float  ──
            case GL_FLOAT: {
                GLfloat v;
                glGetUniformfv(program, loc, &v);
                out << "float" << " =    " << std::setw(10) << v << "\n";
                break;
            }
            case GL_FLOAT_VEC2: {
                GLfloat v[2];
                glGetUniformfv(program, loc, v);
                out << "vec2" << " =    (" << std::setw(10) << v[0] << ", " << std::setw(10) << v[1] << ")\n";
                break;
            }
            case GL_FLOAT_VEC3: {
                GLfloat v[3];
                glGetUniformfv(program, loc, v);
                out << "vec3" << " =    (" << std::setw(10) << v[0] << ", " << std::setw(10) << v[1] << ", "
                    << std::setw(10) << v[2] << ")\n";
                break;
            }
            case GL_FLOAT_VEC4: {
                GLfloat v[4];
                glGetUniformfv(program, loc, v);
                out << "vec4" << " =    (" << std::setw(10) << v[0] << ", " << std::setw(10) << v[1] << ", "
                    << std::setw(10) << v[2] << ", " << std::setw(10) << v[3] << ")\n";
                break;
            }
            // ── square mat  ──
            case GL_FLOAT_MAT2: {
                GLfloat v[4];
                glGetUniformfv(program, loc, v);
                std::string indent(valueIndent, ' ');
                out << "mat2" << " =    ";
                for (int row = 0; row < 2; row++) {
                    if (row != 0)
                        out << indent;
                    out << "[ ";
                    for (int col = 0; col < 2; col++)
                        out << std::setw(10) << v[col * 2 + row] << " ";
                    out << "]\n";
                }
                break;
            }
            case GL_FLOAT_MAT3: {
                GLfloat v[9];
                glGetUniformfv(program, loc, v);
                std::string indent(valueIndent, ' ');
                out << "mat3" << " =    ";
                for (int row = 0; row < 3; row++) {
                    if (row != 0)
                        out << indent;
                    out << "[ ";
                    for (int col = 0; col < 3; col++)
                        out << std::setw(10) << v[col * 3 + row] << " ";
                    out << "]\n";
                }
                break;
            }
            case GL_FLOAT_MAT4: {
                GLfloat v[16];
                glGetUniformfv(program, loc, v);
                std::string indent(valueIndent, ' ');
                out << "mat4" << " =    ";
                for (int row = 0; row < 4; row++) {
                    if (row != 0)
                        out << indent;
                    out << "[ ";
                    for (int col = 0; col < 4; col++)
                        out << std::setw(10) << v[col * 4 + row] << " ";
                    out << "]\n";
                }
                break;
            }
            // ── non-square mat  ──
            case GL_FLOAT_MAT2x3: {
                GLfloat v[6];
                glGetUniformfv(program, loc, v);
                std::string indent(valueIndent, ' ');
                out << "mat2x3" << " =    ";
                for (int row = 0; row < 3; row++) {
                    if (row != 0)
                        out << indent;
                    out << "[ ";
                    for (int col = 0; col < 2; col++)
                        out << std::setw(10) << v[col * 3 + row] << " ";
                    out << "]\n";
                }
                break;
            }
            case GL_FLOAT_MAT2x4: {
                GLfloat v[8];
                glGetUniformfv(program, loc, v);
                std::string indent(valueIndent, ' ');
                out << "mat2x4" << " =    ";
                for (int row = 0; row < 4; row++) {
                    if (row != 0)
                        out << indent;
                    out << "[ ";
                    for (int col = 0; col < 2; col++)
                        out << std::setw(10) << v[col * 4 + row] << " ";
                    out << "]\n";
                }
                break;
            }
            case GL_FLOAT_MAT3x2: {
                GLfloat v[6];
                glGetUniformfv(program, loc, v);
                std::string indent(valueIndent, ' ');
                out << "mat3x2" << " =    ";
                for (int row = 0; row < 2; row++) {
                    if (row != 0)
                        out << indent;
                    out << "[ ";
                    for (int col = 0; col < 3; col++)
                        out << std::setw(10) << v[col * 2 + row] << " ";
                    out << "]\n";
                }
                break;
            }
            case GL_FLOAT_MAT3x4: {
                GLfloat v[12];
                glGetUniformfv(program, loc, v);
                std::string indent(valueIndent, ' ');
                out << "mat3x4" << " =    ";
                for (int row = 0; row < 4; row++) {
                    if (row != 0)
                        out << indent;
                    out << "[ ";
                    for (int col = 0; col < 3; col++)
                        out << std::setw(10) << v[col * 4 + row] << " ";
                    out << "]\n";
                }
                break;
            }
            case GL_FLOAT_MAT4x2: {
                GLfloat v[8];
                glGetUniformfv(program, loc, v);
                std::string indent(valueIndent, ' ');
                out << "mat4x2" << " =    ";
                for (int row = 0; row < 2; row++) {
                    if (row != 0)
                        out << indent;
                    out << "[ ";
                    for (int col = 0; col < 4; col++)
                        out << std::setw(10) << v[col * 2 + row] << " ";
                    out << "]\n";
                }
                break;
            }
            case GL_FLOAT_MAT4x3: {
                GLfloat v[12];
                glGetUniformfv(program, loc, v);
                std::string indent(valueIndent, ' ');
                out << "mat4x3" << " =    ";
                for (int row = 0; row < 3; row++) {
                    if (row != 0)
                        out << indent;
                    out << "[ ";
                    for (int col = 0; col < 4; col++)
                        out << std::setw(10) << v[col * 3 + row] << " ";
                    out << "]\n";
                }
                break;
            }
            // ── int  ──
            case GL_INT: {
                GLint v;
                glGetUniformiv(program, loc, &v);
                out << "int" << " =    " << v << "\n";
                break;
            }
            case GL_INT_VEC2: {
                GLint v[2];
                glGetUniformiv(program, loc, v);
                out << "ivec2" << " =    (" << std::setw(8) << v[0] << ", " << std::setw(8) << v[1] << ")\n";
                break;
            }
            case GL_INT_VEC3: {
                GLint v[3];
                glGetUniformiv(program, loc, v);
                out << "ivec3" << " =    (" << std::setw(8) << v[0] << ", " << std::setw(8) << v[1] << ", "
                    << std::setw(8) << v[2] << ")\n";
                break;
            }
            case GL_INT_VEC4: {
                GLint v[4];
                glGetUniformiv(program, loc, v);
                out << "ivec4" << " =    (" << std::setw(8) << v[0] << ", " << std::setw(8) << v[1] << ", "
                    << std::setw(8) << v[2] << ", " << std::setw(8) << v[3] << ")\n";
                break;
            }
            // ── uint  ──
            case GL_UNSIGNED_INT: {
                GLuint v;
                glGetUniformuiv(program, loc, &v);
                out << "uint" << " =    " << v << "\n";
                break;
            }
            case GL_UNSIGNED_INT_VEC2: {
                GLuint v[2];
                glGetUniformuiv(program, loc, v);
                out << "uvec2" << " =    (" << std::setw(8) << v[0] << ", " << std::setw(8) << v[1] << ")\n";
                break;
            }
            case GL_UNSIGNED_INT_VEC3: {
                GLuint v[3];
                glGetUniformuiv(program, loc, v);
                out << "uvec3" << " =    (" << std::setw(8) << v[0] << ", " << std::setw(8) << v[1] << ", "
                    << std::setw(8) << v[2] << ")\n";
                break;
            }
            case GL_UNSIGNED_INT_VEC4: {
                GLuint v[4];
                glGetUniformuiv(program, loc, v);
                out << "uvec4" << " =    (" << std::setw(8) << v[0] << ", " << std::setw(8) << v[1] << ", "
                    << std::setw(8) << v[2] << ", " << std::setw(8) << v[3] << ")\n";
                break;
            }
            // ── bool  ──
            case GL_BOOL: {
                GLint v;
                glGetUniformiv(program, loc, &v);
                out << "bool" << " =    " << (v ? "true" : "false") << "\n";
                break;
            }
            case GL_BOOL_VEC2: {
                GLint v[2];
                glGetUniformiv(program, loc, v);
                out << "bvec2" << " =    (" << (v[0] ? "true" : "false") << ", " << (v[1] ? "true" : "false") << ")\n";
                break;
            }
            case GL_BOOL_VEC3: {
                GLint v[3];
                glGetUniformiv(program, loc, v);
                out << "bvec3" << " =    (" << (v[0] ? "true" : "false") << ", " << (v[1] ? "true" : "false") << ", "
                    << (v[2] ? "true" : "false") << ")\n";
                break;
            }
            case GL_BOOL_VEC4: {
                GLint v[4];
                glGetUniformiv(program, loc, v);
                out << "bvec4" << " =    (" << (v[0] ? "true" : "false") << ", " << (v[1] ? "true" : "false") << ", "
                    << (v[2] ? "true" : "false") << ", " << (v[3] ? "true" : "false") << ")\n";
                break;
            }
            // ── sampler  ──
            case GL_SAMPLER_2D: {
                GLint v;
                glGetUniformiv(program, loc, &v);
                out << "sampler2D" << " =    unit " << v << "\n";
                break;
            }
            case GL_SAMPLER_3D: {
                GLint v;
                glGetUniformiv(program, loc, &v);
                out << "sampler3D" << " =    unit " << v << "\n";
                break;
            }
            case GL_SAMPLER_CUBE: {
                GLint v;
                glGetUniformiv(program, loc, &v);
                out << "samplerCube" << " =    unit " << v << "\n";
                break;
            }
            case GL_SAMPLER_2D_SHADOW: {
                GLint v;
                glGetUniformiv(program, loc, &v);
                out << "sampler2DShadow" << " =    unit " << v << "\n";
                break;
            }
            case GL_SAMPLER_2D_ARRAY: {
                GLint v;
                glGetUniformiv(program, loc, &v);
                out << "sampler2DArray" << " =    unit " << v << "\n";
                break;
            }
            case GL_SAMPLER_2D_ARRAY_SHADOW: {
                GLint v;
                glGetUniformiv(program, loc, &v);
                out << "sampler2DArrayShadow" << " =    unit " << v << "\n";
                break;
            }
            case GL_SAMPLER_CUBE_SHADOW: {
                GLint v;
                glGetUniformiv(program, loc, &v);
                out << "samplerCubeShadow" << " =    unit " << v << "\n";
                break;
            }
            case GL_INT_SAMPLER_2D: {
                GLint v;
                glGetUniformiv(program, loc, &v);
                out << "isampler2D" << " =    unit " << v << "\n";
                break;
            }
            case GL_INT_SAMPLER_3D: {
                GLint v;
                glGetUniformiv(program, loc, &v);
                out << "isampler3D" << " =    unit " << v << "\n";
                break;
            }
            case GL_INT_SAMPLER_CUBE: {
                GLint v;
                glGetUniformiv(program, loc, &v);
                out << "isamplerCube" << " =    unit " << v << "\n";
                break;
            }
            case GL_UNSIGNED_INT_SAMPLER_2D: {
                GLint v;
                glGetUniformiv(program, loc, &v);
                out << "usampler2D" << " =    unit " << v << "\n";
                break;
            }
            case GL_UNSIGNED_INT_SAMPLER_3D: {
                GLint v;
                glGetUniformiv(program, loc, &v);
                out << "usampler3D" << " =    unit " << v << "\n";
                break;
            }
            case GL_UNSIGNED_INT_SAMPLER_CUBE: {
                GLint v;
                glGetUniformiv(program, loc, &v);
                out << "usamplerCube" << " =    unit " << v << "\n";
                break;
            }

            // TODO : image  (GL_IMAGE_2D ) - NEED ??
            // TODO : GL_UNSIGNED_INT_ATOMIC_COUNTER - NEED ??
            default: out << "unknown" << " =    0x" << std::hex << type << std::dec << "\n"; break;
        }
    }
}

void snapshot::captureTextureInfo(std::ostream& out) const {
    printSeparator(out, "Texture Info");

    GLint savedActiveTexture = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &savedActiveTexture);

    GLint maxUnits = 0;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxUnits);

    struct TexType {
        GLenum binding;
        GLenum target;
        const char* name;
        bool hasSampler;
        bool hasSize;
    };

    TexType types[] = {
      {GL_TEXTURE_BINDING_2D, GL_TEXTURE_2D, "2D", true, true},
      {GL_TEXTURE_BINDING_CUBE_MAP, GL_TEXTURE_CUBE_MAP, "CubeMap", true, true},
      {GL_TEXTURE_BINDING_3D, GL_TEXTURE_3D, "3D", true, true},
      {GL_TEXTURE_BINDING_2D_ARRAY, GL_TEXTURE_2D_ARRAY, "2D_Array", true, true},
      {GL_TEXTURE_BINDING_2D_MULTISAMPLE, GL_TEXTURE_2D_MULTISAMPLE, "2D_Multisample", false, true},
      {GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_2D_MULTISAMPLE_ARRAY, "2D_MS_Array", false, true},
      {GL_TEXTURE_BINDING_CUBE_MAP_ARRAY, GL_TEXTURE_CUBE_MAP_ARRAY, "CubeMap_Array", true, true},
      {GL_TEXTURE_BINDING_RECTANGLE, GL_TEXTURE_RECTANGLE, "Rectangle", false, true},
      {GL_TEXTURE_BINDING_1D, GL_TEXTURE_1D, "1D", true, true},
      {GL_TEXTURE_BINDING_1D_ARRAY, GL_TEXTURE_1D_ARRAY, "1D_Array", true, true},
    };

    const int samplerIndent = 29;
    const std::string indent(samplerIndent, ' ');

    auto centerName = [](const char* name, int width) -> std::string {
        std::string s(name);
        if ((int)s.size() >= width)
            return s;
        int total = width - (int)s.size();
        int left = total / 2;
        int right = total - left;
        return std::string(left, ' ') + s + std::string(right, ' ');
    };

    bool anyBound = false;
    for (int i = 0; i < maxUnits; i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        for (auto& t : types) {
            GLint texId = 0;
            glGetIntegerv(t.binding, &texId);
            if (texId == 0)
                continue;

            anyBound = true;

            GLint w = 0, h = 0, fmt = 0;
            if (t.hasSize) {
                glGetTexLevelParameteriv(t.target, 0, GL_TEXTURE_WIDTH, &w);
                glGetTexLevelParameteriv(t.target, 0, GL_TEXTURE_HEIGHT, &h);
                glGetTexLevelParameteriv(t.target, 0, GL_TEXTURE_INTERNAL_FORMAT, &fmt);
            }

            out << "  Unit " << std::setw(2) << i << "  [" << centerName(t.name, 14) << "]"
                << "  ID=" << std::setw(4) << texId;

            if (t.hasSize)
                out << "  Size=" << w << "x" << h << "  Format=" << glTextureFormatToString(fmt);

            out << "\n";

            if (m_textureIncludeSampler && t.hasSampler) {
                GLint minFilter = 0, magFilter = 0, wrapS = 0, wrapT = 0, compareFunc = 0;
                glGetTexParameteriv(t.target, GL_TEXTURE_MIN_FILTER, &minFilter);
                glGetTexParameteriv(t.target, GL_TEXTURE_MAG_FILTER, &magFilter);
                glGetTexParameteriv(t.target, GL_TEXTURE_WRAP_S, &wrapS);
                glGetTexParameteriv(t.target, GL_TEXTURE_WRAP_T, &wrapT);
                glGetTexParameteriv(t.target, GL_TEXTURE_COMPARE_FUNC, &compareFunc);

                out << indent << "MIN_FILTER=" << glTextureFormatToString(minFilter)
                    << "  MAG_FILTER=" << glTextureFormatToString(magFilter) << "\n"
                    << indent << "WRAP_S=" << glTextureFormatToString(wrapS)
                    << "  WRAP_T=" << glTextureFormatToString(wrapT)
                    << "  COMPARE_FUNC=" << glTextureFormatToString(compareFunc) << "\n";
            }
        }
    }
    if (!anyBound)
        out << "  (no textures bound)\n";

    glActiveTexture(savedActiveTexture);
}

void snapshot::captureBufferVAOInfo(std::ostream& out) const {
    auto formatVertex = [](const unsigned char* ptr, GLenum type, int components) -> std::string {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(4) << std::left << "(";
        for (int c = 0; c < components; c++) {
            switch (type) {
                case GL_FLOAT: oss << std::setw(8) << reinterpret_cast<const GLfloat*>(ptr)[c]; break;
                case GL_DOUBLE: oss << std::setw(10) << reinterpret_cast<const GLdouble*>(ptr)[c]; break;
                case GL_INT: oss << std::setw(8) << reinterpret_cast<const GLint*>(ptr)[c]; break;
                case GL_UNSIGNED_INT: oss << std::setw(8) << reinterpret_cast<const GLuint*>(ptr)[c]; break;
                case GL_SHORT: oss << std::setw(6) << reinterpret_cast<const GLshort*>(ptr)[c]; break;
                case GL_UNSIGNED_SHORT: oss << std::setw(6) << reinterpret_cast<const GLushort*>(ptr)[c]; break;
                case GL_BYTE: oss << std::setw(4) << static_cast<int>(reinterpret_cast<const GLbyte*>(ptr)[c]); break;
                case GL_UNSIGNED_BYTE:
                    oss << std::setw(4) << static_cast<int>(reinterpret_cast<const GLubyte*>(ptr)[c]);
                    break;
                default: oss << "?"; break;
            }
            if (c < components - 1)
                oss << ", ";
        }
        oss << ")";
        return oss.str();
    };

    printSeparator(out, "Buffer VAO Info");

    GLint savedArrayBuffer = 0, savedVAO = 0;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &savedArrayBuffer);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &savedVAO);

    // Get All VAO
    auto& tracker = GLStateTracker::instance();
    const auto& allObjects = tracker.objects.getAll();
    auto vaoIt = allObjects.find("VAO");

    if (vaoIt == allObjects.end() || vaoIt->second.empty()) {
        out << "  No VAOs tracked\n";
        glBindVertexArray(savedVAO);
        glBindBuffer(GL_ARRAY_BUFFER, savedArrayBuffer);
        return;
    }

    for (GLuint vaoId : vaoIt->second) {

        bool isBound = (static_cast<GLuint>(savedVAO) == vaoId);

        if (!isBound && !m_bufferIncludeUnbound)
            continue;

        glBindVertexArray(vaoId);

        out << "\n  VAO ID : " << vaoId << (isBound ? "  [BOUND]" : "  [UNBOUND]") << "\n";

        GLint maxAttribs = 0;
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttribs);

        GLint prevVboId = -1;
        GLint curBufSize = 0;

        for (int i = 0; i < maxAttribs; i++) {
            GLint enabled = 0;
            glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
            if (!enabled && !m_bufferIncludeDisabled)
                continue;

            GLint vboId = 0, size = 0, stride = 0;
            GLenum type = 0;
            void* offset = nullptr;
            glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &vboId);
            glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_SIZE, &size);
            glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_TYPE, (GLint*)&type);
            glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &stride);
            glGetVertexAttribPointerv(i, GL_VERTEX_ATTRIB_ARRAY_POINTER, &offset);
            uintptr_t off = reinterpret_cast<uintptr_t>(offset);

            if (vboId != prevVboId) {
                prevVboId = vboId;
                if (vboId == 0) {
                    continue;
                }
                glBindBuffer(GL_ARRAY_BUFFER, vboId);

                GLint newUsage = 0;
                glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &curBufSize);
                glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_USAGE, &newUsage);

                printSubSeparator(out, "VBO ID=" + std::to_string(vboId));
                out << "    Size  : " << curBufSize << " bytes\n";
                out << "    Usage : " << usageToString(newUsage) << "\n";

                prevVboId = vboId;
            }
            if (vboId == 0)
                continue;
            out << "\n    attrib[" << i << "]" << "     size=" << size << "  type=" << glTypeToString(type)
                << "  stride=" << std::setw(4) << stride << "  offset=" << off << "\n";

            if (m_bufferIncludeData) {
                GLint mapped = 0;
                glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_MAPPED, &mapped);
                if (!mapped) {
                    std::vector<unsigned char> data(curBufSize);
                    glGetBufferSubData(GL_ARRAY_BUFFER, 0, curBufSize, data.data());

                    int typeSize = glTypeSize(type);
                    int actualStride = stride == 0 ? size * typeSize : stride;
                    int numVerts = actualStride > 0 ? curBufSize / actualStride : 0;
                    int printNum = std::min(numVerts, 10);

                    for (int v = 0; v < printNum; v++) {
                        const unsigned char* ptr = data.data() + v * actualStride + off;
                        out << "      vertex[" << v << "] : " << formatVertex(ptr, type, size) << "\n";
                    }
                    if (numVerts > 10)
                        out << "      ... (" << numVerts - 10 << " more)\n";
                } else {
                    out << "      (buffer is mapped, skipping data read)\n";
                }
            }
        }

        // EBO
        GLint ebo = 0;
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &ebo);
        if (ebo != 0) {
            printSubSeparator(out, "EBO ID=" + std::to_string(ebo));

            GLint eboSize = 0, eboUsage = 0;
            glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &eboSize);
            glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_USAGE, &eboUsage);

            out << "    Size  : " << eboSize << " bytes\n";
            out << "    Usage : " << usageToString(eboUsage) << "\n";

            if (m_bufferIncludeData) {
                GLint mapped = 0;
                glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_MAPPED, &mapped);
                if (!mapped) {
                    // tracker에서 dataType 읽기
                    GLenum indexType = GL_UNSIGNED_INT; // fallback
                    auto& tracker = GLStateTracker::instance();
                    if (auto* info = tracker.buffers.get(static_cast<GLuint>(ebo))) {
                        if (info->dataType != 0)
                            indexType = info->dataType;
                    }

                    // indexType에 따라 올바른 크기로 읽기
                    // 가능한 것은 GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT, or GL_UNSIGNED_INT.
                    GLsizei elemSize = (indexType == GL_UNSIGNED_INT)     ? sizeof(GLuint)
                                       : (indexType == GL_UNSIGNED_SHORT) ? sizeof(GLushort)
                                                                          : sizeof(GLubyte);

                    int count = eboSize / elemSize;
                    int printNum = std::min(count, 30);

                    out << "    Type    : " << glTypeToString(indexType) << "\n";
                    out << "    indices : ";

                    std::vector<unsigned char> raw(eboSize);
                    glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, eboSize, raw.data());

                    for (int i = 0; i < printNum; i++) {
                        if (i > 0)
                            out << ", ";
                        if (indexType == GL_UNSIGNED_INT)
                            out << reinterpret_cast<GLuint*>(raw.data())[i];
                        else if (indexType == GL_UNSIGNED_SHORT)
                            out << reinterpret_cast<GLushort*>(raw.data())[i];
                        else
                            out << static_cast<int>(raw.data()[i]);
                    }
                    if (count > 30)
                        out << " ... (" << count - 30 << " more)";
                    out << "\n";
                } else {
                    out << "    (buffer is mapped, skipping data read)\n";
                }
            }
        } else {
            out << "\n  EBO : (none)\n";
        }
        // 원래 바인딩 상태로 복구
        glBindVertexArray(savedVAO);
        glBindBuffer(GL_ARRAY_BUFFER, savedArrayBuffer);
    }
}
void snapshot::captureAllVBOInfo(std::ostream& out) const {

    printSeparator(out, "All VBO Info");
    auto& tracker = GLStateTracker::instance();

    GLint savedArrayBuffer = 0;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &savedArrayBuffer);
    bool bHasVBO = false;

    const std::unordered_map<GLuint, BufferInfo> buffers = tracker.buffers.getAll();
    for (const auto& buffer : buffers) {
        if (buffer.second.role != BufferRole::VBO)
            continue;

        bHasVBO = true;
        GLuint id = buffer.first;
        glBindBuffer(GL_ARRAY_BUFFER, id);

        GLint size = 0, usage = 0;
        glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
        glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_USAGE, &usage);

        out << "  VBO ID=" << id << "  Size=" << size << " bytes"
            << "  Usage=" << usageToString(usage) << "  ";

        GLint mapped = 0;
        glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_MAPPED, &mapped);
        if (!mapped) {
            std::vector<unsigned char> data(size);
            glGetBufferSubData(GL_ARRAY_BUFFER, 0, size, data.data());

            if (buffer.second.associatedVaos.empty()) {
                out << "Not Bound to VAO";
            } else {
                out << "Bound VAO ID : ";
                for (auto& vao : buffer.second.associatedVaos) {
                    out << vao << ' ';
                }
            }
            out << '\n';
        }
    }
    if (!bHasVBO)
        out << "  (no VBOs tracked)\n";
    glBindBuffer(GL_ARRAY_BUFFER, savedArrayBuffer);
}
void snapshot::captureRendererState(std::ostream& out) const {
    printSeparator(out, "Renderer State");

    // Viewport
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    out << "  Viewport     : x=" << vp[0] << " y=" << vp[1] << " w=" << vp[2] << " h=" << vp[3] << "\n";

    // enable/disable 상태
    out << "  Depth Test   : " << (glIsEnabled(GL_DEPTH_TEST) ? "ON" : "OFF") << "\n";
    out << "  Blend        : " << (glIsEnabled(GL_BLEND) ? "ON" : "OFF") << "\n";
    out << "  Cull Face    : " << (glIsEnabled(GL_CULL_FACE) ? "ON" : "OFF");

    if (glIsEnabled(GL_CULL_FACE)) {
        GLint cullMode = 0;
        glGetIntegerv(GL_CULL_FACE_MODE, &cullMode);
        out << "   " << glTextureFormatToString(cullMode);
    }
    out << "\n";

    out << "  Scissor Test : " << (glIsEnabled(GL_SCISSOR_TEST) ? "ON" : "OFF") << "\n";
    out << "  Stencil Test : " << (glIsEnabled(GL_STENCIL_TEST) ? "ON" : "OFF") << "\n";

    // 세부 설정
    GLint depthFunc = 0;
    glGetIntegerv(GL_DEPTH_FUNC, &depthFunc);
    out << "  Depth Func   : " << glTextureFormatToString(depthFunc) << "\n";

    GLint blendSrc = 0, blendDst = 0;
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSrc);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDst);
    out << "  Blend Src    : " << glTextureFormatToString(blendSrc) << "\n";
    out << "  Blend Dst    : " << glTextureFormatToString(blendDst) << "\n";
}
void snapshot::captureBoundInfo(std::ostream& out) const {
    printSeparator(out, "Bound Info");

    auto printBinding = [&](const char* name, GLenum pname) {
        GLint v = 0;
        glGetIntegerv(pname, &v);
        out << "  " << std::left << std::setw(40) << name << " : " << v << "\n";
    };

    printBinding("GL_VERTEX_ARRAY_BINDING", GL_VERTEX_ARRAY_BINDING);
    printBinding("GL_ARRAY_BUFFER_BINDING", GL_ARRAY_BUFFER_BINDING);
    printBinding("GL_ELEMENT_ARRAY_BUFFER_BINDING", GL_ELEMENT_ARRAY_BUFFER_BINDING);
    printBinding("GL_UNIFORM_BUFFER_BINDING", GL_UNIFORM_BUFFER_BINDING);
    printBinding("GL_SHADER_STORAGE_BUFFER_BINDING", GL_SHADER_STORAGE_BUFFER_BINDING);
    printBinding("GL_PIXEL_PACK_BUFFER_BINDING", GL_PIXEL_PACK_BUFFER_BINDING);
    printBinding("GL_PIXEL_UNPACK_BUFFER_BINDING", GL_PIXEL_UNPACK_BUFFER_BINDING);
    printBinding("GL_TEXTURE_BINDING_2D", GL_TEXTURE_BINDING_2D);
    printBinding("GL_SAMPLER_BINDING", GL_SAMPLER_BINDING);
    printBinding("GL_CURRENT_PROGRAM", GL_CURRENT_PROGRAM);
    printBinding("GL_RENDERBUFFER_BINDING", GL_RENDERBUFFER_BINDING);
    printBinding("GL_FRAMEBUFFER_BINDING", GL_FRAMEBUFFER_BINDING);
}



void snapshot::capture(std::ostream& out) const {

    if (m_flag && m_Once)
        return;

    m_flag = true;

    static thread_local bool insideSnapshot = false;
    if (insideSnapshot)
        return;
    insideSnapshot = true;

    GLStateGuard guard;

    out << "\n========================================================\n";
    out << "               glutil::debug::snapshot                 \n";
    out << "========================================================\n";

    if (m_framebufferInfo)
        captureFramebuffer(out);
    if (m_shaderStatus)
        captureShaderStatus(out);
    if (m_shaderUniform)
        captureShaderUniforms(out);
    if (m_textureInfo)
        captureTextureInfo(out);
    if (m_bufferVAOInfo)
        captureBufferVAOInfo(out);
    if (m_allVBOInfo)
        captureAllVBOInfo(out);
    if (m_rendererState)
        captureRendererState(out);
    if (m_boundInfo)
        captureBoundInfo(out);

    out << "\n========================================================\n";
    out << "                     snapshot end                      \n";
    out << "========================================================\n\n";

    insideSnapshot = false;
}

void snapshot::capture(const std::filesystem::path& dir, bool dumpVertexData) const 
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::ostringstream filename;
    filename << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S") << ".txt";

    std::ofstream f(dir / filename.str());
    this->capture(f);

    if (dumpVertexData)
    {
        static thread_local bool insideSnapshot = false;
        if (insideSnapshot)
            return;
        insideSnapshot = true;

        GLStateGuard guard;
        saveBufferInfoToFile(dir);
    }
}

void snapshot::saveBufferInfoToFile(const std::filesystem::path& dir) const {
    std::filesystem::create_directories(dir);
    auto& tracker = GLStateTracker::instance();

    GLint savedVAO = 0, savedArrayBuffer = 0;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &savedVAO);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &savedArrayBuffer);

    char label[1024];
    GLsizei labelLen = 0;

    // ── VBO 파일 저장 ──
    for (auto& [id, info] : tracker.buffers.getAll()) {
        if (info.role != BufferRole::VBO)
            continue;

        glBindBuffer(GL_ARRAY_BUFFER, id);

        GLint size = 0, mapped = 0;
        glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
        glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_MAPPED, &mapped);
        if (mapped)
            continue;

        glGetObjectLabel(GL_BUFFER, id, sizeof(label), &labelLen, label);

        std::ofstream f(dir / (std::to_string(id) + ".vbo"));
        f << "VBO_ID=" << id << "\n";
        f << "LABEL=" << (labelLen > 0 ? label : "") << "\n";

        std::vector<unsigned char> data(size);
        glGetBufferSubData(GL_ARRAY_BUFFER, 0, size, data.data());

        for (int i = 0; i < size; i++) {
            f << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
            if (i < size - 1)
                f << " ";
        }
        f << std::dec << "\n";
    }

    // ── VAO 파일 저장 ──
    const auto& allObjects = tracker.objects.getAll();
    auto vaoIt = allObjects.find("VAO");
    if (vaoIt != allObjects.end()) {
        for (GLuint vaoId : vaoIt->second) {
            glBindVertexArray(vaoId);

           bool savedUnbound = m_bufferIncludeUnbound;
           this->m_bufferIncludeUnbound = false;

            std::ofstream f(dir / (std::to_string(vaoId) + ".vao"));
            captureBufferVAOInfo(f);

            this->m_bufferIncludeUnbound = savedUnbound;
        }
    }

    glBindVertexArray(savedVAO);
    glBindBuffer(GL_ARRAY_BUFFER, savedArrayBuffer);
}

} // namespace glutil::debug

//    // TODO : 아래에 대한 TODO를 수행해야 함. 한번에 모든 걸 하려고 하지 말고, 하나하나 건당 정확하게 해결 후 각각을
//    확인받은 후에 커밋할 것.
//    //
//    //
//    // 1. static thread_local bool insideSnapshot = false;등 이용해서 recursion 방지
//    // 2. stacktrace처럼 builder함수를 이용한 객체로 다시 구조화. 필요한 정보만 골라서 출력할 수 있도록
//    //   예시:
//    // snapshot{true} //snapshot 타입 객체 생성, 생성자는 bool printAll = true, 모든 스냅샷 항목을 defualt로 true로
//    놓는가? 라는 의미
//    //          .shaderStatus(false) // 셰이더 관련 내용 출력할까?
//    //          .textureInfo(false) // 텍스쳐 관련 내용 출력할까?
//    //          .currntBufferOnly(true) // 현재 바인드된 VAO, VBO, EBO 관련 내용 출력할까? false면 현존하는 모든 VAO
//    출력(debug.hpp에 모든 VAO나 VBO를 관리하는 레지스트리가 있다고 가정)
//    //          .bufferContent(false) // VBO attribute 및 내부 데이터 출력할까? (glGetBufferSubData호출하므로 시간
//    오래 걸릴 수 있음)
//    //          .disabledAttribVBO(false) //VERTEX_ATTRIB_ARRAY_ENABLED가 아닌 것, 즉 glDisableVertexAttribArray(0);된
//    VBO도 출력
//    //          .shaderUniform(false) // shader 유니폼 값 출력할까?
//    //          .rendererState(false) // 렌더러 상태(뷰포트, depth-test enbale등등 여부..) 출력할까?
//    //          .boundInfo(false) // 기타 바인딩 정보 출력(GL_ARRAY_BUFFER_BINDING, GL_ELEMENT_ARRAY_BUFFER_BINDING,
//    GL_UNIFORM_BUFFER_BINDING, GL_SHADER_STORAGE_BUFFER_BINDING, GL_PIXEL_PACK_BUFFER_BINDING,
//    GL_PIXEL_UNPACK_BUFFER_BINDING, GL_TEXTURE_BINDING_2D,GL_SAMPLER_BINDING)
//    //          .capture(std::cerr); // 주어진 스트림(콘솔, 파일...)으로 출력
//    // 위와 같은 코드는 예시이며, 추가하거나 삭제하는 것이 필요한 경우 알맞게 처리할 것.
//    //
//    // 필요한 경우 snapshot.hpp에는 일종의 프리셋처럼 설정된(오류가 났을 때 유용한 스냅샷, 셰이더 디버깅에 유용한
//    스냅샷, 버퍼 데이터 디버깅에 유용한 스냅샷 등등..) 다양한 객체를 리턴하는 헬퍼 함수가 존재한다
//    //
//    // 그냥 glutil::debug::snapshot{}.capture(...)하면 모든 내용 다 출력
//    // 각 함수는 bool값만 바꾸고, print시에 실제 스냅샷을 찍고, 현재 snapshot함수처럼 분기하여 알맞게 출력하도록
//    //
//    // 3. "=========== VBO Status ========="과 같이 구분선과 공백 출력하여, 잘 보이게 변경
//    //
//    // 4. 유니폼이나 버텍스 데이터 출력할 때, 알맞게 길이 포매팅하여 한눈에 잘 들어올 수 있도록 해야 한다.
//    //
//    // 5. 코드 내부에 있는 TODO도 수행해야 함!
//    //
//    // 6. 함수 실행 중에 gl state를 오염시키지 않도록 주의해야 함. 실행 이후로는 모든 게 원래 상태여야 함.(VAO
//    binding가 바뀌지 않도록)
//    // 7. glGetBufferSubData가 실패하지 않도록 GL_BUFFER_MAPPED확인
//    // 8. Texture Sampler State도 확인 (GL_TEXTURE_MIN_FILTER, GL_TEXTURE_MAG_FILTER, GL_TEXTURE_WRAP_S,
//    GL_TEXTURE_COMPARE_FUNC 등등)
//    //
//    //
