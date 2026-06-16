#ifndef GLUTIL_PATH_HPP
#define GLUTIL_PATH_HPP

#include <glutil/gl.hpp>

#include <filesystem>

namespace glutil {

/**
 * Result of a path resolution attempt.
 */
struct PathResolveResult {
    bool success = false;
    /** Absolute resolved filesystem path (valid only if success == true). */
    std::string resolvedPath{};
    /** Diagnostic log of all resolution attempts and outcomes. */
    std::string message;
};

/**
 * Returns the directory containing the currently running executable.
 * 
 * Returns empty path if resolution fails.
 */
std::filesystem::path getExecutableDirectory();
/**
 * Returns the project root directory defined by compile-time macro PROJECT_ROOT.
 *
 * This value is not computed at runtime; it is injected via build configuration.
 */
std::filesystem::path getProjectRootDirectory();
/**
 * Resolves a filesystem path using multiple fallback strategies.
 *
 * Resolution order:
 * 1. Absolute path (checked directly)
 * 2. Working directory (std::filesystem::current_path)
 * 3. Executable directory (getExecutableDirectory)
 * 4. Project root directory (getProjectRootDirectory)
 *
 * For each candidate:
 * - checks existence using fs::exists (via weakly_canonical path)
 * - logs success or failure into diagnostic message
 *
 * Returns:
 * - success == true if any candidate path exists
 * - resolvedPath contains canonical absolute path of first valid match
 * - message contains full resolution trace
 */
PathResolveResult pathResolve(const std::filesystem::path& inputPath);

} // namespace glutil

#endif // GLUTIL_PATH_HPP
