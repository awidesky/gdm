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
find_package(GLEW QUIET CONFIG)
# Ubuntu installs GLEW's package as lib/<arch>/cmake/glew/glew-config.cmake -
# lowercase, which CMake's find_package(GLEW) does match through its config-mode
# search, so the target is reported here. (It carries no version file.)
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

if (TARGET GLEW::GLEW)
    set(_glew_target "GLEW::GLEW")
elseif (TARGET GLEW::glew)
    set(_glew_target "GLEW::glew")
else()
    set(_glew_target "")
endif()

# Report the version even though the package cannot: its config file has no
# version file (find_package(GLEW CONFIG EXACT 2.3.1) fails on a machine that
# has 2.3.1), and it would still be nice to know which GLEW is installed. The
# version is in the library name - libGLEW.so.2.3.1, libGLEW.2.3.1.dylib - and
# LOCATION resolves it even when only a per-config location is set. Empty where
# the name holds no version (GLEW32.lib).
set(_glew_version "")
if (_glew_target)
    get_target_property(_glew_lib ${_glew_target} LOCATION)
    if (_glew_lib MATCHES "[0-9]+\\.[0-9]+(\\.[0-9]+)*")
        set(_glew_version "${CMAKE_MATCH_0}")
    endif()
    unset(_glew_lib)
endif()

# gdm accepts an installed GLEW only when it ships the variant GDM_GLEW_STATIC
# asks for, and the packages are not consistent about it: Ubuntu ships both
# libGLEW.a and libGLEW.so but only defines the shared target, Homebrew defines
# only the shared one, vcpkg defines the static one. So the probe answers the
# question this job's configure depends on, and gdm falls back to building
# glew-cmake when the answer is "no".
set(_glev_static "OFF")
foreach(_static GLEW::glev_s GLEW::GLEW_static libglew_static)
    if (TARGET ${_static})
        set(_glev_static "ON")
        break()
    endif()
endforeach()

if (TARGET cpptrace::cpptrace)
    set(_cpptrace_target "cpptrace::cpptrace")
else()
    set(_cpptrace_target "")
endif()

message(STATUS "GDM_PROBE|GDM_GLFW_VERSION|${glfw3_VERSION}")
message(STATUS "GDM_PROBE|GDM_GLFW_TARGET|${_glfw_target}")
message(STATUS "GDM_PROBE|GDM_GLM_VERSION|${glm_VERSION}")
message(STATUS "GDM_PROBE|GDM_GLM_TARGET|${_glm_target}")
# Informational only: the workflow exports lines tagged GDM_PROBE as gdm options,
# and exporting this one would set GDM_GLEW_VERSION to a concrete version - which
# means "always build from source" and silently defeats the "installed" default
# this job is testing. A distinct tag keeps it out of the way.
message(STATUS "INFO|GLEW_VERSION|${_glew_version}")
message(STATUS "GDM_PROBE|GDM_GLEW_TARGET|${_glew_target}")
message(STATUS "GDM_PROBE|GDM_GLEW_STATIC|${_glev_static}")
message(STATUS "GDM_PROBE|GDM_CPPTRACE_VERSION|${cpptrace_VERSION}")
message(STATUS "GDM_PROBE|GDM_CPPTRACE_TARGET|${_cpptrace_target}")

if (NOT _glfw_target OR NOT _glm_target)
    message(FATAL_ERROR
        "the system packages are missing a usable CMake config; "
        "glfw3=${glfw3_VERSION}(${_glfw_target}) glm=${glm_VERSION}(${_glm_target})")
endif()
