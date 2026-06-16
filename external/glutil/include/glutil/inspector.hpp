#ifndef GLUTIL_INSPECTOR
#define GLUTIL_INSPECTOR

#include <glutil/gl.hpp>
#include <string>

namespace glutil {

/**
 * Result of an OpenGL inspection query.
 * Contains success state and an optional info/log message.
 */
struct InspectResult {
    bool ok = false;     // Indicates whether the inspected operation succeeded
    std::string message; // Info log or diagnostic message (if available)
};

/**
 * Utility class for querying OpenGL compilation and linking status.
 */
class Inspector {
public:
    /**
     * Queries the compilation result of a shader object.
     *
     * Reads GL_COMPILE_STATUS and GL_SHADER_INFO_LOG
     *
     * @param shader the Shader object ID
     * @return InspectResult containing success state and compiler log
     */
    static InspectResult shaderCompileResult(GLuint shader);

    /**
     * Queries the linking result of a shader program.
     *
     * Reads GL_LINK_STATUS and GL_PROGRAM_INFO_LOG
     *
     * @param program the Program object ID
     * @return InspectResult containing success state and linker log
     */
    static InspectResult programLinkResult(GLuint program);
};

} // namespace glutil

#endif // GLUTIL_INSPECTOR