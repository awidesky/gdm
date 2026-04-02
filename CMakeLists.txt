cmake_minimum_required(VERSION 3.16) # TODO : check
project(GDM
	VERSION 0.0.1 
	LANGUAGES C CXX
	HOMEPAGE_URL "https://github.com/awidesky/gdm"
)

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
option(GDM_FORCE_GEN_GLAD "Force generate GLAD using CMake/Python" OFF)
option(GDM_BUILD_EXAMPLES "Build GDM examples" ${GDM_STANDALONE})

# -----------------------
# User-configurable dependency versions
# -----------------------
set(GDM_GLFW_VERSION "3.4.0" CACHE STRING "GLFW version")
set(GDM_GLM_VERSION  "1.0.3" CACHE STRING "GLM version")
set(GDM_GLAD_VERSION "2.0.6" CACHE STRING "GLAD version")
set(GDM_GLAD_API "gl:core=4.6" CACHE STRING "GLAD API parameter")

# For dependencies
include(cmake/Dependencies.cmake)

# -----------------------
# OpenGL (system)
# -----------------------
find_package(OpenGL REQUIRED)

# -----------------------
# GLFW
# -----------------------
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
    ALIAS_TARGET gdm::glfw
    CANDIDATE_TARGETS glfw glfw3
)

#
# -----------------------
# GLAD
# -----------------------
include(FetchContent)

set(GDM_EXTERNAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external)
set(GDM_GLAD_DEBUG_DIR ${GDM_EXTERNAL_DIR}/glad_debug)
set(GDM_GLAD_RELEASE_DIR ${GDM_EXTERNAL_DIR}/glad_release)
set(GDM_GLAD_SINGLE_DIR ${GDM_EXTERNAL_DIR}/glad)
string(FIND "${GDM_GLAD_API}" "gl:core=" GDM_GLAD_IS_GLCORE)

if (EXISTS ${GDM_GLAD_DEBUG_DIR}/src/gl.c AND EXISTS ${GDM_GLAD_RELEASE_DIR}/src/gl.c)
    message(STATUS "Found installed glad_debug and glad_release")

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
    message(STATUS "Found installed single glad")

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

elseif (GDM_GLAD_IS_GLCORE EQUAL 0)
    string(REGEX MATCH "([0-9]+\\.[0-9]+)$" GDM_GLAD_VERSION_MATCH ${GDM_GLAD_API})
    set(GDM_GLAD_API_VERSION ${CMAKE_MATCH_1})

    if (GDM_GLAD_API MATCHES "^gl:core=3\\.[0-3]$" OR GDM_GLAD_API MATCHES "^gl:core=[0-2]\\.[0-9]+$")
        set(GDM_GLAD_PREGEN_VERSION "3.3")
    else()
        set(GDM_GLAD_PREGEN_VERSION "4.6")
    endif()

    message(STATUS "Fetching pre-generated glad ${GDM_GLAD_PREGEN_VERSION}" )
    
    set(GDM_RELEASE_BASE_URL "https://github.com/awidesky/gdm/releases/download/v0.0.1")

    FetchContent_Declare(
        glad_debug_pregen
        URL ${GDM_RELEASE_BASE_URL}/glad.${GDM_GLAD_PREGEN_VERSION}_debug.zip
        SOURCE_DIR ${GDM_GLAD_DEBUG_DIR}
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_Declare(
        glad_release_pregen
        URL ${GDM_RELEASE_BASE_URL}/glad.${GDM_GLAD_PREGEN_VERSION}_release.zip
        SOURCE_DIR ${GDM_GLAD_RELEASE_DIR}
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )

    if (NOT EXISTS ${GDM_GLAD_DEBUG_DIR}/src/gl.c)
        FetchContent_MakeAvailable(glad_debug_pregen)
    endif()

    if (NOT EXISTS ${GDM_GLAD_RELEASE_DIR}/src/gl.c)
        FetchContent_MakeAvailable(glad_release_pregen)
    endif()

    add_library(gdm_glad_debug STATIC
        ${glad_debug_pregen_SOURCE_DIR}/src/gl.c
    )
    target_include_directories(gdm_glad_debug PUBLIC
        ${glad_debug_pregen_SOURCE_DIR}/include
    )

    add_library(gdm_glad_release STATIC
        ${glad_release_pregen_SOURCE_DIR}/src/gl.c
    )
    target_include_directories(gdm_glad_release PUBLIC
        ${glad_release_pregen_SOURCE_DIR}/include
    )

else()
    if (GDM_FORCE_GEN_GLAD)
        message(STATUS "Force-generating GLAD")

        find_package(Python3 REQUIRED)

        FetchContent_Declare(
            glad
            URL https://github.com/Dav1dde/glad/archive/v${GDM_GLAD_VERSION}.zip
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        FetchContent_GetProperties(glad)
        if (NOT glad_POPULATED)
            FetchContent_Populate(glad)
            add_subdirectory(${glad_SOURCE_DIR}/cmake ${glad_BINARY_DIR})
        endif()

        glad_add_library(gdm_glad_debug
            STATIC
            API ${GDM_GLAD_API}
            LANGUAGE c
            OPTIONS DEBUG
        )

        glad_add_library(gdm_glad_release
            STATIC
            API ${GDM_GLAD_API}
            LANGUAGE c
        )
    else()
        message(FATAL_ERROR
            "If you want custom glad loader other than OpenGL core, you might want to generate it yourself.\n"
            "Generating glad with cmake is tricky and requires Python3 and jinja2\n"
            "Generate glad with your configuration in https://gen.glad.sh and place \"glad\" folder in \"external\" directory.\n"
            "If you place both \"glad_debug\" and \"glad_release\", GDM will link \"glad_debug\" if build configuration is Debug and \"glad_release\" if otherwise\n"
            "If you want GDM to generate glad via CMake and Python, set GDM_FORCE_GEN_GLAD ON"
        )
    endif()
endif()

# Interface wrapper
add_library(gdm_glad INTERFACE)
add_library(gdm::glad ALIAS gdm_glad)

target_link_libraries(gdm_glad INTERFACE
    $<$<CONFIG:Debug>:gdm_glad_debug>
    $<$<NOT:$<CONFIG:Debug>>:gdm_glad_release>
)


# -----------------------
# GLM
# -----------------------

set(GLM_ENABLE_CXX_17 ON)
use_or_fetch_package(
    NAME glm
    VERSION ${GDM_GLM_VERSION}
    GIT_REPOSITORY https://github.com/g-truc/glm
    GIT_TAG ${GDM_GLM_VERSION}
)

# -----------------------
# deps
# -----------------------
add_library(gdm_deps INTERFACE)
add_library(gdm::deps ALIAS gdm_deps)

target_link_libraries(gdm_deps
    INTERFACE
        OpenGL::GL
        gdm::glfw
        gdm::glad
        glm::glm
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
