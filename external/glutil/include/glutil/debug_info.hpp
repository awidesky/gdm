#ifndef GLUTIL_DEBUG_INFO_HPP
#define GLUTIL_DEBUG_INFO_HPP

#include <set>
#include <iosfwd>
#include <stdio.h>

namespace glutil::debug {

const std::set<std::string>& getGLExtensions();
bool hasGLExtension(const char* extName);

void printRuntimeInfo(std::ostream& os, bool verbose = false);
void printRuntimeInfo(bool verbose = false);

struct GLVersion; GLVersion parseGLVersion(const char* version);
struct GLVersion {
    int major = 0;
    int minor = 0;

    int compare(const GLVersion& other) const {
        if (major != other.major) return (major < other.major) ? -1 : 1;
        if (minor != other.minor) return (minor < other.minor) ? -1 : 1;
        return 0;
    }
public:
    bool operator==(const char* rhs) const {
        return compare(parseGLVersion(rhs)) == 0;
    }
    bool operator!=(const char* rhs) const {
        return compare(parseGLVersion(rhs)) != 0;
    }
    bool operator<(const char* rhs) const {
        return compare(parseGLVersion(rhs)) < 0;
    }
    bool operator<=(const char* rhs) const {
        return compare(parseGLVersion(rhs)) <= 0;
    }
    bool operator>(const char* rhs) const {
        return compare(parseGLVersion(rhs)) > 0;
    }
    bool operator>=(const char* rhs) const {
        return compare(parseGLVersion(rhs)) >= 0;
    }
};
GLVersion getOpenGLVersion();
inline GLVersion parseGLVersion(const char* version) {
    GLVersion ret;
    std::sscanf(version, "%d.%d", &ret.major, &ret.minor);
    return ret;
}
}
#endif // GLUTIL_DEBUG_INFO_HPP