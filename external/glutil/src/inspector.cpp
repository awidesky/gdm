#include "glutil/inspector.hpp"

#include <vector>
//TODO : use result.message directly instead of vector, and add null terminator. if logLength > written, shrink result.message and set null terminator correctly.

InspectResult Inspector::ShaderCompileResult(unsigned int shader) {
    InspectResult result;

    GLint status = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    result.ok = (status == GL_TRUE);

    GLint logLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

    if (logLength > 1) {
        std::vector<char> buffer(static_cast<size_t>(logLength), '\0');
        GLsizei written = 0;
        glGetShaderInfoLog(shader, logLength, &written, buffer.data());
        if (written > 0)
            result.message.assign(buffer.data(), static_cast<size_t>(written));
    }

    return result;
}

InspectResult Inspector::ProgramLinkResult(unsigned int program) {
    InspectResult result;

    GLint status = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    result.ok = (status == GL_TRUE);

    GLint logLength = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

    if (logLength > 1) {
        std::vector<char> buffer(static_cast<size_t>(logLength), '\0');
        GLsizei written = 0;
        glGetProgramInfoLog(program, logLength, &written, buffer.data());
        if (written > 0)
            result.message.assign(buffer.data(), static_cast<size_t>(written));
    }

    return result;
}
