#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glutil/glutil.hpp>
#include <iostream>
#include <glutil/debug_snapshot.hpp>
#include <map>
#include "config.hpp"

const char* glTypeToString(GLint type) {
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

void glutil::debug::snapshot() 
{
    // Check Frame Buffer
    const GLenum fbStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    LOG_ERROR() << "[OpenGL state dump]";
    LOG_ERROR() << "  Framebuffer status: " << glutil::debug::glErrorToString(fbStatus);

    // Check Shader progame Link
    GLint bound = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &bound);
    if (bound != 0) {
        GLint linkStatus = 0;
        glGetProgramiv(bound, GL_LINK_STATUS, &linkStatus);
        GLint infoLogLength = 0;
        glGetProgramiv(bound, GL_INFO_LOG_LENGTH, &infoLogLength);
        LOG_ERROR() << "  Current shader program ID: " << bound
                    << ", LinkStatus: " << (linkStatus == GL_TRUE ? "OK" : "FAIL");
        std::string infoLog(infoLogLength, '\0');
        if (infoLogLength > 0) {
            glGetProgramInfoLog(bound, infoLogLength, nullptr, infoLog.data());
            LOG_ERROR() << "  Program InfoLog: " << infoLog;
        }
    } else {
        LOG_ERROR() << "  No shader program bound";
    }

    // Check Texture
    GLint currentUnit = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &currentUnit);

    GLint maxUnits = 0;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxUnits);

    for (int i = 0; i < maxUnits; i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        GLint texId = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &texId);
        if (texId == 0)
        {
            continue;
        }
 
        GLint width = 0, height = 0, internalFormat = 0;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalFormat);
        LOG_ERROR() << "  Current 2D texture bound (ID: " << texId << ") Size: " << width << "x" << height
                    << ", Format: " << internalFormat;
    }
    glActiveTexture(currentUnit);


    //Check VAO/VBO limit 10
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &bound);
    if (bound != 0) {
        GLint vboBound = 0, bufSize = 0, usage = 0;
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &vboBound);
        glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufSize);
        glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_USAGE, &usage);

        const char* usageStr = "UNKNOWN";
        switch (usage) {
            case GL_STATIC_DRAW: usageStr = "GL_STATIC_DRAW"; break;
            case GL_DYNAMIC_DRAW: usageStr = "GL_DYNAMIC_DRAW"; break;
            case GL_STREAM_DRAW: usageStr = "GL_STREAM_DRAW"; break;
        }

        LOG_ERROR() << "  Current VAO bound: " << bound << "  VBO ID=" << vboBound << "  " << bufSize << " bytes"
                    << "  " << usageStr;

        std::vector<unsigned char> data(bufSize);
        glGetBufferSubData(GL_ARRAY_BUFFER, 0, bufSize, data.data());

        GLint maxAttribs = 0;
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttribs);

        for (int i = 0; i < maxAttribs; i++) {
            GLint enabled = 0;
            glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
            if (!enabled)
                continue;

            GLint vbo = 0, size = 0, type = 0, stride = 0;
            void* offset = nullptr;
            glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &vbo);
            glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_SIZE, &size);
            glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_TYPE, &type);
            glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &stride);
            glGetVertexAttribPointerv(i, GL_VERTEX_ATTRIB_ARRAY_POINTER, &offset);
            uintptr_t off = reinterpret_cast<uintptr_t>(offset);

            LOG_ERROR() << "    attrib[" << i << "]"
                        << " vbo=" << vbo << " size=" << size << " type=" << glTypeToString(type) << std::dec
                        << " stride=" << stride << " offset=" << off;

            int s = stride == 0 ? size * (int)sizeof(float) : stride;
            int numVerts = bufSize / s;
            int printNum = std::min(numVerts, 10);

            for (int v = 0; v < printNum; v++) {
                const float* ptr = reinterpret_cast<const float*>(data.data() + v * s + off);
                std::ostringstream oss;
                oss << "      vertex[" << v << "]: (";
                for (int c = 0; c < size; c++) {
                    oss << ptr[c];
                    if (c < size - 1)
                        oss << ", ";
                }
                oss << ")";
                LOG_ERROR() << oss.str();
            }
            if (numVerts > 10)
                LOG_ERROR() << "      ... (" << numVerts - 10 << " more)";
        }
    } else {
        LOG_ERROR() << "  No VAO bound: " << bound;
    }


    //Check EBO
    GLint eboBound = 0;
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &eboBound);
    if (eboBound != 0) {
        GLint eboSize = 0, eboUsage = 0;
        glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &eboSize);
        glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_USAGE, &eboUsage);

        const char* eboUsageStr = "UNKNOWN";
        switch (eboUsage) {
            case GL_STATIC_DRAW: eboUsageStr = "GL_STATIC_DRAW"; break;
            case GL_DYNAMIC_DRAW: eboUsageStr = "GL_DYNAMIC_DRAW"; break;
            case GL_STREAM_DRAW: eboUsageStr = "GL_STREAM_DRAW"; break;
        }

        LOG_ERROR() << "  [EBO dump] ID=" << eboBound << "  " << eboSize << " bytes"
                    << "  " << eboUsageStr;

        // VRAM → CPU 복사
        std::vector<unsigned int> indices(eboSize / sizeof(unsigned int));
        glGetBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, eboSize, indices.data());

        // 최대 30개 출력
        int printNum = std::min((int)indices.size(), 30);
        std::ostringstream oss;
        oss << "    indices: ";
        for (int i = 0; i < printNum; i++) {
            oss << indices[i];
            if (i < printNum - 1)
                oss << ", ";
        }
        if ((int)indices.size() > 30)
            oss << " ... (" << indices.size() - 30 << " more)";
        LOG_ERROR() << oss.str();
    } else {
        LOG_ERROR() << "  EBO: (none)";
    }

    //Check Shader Uniform Value
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    if (program != 0) {
        GLint count = 0;
        glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &count);

        GLint maxLen = 0;
        glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLen);

        LOG_ERROR() << "  [Uniforms] Shader ID=" << program << " count=" << count;

        std::vector<char> name(maxLen);

        for (GLint i = 0; i < count; i++) {
            GLint size = 0;
            GLenum type = 0;
            glGetActiveUniform(program, i, maxLen, nullptr, &size, &type, name.data());

            GLint loc = glGetUniformLocation(program, name.data());
            if (loc == -1)
                continue;

            std::ostringstream oss;
            oss << "    " << name.data() << " (";

            switch (type) {
                case GL_FLOAT: {
                    GLfloat v;
                    glGetUniformfv(program, loc, &v);
                    oss << "float) = " << v;
                    break;
                }
                case GL_FLOAT_VEC2: {
                    GLfloat v[2];
                    glGetUniformfv(program, loc, v);
                    oss << "vec2) = (" << v[0] << ", " << v[1] << ")";
                    break;
                }
                case GL_FLOAT_VEC3: {
                    GLfloat v[3];
                    glGetUniformfv(program, loc, v);
                    oss << "vec3) = (" << v[0] << ", " << v[1] << ", " << v[2] << ")";
                    break;
                }
                case GL_FLOAT_VEC4: {
                    GLfloat v[4];
                    glGetUniformfv(program, loc, v);
                    oss << "vec4) = (" << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3] << ")";
                    break;
                }
                case GL_FLOAT_MAT4: {
                    GLfloat v[16];
                    glGetUniformfv(program, loc, v);
                    oss << "mat4) =\n";
                    for (int row = 0; row < 4; row++) {
                        oss << "                    [ ";
                        for (int col = 0; col < 4; col++)
                            oss << v[col * 4 + row] << " ";
                        oss << "]";
                        if (row < 3)
                            oss << "\n";
                    }
                    break;
                }
                case GL_INT: {
                    GLint v;
                    glGetUniformiv(program, loc, &v);
                    oss << "int) = " << v;
                    break;
                }
                case GL_SAMPLER_2D: {
                    GLint v;
                    glGetUniformiv(program, loc, &v);
                    oss << "sampler2D) = unit " << v;
                    break;
                }
                default: oss << "type=0x" << std::hex << type << std::dec << ") = ?";
            }
            LOG_ERROR() << oss.str();
        }
    } else {
        LOG_ERROR() << "  [Uniforms] No shader bound";
    }

    

    // Check Render State
    LOG_ERROR() << "  [Render State]";
    // Check Viewport
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    LOG_ERROR() << "    Viewport: x=" << vp[0] << ", y=" << vp[1] << ", w=" << vp[2] << ", h=" << vp[3];
    LOG_ERROR() << "    Depth Test: " << (glIsEnabled(GL_DEPTH_TEST) ? "ON" : "OFF");
    LOG_ERROR() << "    Blend     : " << (glIsEnabled(GL_BLEND) ? "ON" : "OFF");
    LOG_ERROR() << "    Cull Face : " << (glIsEnabled(GL_CULL_FACE) ? "ON" : "OFF");
}
