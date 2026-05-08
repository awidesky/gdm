#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glutil/glutil.hpp>
#include <iostream>
#include <glutil/debug_snapshot.hpp>
#include "config.hpp"

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
            LOG_ERROR() << "  No 2D texture bound";

        GLint width = 0, height = 0, internalFormat = 0;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalFormat);
        LOG_ERROR() << "  Current 2D texture bound (ID: " << bound << ") Size: " << width << "x" << height
                    << ", Format: " << internalFormat;
    }
    glActiveTexture(currentUnit);
    // glGetIntegerv(GL_TEXTURE_BINDING_2D, &bound);
    // if (bound != 0) {
    //     GLint width = 0, height = 0, internalFormat = 0;
    //     glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    //     glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
    //     glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalFormat);
    //     LOG_ERROR() << "  Current 2D texture bound (ID: " << bound << ") Size: " << width << "x" << height
    //                 << ", Format: " << internalFormat;
    // } else {
    //     LOG_ERROR() << "  No 2D texture bound";
    // }

    // Check VAO/VBO/EBO Bind
    // glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &bound);
    // LOG_ERROR() << "  Current VAO bound: " << bound;'

    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &bound);
    if (bound != 0) {
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

            LOG_ERROR() << "    attrib[" << i << "]"
                        << " vbo=" << vbo << " size=" << size << " type=0x" << std::hex << type << std::dec
                        << " stride=" << stride << " offset=" << reinterpret_cast<uintptr_t>(offset);
        }
    } else {
        LOG_ERROR() << "  No VBO bound: " << bound;
    }

    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &bound);
    LOG_ERROR() << "  Current VBO bound: " << bound;
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &bound);
    LOG_ERROR() << "  Current EBO bound: " << bound;

    // Check Viewport
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    LOG_ERROR() << "  Viewport: x=" << vp[0] << ", y=" << vp[1] << ", w=" << vp[2] << ", h=" << vp[3];

    // Check Render State
    LOG_ERROR() << "  [Render State]";
    LOG_ERROR() << "    Depth Test: " << (glIsEnabled(GL_DEPTH_TEST) ? "ON" : "OFF");
    LOG_ERROR() << "    Blend     : " << (glIsEnabled(GL_BLEND) ? "ON" : "OFF");
    LOG_ERROR() << "    Cull Face : " << (glIsEnabled(GL_CULL_FACE) ? "ON" : "OFF");
}
