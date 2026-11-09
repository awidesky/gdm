#ifndef GLUTIL_DEBUG_INFO_HPP
#define GLUTIL_DEBUG_INFO_HPP

#include <cassert>
#include <cstdio>
#include <iosfwd>
#include <iostream>
#include <set>
#include <string>

namespace glutil::debug {

/** Cached set of supported OpenGL extension strings */
const std::set<std::string>& getGLExtensions();
/** Check whether a specific OpenGL extension is available */
bool hasGLExtension(const char* extName);

/**
 * Print runtime OpenGL / graphics API information.
 * @param os output stream
 * @param verbose if true, prints extended capability limits
 */
void printRuntimeInfo(bool verbose = false, std::ostream& os = std::cout);
/** Query GPU memory information using vendor-specific extensions or system interfaces */
bool printGpuMemoryInfo(std::ostream& os);

/** OpenGL version wrapper used for safe version comparison. */
struct GLVersion;
/** Parse OpenGL version string into GLVersion. */
GLVersion parseGLVersion(const char* version);
/**
 * Simple OpenGL version representation (major.minor).
 * Used for conditional feature queries.
 */
struct GLVersion {
    int major = -1;
    int minor = -1;

    /** Compare two GL versions */
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

    /**
     * Format as "major.minor" (e.g. "4.6").
     * Returns "UNKNOWN" if the version is invalid (major or minor < 0).
     */
    std::string toString() const {
        if (major < 0 || minor < 0) return "UNKNOWN";

        // OpenGL versions have a single digit in both components (1.0 - 4.6),
        // so the digits go straight into the string, no conversion needed.
        std::string ret = " . ";
        ret[0] = static_cast<char>('0' + major);
        ret[2] = static_cast<char>('0' + minor);
        return ret;
    }
};
/** Get current OpenGL context version */
GLVersion getOpenGLVersion();
/**
 * Query the best available OpenGL version the system can provide, by creating a context per candidate.
 * Returns {-1, -1} when that is impossible;
 * freeglut exits the process on the first context it cannot provide,
 * so the probe cannot run and reports {-1, -1}.
 */
GLVersion availableGLversion();
inline GLVersion parseGLVersion(const char* version) {
    GLVersion ret;

    (void)
#ifdef _MSC_VER
    ::sscanf_s
#else
    std::sscanf
#endif
               (version, "%d.%d", &ret.major, &ret.minor);

    return ret;
}
}
#endif // GLUTIL_DEBUG_INFO_HPP