#ifndef GLUTIL_INSPECTOR
#define GLUTIL_INSPECTOR

#include <glutil/gl.hpp>
#include <string>

struct InspectResult {
    bool ok = false;
    std::string message;
};

class Inspector {
public:
    static InspectResult ShaderCompileResult(unsigned int shader);
    static InspectResult ProgramLinkResult(unsigned int program);
};

#endif // GLUTIL_INSPECTOR
