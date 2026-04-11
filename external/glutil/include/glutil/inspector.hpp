#ifndef GLUTIL_INSPECTOR
#define GLUTIL_INSPECTOR

#include <glutil/gl.hpp>
#include <string>

namespace glutil {

struct InspectResult {
    bool ok = false;
    std::string message;
};

class Inspector {
public:
    static InspectResult shaderCompileResult(GLuint shader);
    static InspectResult programLinkResult(GLuint program);
};

} // namespace glutil

#endif // GLUTIL_INSPECTOR
