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

if (GDM_BUILD_TESTS)
    enable_testing()
endif()

# Dependency providers
set(GDM_WINDOW_BACKEND "glfw" CACHE STRING
    "Window backend: none, glfw, or freeglut")
set_property(CACHE GDM_WINDOW_BACKEND
    PROPERTY STRINGS none glfw freeglut)

set(GDM_GL_LOADER "glad" CACHE STRING
    "OpenGL loader: none, glad, or glew")
set_property(CACHE GDM_GL_LOADER
    PROPERTY STRINGS none glad glew)

option(GDM_USE_GLM "Enable GLM support" ON)

# -----------------------
# User-configurable dependency versions
# -----------------------

# Create version/config cache variables only for selected dependencies
if (GDM_WINDOW_BACKEND STREQUAL "glfw")
    set(GDM_GLFW_VERSION "3.4.0" CACHE STRING "GLFW version")
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

    option(GDM_FORCE_GEN_GLAD "Force generate GLAD using CMake/Python" OFF)
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

# For dependencies
include(cmake/Dependencies.cmake)
include(cmake/FetchGlad.cmake)

# -----------------------
# OpenGL (system)
# -----------------------
find_package(OpenGL REQUIRED)

# -----------------------
# Window backend (GLFW, FreeGLUT)
# -----------------------
if (GDM_WINDOW_BACKEND STREQUAL "none")
    add_library(gdm_window INTERFACE)
    add_library(gdm::window ALIAS gdm_window)
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
else()
    message(FATAL_ERROR "[GDM] Unsupported GDM_WINDOW_BACKEND: ${GDM_WINDOW_BACKEND}")
endif()

# -----------------------
# OpenGL loader (GLAD, GLEW)
# -----------------------

# [TEST] fetch all-extension glad package
#    fetch_glad_allEXT(
#        OUT_VAR GDM_GLAD_ALLEXT_DIR
#        NAME glad_debug_remote
#        DEST_DIR "${GDM_ROOT_DIR}/external/glad_ALLEXT"
#        GLAD_OPTIONS DEBUG
#    )
#    message(STATUS "[GDM][TEST] GLAD all-extension dir: ${GDM_GLAD_ALLEXT_DIR}")

if (GDM_GL_LOADER STREQUAL "none")
    add_library(gdm_loader INTERFACE)
    add_library(gdm::loader ALIAS gdm_loader)
elseif (GDM_GL_LOADER STREQUAL "glad")
    include(FetchContent)

    set(GDM_EXTERNAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external)
    set(GDM_GLAD_DEBUG_DIR ${GDM_EXTERNAL_DIR}/glad_debug)
    set(GDM_GLAD_RELEASE_DIR ${GDM_EXTERNAL_DIR}/glad_release)
    set(GDM_GLAD_SINGLE_DIR ${GDM_EXTERNAL_DIR}/glad)

    if (EXISTS ${GDM_GLAD_DEBUG_DIR}/src/gl.c AND EXISTS ${GDM_GLAD_RELEASE_DIR}/src/gl.c)
        message(STATUS "[GDM] Found installed glad_debug and glad_release")

    elseif (EXISTS ${GDM_GLAD_SINGLE_DIR}/src/gl.c)
        message(STATUS "[GDM] Found installed single glad")

    else()
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
        message(STATUS "[GDM] Found installed single glad")

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

elseif (GDM_GL_LOADER STREQUAL "glew")
    # TODO : fetch https://github.com/Perlmint/glew-cmake
    find_package(GLEW REQUIRED)
    add_library(gdm_loader INTERFACE)
    add_library(gdm::loader ALIAS gdm_loader)
    target_link_libraries(gdm_loader INTERFACE GLEW::GLEW)
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
else()
    add_library(gdm_math INTERFACE)
    add_library(gdm::math ALIAS gdm_math)
endif()

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

# -----------------------
# GLUtil (internal, optional)
# -----------------------
if (GDM_USE_GLUTIL)
    add_subdirectory(external/glutil)
endif()

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
