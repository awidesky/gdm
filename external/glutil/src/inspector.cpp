#include <glutil/inspector.hpp>

namespace glutil {

/**
 * Retrieves the compilation result of an OpenGL shader.
 *
 * Queries:
 * - GL_COMPILE_STATUS (success/failure)
 * - GL_SHADER_INFO_LOG (compiler log message)
 *
 * If no log is available, message will be empty.
 */
InspectResult Inspector::shaderCompileResult(GLuint shader) {
    InspectResult result;

    GLint status = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    result.ok = (status == GL_TRUE);

    GLint logLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

    if (logLength == 0) {
        result.message.clear();
        return result;
    }

    result.message.resize(static_cast<size_t>(logLength));

    GLsizei written = 0;
    glGetShaderInfoLog(shader, logLength, &written, result.message.data());

    if (written <= 0) result.message.clear();
    else result.message.resize(static_cast<size_t>(written));

    return result;
}

/**
 * Retrieves the link result of an OpenGL program.
 *
 * Queries:
 * - GL_LINK_STATUS (success/failure)
 * - GL_PROGRAM_INFO_LOG (linker log message)
 *
 * If no log is available, message will be empty.
 */
InspectResult Inspector::programLinkResult(GLuint program) {
    InspectResult result;

    GLint status = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    result.ok = (status == GL_TRUE);

    GLint logLength = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

    if (logLength == 0) {
        result.message.clear();
        return result;
    }

    result.message.resize(static_cast<size_t>(logLength));

    GLsizei written = 0;
    glGetProgramInfoLog(program, logLength, &written, result.message.data());

    if (written <= 0) result.message.clear();
    else result.message.resize(static_cast<size_t>(written));

    return result;
}

} // namespace glutil
