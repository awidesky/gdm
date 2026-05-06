cmake_minimum_required(VERSION 3.24)
project(GDM
	VERSION 0.0.1 
	LANGUAGES C CXX
	HOMEPAGE_URL "https://github.com/awidesky/gdm"
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Is gdm the main project, not a library?
string(COMPARE EQUAL "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}" GDM_STANDALONE)

# C++ options
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build options
if (NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Default type of build is Debug.")
endif()

option(GDM_USE_GLUTIL "Enable GLUtil module" ON)
option(GDM_BUILD_EXAMPLES "Build GDM examples" ${GDM_STANDALONE})
option(GDM_BUILD_TESTS "Build the GDM/GLUtil test programs" ${GDM_STANDALONE})
option(GDM_TLS_VERIFY "Enable TLS verification" ON)

if (GDM_BUILD_TESTS)
    enable_testing()
endif()

# Dependency providers
set(GDM_WINDOW_BACKEND "glfw" CACHE STRING
    "Window backend: none, glfw, or freeglut")
set_property(CACHE GDM_WINDOW_BACKEND
    PROPERTY STRINGS none glfw freeglut)

set(GDM_GL_LOADER "glad" CACHE STRING
    "OpenGL loader: none, glad, or glew glew-glad(glad wrapped by glew. gives glew interface but use glad internally for debug feature)")
set_property(CACHE GDM_GL_LOADER
    PROPERTY STRINGS none glad glew glew-glad)

option(GDM_USE_GLM "Enable GLM support" ON)

# Default external dependency directory; users may override via cache
set(GDM_EXTERNAL_DIR "${CMAKE_SOURCE_DIR}/external" CACHE PATH
    "Directory where GDM external dependencies are stored"
)

# -----------------------
# User-configurable dependency versions
# -----------------------

# Create version/config cache variables only for selected dependencies
if (GDM_WINDOW_BACKEND STREQUAL "glfw")
    set(GDM_GLFW_VERSION "3.4.0" CACHE STRING "GLFW version")
    find_package(Threads REQUIRED) # pre-find threads
elseif (GDM_WINDOW_BACKEND STREQUAL "freeglut")
    set(GDM_FREEGLUT_VERSION "3.8.0" CACHE STRING "FreeGLUT version")
endif()

if (GDM_USE_GLM)
    set(GDM_GLM_VERSION "1.0.3" CACHE STRING "GLM version")
endif()

if (GDM_GL_LOADER STREQUAL "glad")
    set(GDM_GLAD_VERSION "2.0.6" CACHE STRING "GLAD version")
    set(GDM_GLAD_API "4.6" CACHE STRING "GLAD GL API version (e.g. 4.6)")
    set(GDM_GLAD_PROFILE "core" CACHE STRING "GLAD GL profile")
    set_property(CACHE GDM_GLAD_PROFILE PROPERTY STRINGS core compatibility)

    set(_GDM_GLAD_EXT_DEFAULT_LT43
        "GL_ARB_debug_output;GL_EXT_debug_label;GL_EXT_debug_marker;GL_EXT_texture_compression_s3tc;GL_KHR_debug"
    )
    set(_GDM_GLAD_EXT_DEFAULT_GE43
        "GL_EXT_texture_compression_s3tc"
    )

    if (GDM_GLAD_API VERSION_LESS "4.3")
        set(_GDM_GLAD_EXTENSION_DEFAULT "${_GDM_GLAD_EXT_DEFAULT_LT43}")
    else()
        set(_GDM_GLAD_EXTENSION_DEFAULT "${_GDM_GLAD_EXT_DEFAULT_GE43}")
    endif()

    # Auto-update default when API version changes,
    # but keep user-custom extension list untouched.
    if (NOT DEFINED GDM_GLAD_EXTENSION
        OR GDM_GLAD_EXTENSION STREQUAL ""
        OR GDM_GLAD_EXTENSION STREQUAL "${_GDM_GLAD_EXT_DEFAULT_LT43}"
        OR GDM_GLAD_EXTENSION STREQUAL "${_GDM_GLAD_EXT_DEFAULT_GE43}")
        set(GDM_GLAD_EXTENSION "${_GDM_GLAD_EXTENSION_DEFAULT}" CACHE STRING
            "Additional GL extensions for glad (semicolon-separated list)" FORCE
        )
    endif()

    unset(_GDM_GLAD_EXT_DEFAULT_LT43)
    unset(_GDM_GLAD_EXT_DEFAULT_GE43)
    unset(_GDM_GLAD_EXTENSION_DEFAULT)

elseif (GDM_GL_LOADER STREQUAL "glew")
    set(GDM_GLEW_VERSION "2.3.1" CACHE STRING "GLEW version")
    option(GDM_GLEW_STATIC "Use static GLEW when available" ON)

elseif (GDM_GL_LOADER STREQUAL "glew-glad")
    # GLEW (public interface) + GLAD (actual GL entrypoints via fetch_glad_allEXT)
    set(GDM_GLEW_VERSION "2.3.1" CACHE STRING "GLEW version")

endif()

# Clean up stale cache variables when provider selection changes
if (NOT GDM_WINDOW_BACKEND STREQUAL "glfw")
    unset(GDM_GLFW_VERSION CACHE)
endif()

if (NOT GDM_WINDOW_BACKEND STREQUAL "freeglut")
    unset(GDM_FREEGLUT_VERSION CACHE)
endif()

if (NOT GDM_USE_GLM)
    unset(GDM_GLM_VERSION CACHE)
endif()

if (NOT GDM_GL_LOADER STREQUAL "glad")
    unset(GDM_GLAD_VERSION CACHE)
    unset(GDM_GLAD_API CACHE)
    unset(GDM_GLAD_PROFILE CACHE)
    unset(GDM_GLAD_EXTENSION CACHE)
    unset(GDM_FORCE_GEN_GLAD CACHE)
endif()

if (NOT (GDM_GL_LOADER STREQUAL "glew" OR GDM_GL_LOADER STREQUAL "glew-glad"))
    unset(GDM_GLEW_VERSION CACHE)
    unset(GDM_GLEW_STATIC CACHE)
endif()

# For dependencies
include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "")
if(DEFINED CMAKE_TLS_VERIFY)
    set(GDM_TLS_VERIFY ${CMAKE_TLS_VERIFY})
endif()
include(cmake/Dependencies.cmake)
include(cmake/FetchGlad.cmake)

# -----------------------
# OpenGL (system)
# -----------------------
find_package(OpenGL REQUIRED)
set(_gdm_defs GDM_HAS_OPENGL)


# measure start time for dependency fetching and generation
string(TIMESTAMP START_TIME "%s" UTC)

# -----------------------
# Window backend (GLFW, FreeGLUT)
# -----------------------
if (GDM_WINDOW_BACKEND STREQUAL "none")
    add_library(gdm_window INTERFACE)
    add_library(gdm::window ALIAS gdm_window)
    list(APPEND _gdm_defs GDM_HAS_WINDOW_NONE)
elseif (GDM_WINDOW_BACKEND STREQUAL "glfw")
    # Derive git tag from version (strip trailing .0)
    set(GDM_GLFW_GIT_TAG ${GDM_GLFW_VERSION})
    string(REGEX REPLACE "\\.0$" "" GDM_GLFW_GIT_TAG "${GDM_GLFW_GIT_TAG}")

    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_LIBRARY_TYPE STATIC CACHE STRING "" FORCE)

    use_or_fetch_package(
        NAME glfw3
        VERSION ${GDM_GLFW_VERSION}
        GIT_REPOSITORY https://github.com/glfw/glfw
        GIT_TAG ${GDM_GLFW_GIT_TAG}
        ALIAS_TARGET gdm::window
        CANDIDATE_TARGETS glfw glfw3
    )

    if (MSVC)
        foreach(t glfw glfw3)
            if(TARGET ${t})
                target_compile_options(${t} PRIVATE "/utf-8")
            endif()
        endforeach()
    endif()
    list(APPEND _gdm_defs GDM_HAS_GLFW)
elseif (GDM_WINDOW_BACKEND STREQUAL "freeglut")
    set(GDM_FREEGLUT_GIT_TAG "v${GDM_FREEGLUT_VERSION}")

    set(FREEGLUT_BUILD_DEMOS OFF CACHE BOOL "" FORCE)

    use_or_fetch_package(
        NAME freeglut
        VERSION ${GDM_FREEGLUT_VERSION}
        GIT_REPOSITORY https://github.com/freeglut/freeglut
        GIT_TAG ${GDM_FREEGLUT_GIT_TAG}
        ALIAS_TARGET gdm::window
        CANDIDATE_TARGETS freeglut freeglut_static freeglut::freeglut freeglut::freeglut_static
    )
    list(APPEND _gdm_defs GDM_HAS_FREEGLUT)
else()
    message(FATAL_ERROR "[GDM] Unsupported GDM_WINDOW_BACKEND: ${GDM_WINDOW_BACKEND}")
endif()

# -----------------------
# OpenGL loader (GLAD, GLEW)
# -----------------------

if (GDM_GL_LOADER STREQUAL "none")
    add_library(gdm_loader INTERFACE)
    add_library(gdm::loader ALIAS gdm_loader)
    list(APPEND _gdm_defs GDM_HAS_LOADER_NONE)
elseif (GDM_GL_LOADER STREQUAL "glad")
    set(GDM_GLAD_DEBUG_DIR ${GDM_EXTERNAL_DIR}/glad_debug)
    set(GDM_GLAD_RELEASE_DIR ${GDM_EXTERNAL_DIR}/glad_release)
    set(GDM_GLAD_SINGLE_DIR ${GDM_EXTERNAL_DIR}/glad)

    if (EXISTS ${GDM_GLAD_DEBUG_DIR}/src/gl.c AND EXISTS ${GDM_GLAD_RELEASE_DIR}/src/gl.c)
        message(STATUS "[GDM] Found installed glad_debug and glad_release")

    elseif (EXISTS ${GDM_GLAD_SINGLE_DIR}/src/gl.c)
        message(STATUS "[GDM] Found installed single glad")

    else()
        message(STATUS "[GDM] Fetching glad debug/release mode")
        fetch_glad(
            OUT_VAR _glad_debug_src
            NAME glad_debug_remote
            DEST_DIR ${GDM_GLAD_DEBUG_DIR}
            APIS "gl=${GDM_GLAD_API}"
            PROFILES "gl=${GDM_GLAD_PROFILE}"
            OPTIONS DEBUG
            EXTENSIONS ${GDM_GLAD_EXTENSION}
        )

        fetch_glad(
            OUT_VAR _glad_release_src
            NAME glad_release_remote
            DEST_DIR ${GDM_GLAD_RELEASE_DIR}
            APIS "gl=${GDM_GLAD_API}"
            PROFILES "gl=${GDM_GLAD_PROFILE}"
            EXTENSIONS ${GDM_GLAD_EXTENSION}
        )
    endif()

    if (EXISTS ${GDM_GLAD_DEBUG_DIR}/src/gl.c AND EXISTS ${GDM_GLAD_RELEASE_DIR}/src/gl.c)
        add_library(gdm_glad_debug STATIC
            ${GDM_GLAD_DEBUG_DIR}/src/gl.c
        )
        target_include_directories(gdm_glad_debug PUBLIC
            ${GDM_GLAD_DEBUG_DIR}/include
        )

        add_library(gdm_glad_release STATIC
            ${GDM_GLAD_RELEASE_DIR}/src/gl.c
        )
        target_include_directories(gdm_glad_release PUBLIC
            ${GDM_GLAD_RELEASE_DIR}/include
        )

    elseif (EXISTS ${GDM_GLAD_SINGLE_DIR}/src/gl.c)

        add_library(gdm_glad_single STATIC
            ${GDM_GLAD_SINGLE_DIR}/src/gl.c
        )
        target_include_directories(gdm_glad_single PUBLIC
            ${GDM_GLAD_SINGLE_DIR}/include
        )

        add_library(gdm_glad_debug INTERFACE)
        add_library(gdm_glad_release INTERFACE)
        target_link_libraries(gdm_glad_debug INTERFACE gdm_glad_single)
        target_link_libraries(gdm_glad_release INTERFACE gdm_glad_single)
    else()
        message(FATAL_ERROR "[GDM] glad source was not generated correctly.")
    endif()

    add_library(gdm_loader INTERFACE)
    add_library(gdm::loader ALIAS gdm_loader)
    add_dependencies(gdm_loader gdm_glad_debug gdm_glad_release)
    target_link_libraries(gdm_loader INTERFACE
        $<$<CONFIG:Debug>:gdm_glad_debug>
        $<$<NOT:$<CONFIG:Debug>>:gdm_glad_release>
    )
    list(APPEND _gdm_defs GDM_HAS_GLAD)

elseif (GDM_GL_LOADER STREQUAL "glew")
    if (GDM_WINDOW_BACKEND STREQUAL "none")
        message(FATAL_ERROR "[GDM] GDM_GL_LOADER=glew-glad requires GDM_WINDOW_BACKEND=glfw or freeglut.")
    endif()
    set(GLEW_USE_STATIC_LIBS ${GDM_GLEW_STATIC})

    # Prefer system-installed GLEW package first.
    find_package(GLEW QUIET)

    if (TARGET GLEW::GLEW)
        message(STATUS "[GDM] Found installed GLEW target: GLEW::GLEW")
    else()
        # Fallback to fetching glew-cmake.
        set(GDM_GLEW_GIT_TAG "glew-cmake-${GDM_GLEW_VERSION}")

        # Configure glew-cmake to build only libs (no glewinfo/visualinfo)
        set(ONLY_LIBS ON CACHE BOOL "" FORCE)

        if (GDM_GLEW_STATIC)
            set(glew-cmake_BUILD_SHARED OFF CACHE BOOL "" FORCE)
            set(glew-cmake_BUILD_STATIC ON CACHE BOOL "" FORCE)
            set(_GDM_GLEW_CANDIDATES libglew_static libglew_shared GLEW::GLEW)
        else()
            set(glew-cmake_BUILD_SHARED ON CACHE BOOL "" FORCE)
            set(glew-cmake_BUILD_STATIC OFF CACHE BOOL "" FORCE)
            set(_GDM_GLEW_CANDIDATES libglew_shared libglew_static GLEW::GLEW)
        endif()

        use_or_fetch_package(
            NAME glew-cmake
            VERSION ${GDM_GLEW_VERSION}
            GIT_REPOSITORY https://github.com/Perlmint/glew-cmake
            GIT_TAG ${GDM_GLEW_GIT_TAG}
            ALIAS_TARGET GLEW::GLEW
            CANDIDATE_TARGETS ${_GDM_GLEW_CANDIDATES}
        )

        unset(_GDM_GLEW_CANDIDATES)
    endif()

    if (NOT TARGET GLEW::GLEW)
        message(FATAL_ERROR "[GDM] Failed to resolve GLEW target (expected GLEW::GLEW).")
    endif()

    add_library(gdm_loader INTERFACE)
    add_library(gdm::loader ALIAS gdm_loader)
    target_link_libraries(gdm_loader INTERFACE GLEW::GLEW)
    list(APPEND _gdm_defs GDM_HAS_GLEW)

elseif (GDM_GL_LOADER STREQUAL "glew-glad")
    # Dedicated GLAD dirs for glew-glad (ALL extensions, gl=4.6 core)
    set(GDM_GLAD_ALLEXT_DEBUG_DIR   ${GDM_EXTERNAL_DIR}/glad_ALLEXT_debug)
    set(GDM_GLAD_ALLEXT_RELEASE_DIR ${GDM_EXTERNAL_DIR}/glad_ALLEXT_release)

    if (EXISTS ${GDM_GLAD_ALLEXT_DEBUG_DIR}/src/gl.c
        AND EXISTS ${GDM_GLAD_ALLEXT_RELEASE_DIR}/src/gl.c)
        message(STATUS "[GDM] Found installed glad_ALLEXT_debug and glad_ALLEXT_release")
    else()
        message(STATUS "[GDM] Fetching glad (ALL extensions) for glew-glad debug/release mode")

        fetch_glad_allEXT(
            OUT_VAR _glad_allex_debug_src
            NAME glad_allex_debug_remote
            DEST_DIR ${GDM_GLAD_ALLEXT_DEBUG_DIR}
            GLAD_OPTIONS DEBUG
        )

        fetch_glad_allEXT(
            OUT_VAR _glad_allex_release_src
            NAME glad_allex_release_remote
            DEST_DIR ${GDM_GLAD_ALLEXT_RELEASE_DIR}
        )
    endif()

    if (NOT EXISTS ${GDM_GLAD_ALLEXT_DEBUG_DIR}/src/gl.c
        OR NOT EXISTS ${GDM_GLAD_ALLEXT_RELEASE_DIR}/src/gl.c)
        message(FATAL_ERROR "[GDM] glad_ALLEXT source was not generated correctly (glew-glad).")
    endif()

    add_library(gdm_glad_allext_debug STATIC
        ${GDM_GLAD_ALLEXT_DEBUG_DIR}/src/gl.c
    )
    target_include_directories(gdm_glad_allext_debug PUBLIC
        ${GDM_GLAD_ALLEXT_DEBUG_DIR}/include
    )

    add_library(gdm_glad_allext_release STATIC
        ${GDM_GLAD_ALLEXT_RELEASE_DIR}/src/gl.c
    )
    target_include_directories(gdm_glad_allext_release PUBLIC
        ${GDM_GLAD_ALLEXT_RELEASE_DIR}/include
    )

    # GLEW (always fetch from glew-cmake for glew-glad)
    set(GDM_GLEW_GIT_TAG "glew-cmake-${GDM_GLEW_VERSION}")

    # Configure glew-cmake to build only libs (no glewinfo/visualinfo)
    set(ONLY_LIBS ON CACHE BOOL "" FORCE)

    set(glew-cmake_BUILD_SHARED OFF CACHE BOOL "" FORCE)
    set(glew-cmake_BUILD_STATIC ON CACHE BOOL "" FORCE)

    use_or_fetch_package(
        NAME glew-cmake
        VERSION ${GDM_GLEW_VERSION}
        GIT_REPOSITORY https://github.com/Perlmint/glew-cmake
        GIT_TAG ${GDM_GLEW_GIT_TAG}
        CANDIDATE_TARGETS libglew_static
    )
    if (NOT TARGET libglew_static)
        message(FATAL_ERROR "[GDM] Failed to resolve glew-cmake target for glew-glad.")
    endif()


    # Find the directory that contains GL/glew.h
    set(_gdm_real_glew_root "${GDM_EXTERNAL_DIR}/glew-cmake-${GDM_GLEW_VERSION}/include")
    if (NOT _gdm_real_glew_root)
        message(FATAL_ERROR "[GDM] Could not locate real GL/glew.h in ${_gdm_real_glew_root}")
    endif()

    # Generate glew-glad wrapper header (fake glew.h)
    set(_gdm_real_glew_h "${_gdm_real_glew_root}/GL/glew.h")

    set(_gdm_glewglad_gen_root "${CMAKE_BINARY_DIR}/_gdm_deps/glew_glad")
    set(_gdm_glewglad_gen_inc  "${_gdm_glewglad_gen_root}/include")
    set(_gdm_fake_glew_h "${_gdm_glewglad_gen_inc}/GL/glew.h")

    # Skip header generation if it already exists
    if (NOT EXISTS "${_gdm_fake_glew_h}")
        file(MAKE_DIRECTORY "${_gdm_glewglad_gen_inc}/GL")
        file(STRINGS "${_gdm_real_glew_h}" _gdm_glew_h_lines)
        message(DEBUG "glew-glad header generation dir : ${_gdm_glewglad_gen_inc}")

        set(_gdm_strip_define_count 0)
        set(_gdm_strip_typedef_count 0)
        set(_gdm_strip_export_count 0)

        set(_gdm_fake_glew_h_text [=[/* Auto-generated by GDM: GLEW interface, but gl function loading by GLAD */
#ifndef GDM_GLEW_GLAD_GLEW_H
#define GDM_GLEW_GLAD_GLEW_H

]=])
        foreach(line IN LISTS _gdm_glew_h_lines)
            # Strip: #define glXxx GLEW_GET_FUN(...)
            if(line MATCHES "^[ \t]*#[ \t]*define[ \t]+gl[A-Za-z0-9_]+[ \t]+GLEW_GET_FUN[ \t]*[(]")
                math(EXPR _gdm_strip_define_count "${_gdm_strip_define_count} + 1")
                continue()
            endif()
            # Strip: typedef ... PFNGLXXXPROC ... (opening line of typedef, may be multi-line)
            if(line MATCHES "^[ \t]*typedef[ \t]+[^;]*[(]GLAPIENTRY[ \t]*[*][ \t]*PFNGL[A-Z0-9_]+PROC[)][ \t]*[(]")
                math(EXPR _gdm_strip_typedef_count "${_gdm_strip_typedef_count} + 1")
                continue()
            endif()
            # Strip: GLEW_FUN_EXPORT PFNGLXXXPROC __glewXxx;
            if(line MATCHES "^[ \t]*GLEW_FUN_EXPORT[ \t]+PFNGL[A-Z0-9_]+[ \t]+__glew[A-Za-z0-9_]+[ \t]*;")
                math(EXPR _gdm_strip_export_count "${_gdm_strip_export_count} + 1")
                continue()
            endif()
            string(APPEND _gdm_fake_glew_h_text "${line}\n")
        endforeach()
        string(APPEND _gdm_fake_glew_h_text [=[

// if not undef, glew.h will complain that another gl header is included.
#undef __gl_h_
#undef __glext_h_
// glad define some values like 0x000A, and glew does 0xA. so, undef or we get a warning
#if defined(_MSC_VER)
	#pragma warning(push)
	#pragma warning(disable : 4005)
	#include <glad/gl.h>
	#pragma warning(pop)
#elif defined(__GNUC__)
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wmacro-redefined"
	#include <glad/gl.h>
	#pragma GCC diagnostic pop
#elif defined(__clang__)
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wmacro-redefined"
	#include <glad/gl.h>
	#pragma clang diagnostic pop
#else
	#include <glad/gl.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif
GLEWAPI GLenum GLEWAPIENTRY gdm_glew_glad_glewInit(void);
#ifdef __cplusplus
}
#endif
#define glewInit gdm_glew_glad_glewInit
#endif //GDM_GLEW_GLAD_GLEW_H
]=])
        file(WRITE "${_gdm_fake_glew_h}" "${_gdm_fake_glew_h_text}")
        unset(_gdm_fake_glew_h_text)
        unset(_gdm_glew_h_lines)
        message(STATUS
            "[GDM] glew-glad wrapper: "
            "removed ${_gdm_strip_define_count} gl* functions, "
            "${_gdm_strip_typedef_count} PFNGL typedefs, "
            "${_gdm_strip_export_count} GLEW_FUN_EXPORT declarations"
        )
    else()
        message(DEBUG "glew-glad wrapper header already exists, skipping generation")
    endif()

    # add glew wrapper front target
    add_library(gdm_glew_glad STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/glewToGlad.cpp
    )

    target_include_directories(gdm_glew_glad
        PUBLIC ${_gdm_glewglad_gen_inc}
    )
    unset(_gdm_real_glew_root)
    unset(_gdm_glewglad_gen_inc)
    unset(_gdm_glewglad_gen_root)

    add_library(gdm_glewglad_glad INTERFACE)
    add_dependencies(gdm_glewglad_glad gdm_glad_allext_debug gdm_glad_allext_release)
    target_link_libraries(gdm_glewglad_glad INTERFACE
        $<$<CONFIG:Debug>:gdm_glad_allext_debug>
        $<$<NOT:$<CONFIG:Debug>>:gdm_glad_allext_release>
    )
    target_link_libraries(gdm_glew_glad
        PUBLIC
            gdm_glewglad_glad
        PRIVATE
            gdm::window
            libglew_static
    )
    target_compile_definitions(gdm_glew_glad PRIVATE GDM_HAS_GLEW_GLAD)

    add_library(gdm_loader INTERFACE)
    add_library(gdm::loader ALIAS gdm_loader)
    target_link_libraries(gdm_loader INTERFACE gdm_glew_glad)

    list(APPEND _gdm_defs GDM_HAS_GLEW)
    list(APPEND _gdm_defs GDM_HAS_GLAD)
    list(APPEND _gdm_defs GDM_HAS_GLEW_GLAD)

    set_target_properties(gdm_glad_allext_debug   PROPERTIES FOLDER "GLEW_GLAD")
    set_target_properties(gdm_glad_allext_release PROPERTIES FOLDER "GLEW_GLAD")
    set_target_properties(libglew_static          PROPERTIES FOLDER "GLEW_GLAD")
    set_target_properties(gdm_glew_glad           PROPERTIES FOLDER "GLEW_GLAD")

else()
    message(FATAL_ERROR "[GDM] Unsupported GDM_GL_LOADER: ${GDM_GL_LOADER}")
endif()

if (TARGET gdm_glad_debug)
    set_target_properties(gdm_glad_debug PROPERTIES FOLDER "GLAD")
endif()
if (TARGET gdm_glad_release)
    set_target_properties(gdm_glad_release PROPERTIES FOLDER "GLAD")
endif()
if (TARGET gdm_glad_single)
    set_target_properties(gdm_glad_single PROPERTIES FOLDER "GLAD")
endif()


# -----------------------
# GLM
# -----------------------
if (GDM_USE_GLM)
    set(GLM_ENABLE_CXX_17 ON)
    use_or_fetch_package(
        NAME glm
        VERSION ${GDM_GLM_VERSION}
        GIT_REPOSITORY https://github.com/g-truc/glm
        GIT_TAG ${GDM_GLM_VERSION}
    )

    add_library(gdm_math INTERFACE)
    add_library(gdm::math ALIAS gdm_math)
    target_link_libraries(gdm_math INTERFACE glm::glm)
    list(APPEND _gdm_defs GDM_HAS_GLM)
else()
    add_library(gdm_math INTERFACE)
    add_library(gdm::math ALIAS gdm_math)
endif()

# measure end time for dependency fetching and generation
string(TIMESTAMP END_TIME "%s" UTC)
math(EXPR ELAPSED_TIME "${END_TIME} - ${START_TIME}")
message(STATUS "[GDM] Resolving dependencies took ${ELAPSED_TIME} seconds")

# -----------------------
# deps
# -----------------------
add_library(gdm_deps INTERFACE)
add_library(gdm::deps ALIAS gdm_deps)

target_link_libraries(gdm_deps
    INTERFACE
        OpenGL::GL
        gdm::window
        gdm::loader
        gdm::math
)

set_property(TARGET gdm_deps PROPERTY INTERFACE_COMPILE_DEFINITIONS "")

# -----------------------
# GLUtil (internal, optional)
# -----------------------
if (GDM_USE_GLUTIL)
    add_subdirectory(external/glutil)
    list(APPEND _gdm_defs GDM_HAS_GLUTIL)
endif()

target_compile_definitions(gdm_deps INTERFACE ${_gdm_defs})
if (GDM_GL_LOADER STREQUAL "glew-glad")
    target_compile_definitions(gdm_glew_glad PRIVATE ${_gdm_defs})
endif()
unset(_gdm_defs)

# -----------------------
# meta target (for user)
# -----------------------
add_library(gdm INTERFACE)
add_library(gdm::gdm ALIAS gdm)

target_link_libraries(gdm
    INTERFACE
        gdm::deps
)

if (GDM_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
