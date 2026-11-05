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
# workflow's pinned default, and fails when glfw3/glm/GLEW report nothing.

find_package(glfw3 QUIET CONFIG)
find_package(glm QUIET CONFIG)
find_package(GLEW QUIET CONFIG)
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
else()
    set(_glew_target "")
endif()

# gdm sets GLEW_USE_STATIC_LIBS from GDM_GLEW_STATIC before find_package(GLEW),
# and a config package then copies GLEW::GLEW out of the static target - which
# some packages do not ship (Homebrew's glew, for one, has only the shared one)
# and which then fails the configure outright. gdm's option is documented as
# "use static GLEW when available", so the probe answers that question.
set(_glew_static "OFF")
foreach(_static GLEW::glew_s GLEW::GLEW_static libglew_static)
    if (TARGET ${_static})
        set(_glew_static "ON")
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
message(STATUS "GDM_PROBE|GDM_GLEW_VERSION|${GLEW_VERSION}")
message(STATUS "GDM_PROBE|GDM_GLEW_TARGET|${_glew_target}")
message(STATUS "GDM_PROBE|GDM_GLEW_STATIC|${_glew_static}")
message(STATUS "GDM_PROBE|GDM_CPPTRACE_VERSION|${cpptrace_VERSION}")
message(STATUS "GDM_PROBE|GDM_CPPTRACE_TARGET|${_cpptrace_target}")

if (NOT _glfw_target OR NOT _glm_target OR NOT _glew_target)
    message(FATAL_ERROR
        "the system packages are missing a usable CMake config; "
        "glfw3=${glfw3_VERSION}(${_glfw_target}) glm=${glm_VERSION}(${_glm_target}) "
        "GLEW=${GLEW_VERSION}(${_glew_target})")
endif()
