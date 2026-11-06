cmake_minimum_required(VERSION 3.24)
project(gdm_dependency_probe LANGUAGES C CXX)

# Version probe for the CI's "installed dependencies" job.
#
# gdm resolves a dependency in three tiers: a pre-existing
# ${GDM_EXTERNAL_DIR}/<name>-<version>/ directory, then
# find_package(<name> CONFIG EXACT <version>), then a download. An installed
# package is only *used* when its version equals the requested one exactly, so
# the CI must pin GDM_<lib>_VERSION to whatever the package manager actually
# installed - hardcoding that would drift every time a runner image changes.
#
# This is a real project rather than a `cmake -P` script, because glfw's config
# file calls find_dependency(Threads), which only works with languages enabled.
#
#     cmake -S .github/scripts/probe -B probe-build -DCMAKE_PREFIX_PATH=<prefix>
#
# Prints one machine-readable line per dependency to stdout:
#
#     GDM_PROBE|<cache variable>|<value>
#
# The workflow turns each line into an environment variable, which overrides the
# workflow's pinned default, and fails when glfw3/glm report nothing.

find_package(glfw3 QUIET CONFIG)
find_package(glm QUIET CONFIG)
# GLEW is not probed: gdm always builds GLEW from source (glew-cmake, at the
# resolved version), so no installed GLEW is consulted - and its config packages
# cannot report a version anyway. Ubuntu's, for one, installs it as
# lib/<arch>/cmake/glev/glev-config.cmake, which CMake's GLEW search does not
# match on a case-sensitive filesystem.
find_package(cpptrace QUIET CONFIG)

# gdm accepts either target name (see use_or_fetch_package CANDIDATE_TARGETS),
# so report the one this machine actually provides, not just the version: a
# package can ship a version file and no target, and then nothing would link.
if (TARGET glfw3)
    set(_glfw_target "glfw3")
elseif (TARGET glfw)
    set(_glfw_target "glfw")
else()
    set(_glfw_target "")
endif()

if (TARGET glm::glm)
    set(_glm_target "glm::glm")
else()
    set(_glm_target "")
endif()

if (TARGET cpptrace::cpptrace)
    set(_cpptrace_target "cpptrace::cpptrace")
else()
    set(_cpptrace_target "")
endif()

message(STATUS "GDM_PROBE|GDM_GLFW_VERSION|${glfw3_VERSION}")
message(STATUS "GDM_PROBE|GDM_GLFW_TARGET|${_glfw_target}")
message(STATUS "GDM_PROBE|GDM_GLM_VERSION|${glm_VERSION}")
message(STATUS "GDM_PROBE|GDM_GLM_TARGET|${_glm_target}")
message(STATUS "GDM_PROBE|GDM_CPPTRACE_VERSION|${cpptrace_VERSION}")
message(STATUS "GDM_PROBE|GDM_CPPTRACE_TARGET|${_cpptrace_target}")

if (NOT _glfw_target OR NOT _glm_target)
    message(FATAL_ERROR
        "the system packages are missing a usable CMake config; "
        "glfw3=${glfw3_VERSION}(${_glfw_target}) glm=${glm_VERSION}(${_glm_target})")
endif()
