# GDM

OpenGL Dependency Manager (GDM) is a CMake-first dependency aggregator for OpenGL projects.
It resolves and links together:

- window backend (`glfw`, `freeglut`, or `none`)
- OpenGL loader (`glad`, `glew`, `glew-glad`, or `none`)
- optional math support (`glm`)
- optional debugging utility module (`glutil`)

After you specified the desired library and versions, `GDM` will search for installed libraries, download the source if installed package not existes, and provide targets that resolves to specified dependency.  
The main value is a small set of interface targets that carry link dependencies and compile definitions so application code can resolve OpenGL dependencies through all environment, and switch providers without changing includes or link lines.

## Usage Example

Following example
```
# Specify OpenGL libraries
set(GDM_WINDOW_BACKEND "glfw" CACHE STRING "" FORCE)
set(GDM_GL_LOADER "glad" CACHE STRING "" FORCE)
set(GDM_USE_GLM ON CACHE BOOL "" FORCE)
set(GDM_USE_GLUTIL ON CACHE BOOL "" FORCE)

# Version variables are set to the latest by default, so it's not guaranteed
set(GDM_GLFW_VERSION "3.4.0" CACHE STRING "" FORCE)
set(GDM_GLM_VERSION "1.0.3" CACHE STRING "" FORCE)

set(GDM_GLAD_EXTENSION
    # These are the default extensions. Remove this part if following extensions is enough for you
    "GL_ARB_debug_output;GL_EXT_debug_label;GL_EXT_debug_marker;GL_EXT_texture_compression_s3tc;GL_KHR_debug"
    CACHE STRING "" FORCE
)

# Fetch gdm
include(FetchContent)
FetchContent_Declare(
    gdm
    GIT_REPOSITORY https://github.com/awidesky/gdm.git
    GIT_TAG master
    GIT_SHALLOW TRUE
    GIT_REMOTE_UPDATE_STRATEGY CHECKOUT
    SOURCE_DIR "external/gdm"
)

FetchContent_MakeAvailable(gdm)
```
## Current CMake Targets

GDM defines the following namespaced targets in the top-level `CMakeLists.txt`:

- `gdm::window`: selected window backend provider target
- `gdm::loader`: selected OpenGL loader target
- `gdm::opengl`: system OpenGL target bridge (`OpenGL::GL` or `OpenGL::OpenGL`)
- `gdm::math`: optional GLM alias target
- `gdm::defs`: compile-time feature macros (`GDM_HAS_*`, `GDM_DEBUG`, build type macros)
- `gdm::deps`: bundle target that links `gdm::defs`, `gdm::opengl`, `gdm::window`, `gdm::loader`, and `gdm::math`
- `gdm::gdm`: meta target that links `gdm::deps`
- `gdm::glutil`: present only when `GDM_USE_GLUTIL=ON`

Using imported target name(like glm::glm or glfw, etc) should work since every imported target's IMPORTED_GLOBAL property is
set to ON, but using genearted alias target is recommended.

### What `gdm::window` does

`gdm::window` is an alias for the selected backend:

- `GDM_WINDOW_BACKEND=glfw`: aliases a GLFW target (`glfw`, `glfw3`, or `glfw::glfw`)
- `GDM_WINDOW_BACKEND=freeglut`: aliases a FreeGLUT target (`freeglut`, `freeglut_static`, etc.)
- `GDM_WINDOW_BACKEND=none`: aliases an empty interface target

Use this when you only want window-system linkage.

### What `gdm::deps` does

`gdm::deps` is the most useful consumer target. Link it to get:

- the selected OpenGL/window/loader stack
- optional GLM linkage (if enabled)
- compile definitions such as:
	- `GDM_HAS_GLFW` or `GDM_HAS_FREEGLUT`
	- `GDM_HAS_GLAD`, `GDM_HAS_GLEW`, `GDM_HAS_GLEW_GLAD`
	- `GDM_HAS_GLM`, `GDM_HAS_GLUTIL`
	- `GDM_DEBUG`

This lets application code gate includes and behavior with `#ifdef GDM_HAS_*`.

## How To Use

### 1) Add GDM as a subdirectory

```cmake
cmake_minimum_required(VERSION 3.24)
project(MyApp LANGUAGES CXX)

# Configure GDM before add_subdirectory.
set(GDM_WINDOW_BACKEND glfw CACHE STRING "")
set(GDM_GL_LOADER glad CACHE STRING "")
set(GDM_USE_GLM ON CACHE BOOL "")
set(GDM_USE_GLUTIL OFF CACHE BOOL "")

add_subdirectory(external/gdm)

add_executable(my_app src/main.cpp)
target_link_libraries(my_app PRIVATE gdm::deps)
target_compile_features(my_app PRIVATE cxx_std_17)
```

### 2) Optionally link only what you need

```cmake
# Only window backend abstraction
target_link_libraries(my_window_tool PRIVATE gdm::window)

# Full stack via meta target (equivalent dependency closure to gdm::deps)
target_link_libraries(my_app PRIVATE gdm::gdm)
```

### 3) Use feature macros in code

```cpp
#ifdef GDM_HAS_GLEW
#include <GL/glew.h>
#endif

#ifdef GDM_HAS_GLAD
#include <glad/gl.h>
#endif

#ifdef GDM_HAS_GLFW
#include <GLFW/glfw3.h>
#elif defined(GDM_HAS_FREEGLUT)
#include <GL/freeglut.h>
#endif
```

## Common Configuration Options

- `GDM_WINDOW_BACKEND`: `none`, `glfw`, `freeglut` (default: `glfw`)
- `GDM_GL_LOADER`: `none`, `glad`, `glew`, `glew-glad` (default: `glad`)
- `GDM_USE_GLM`: `ON/OFF` (default: `ON`)
- `GDM_USE_GLUTIL`: `ON/OFF` (default: `ON`)
- `GDM_BUILD_EXAMPLES`: build examples (default: `ON` when standalone)
- `GDM_BUILD_TESTS`: build tests (default: `ON` when standalone)

## Notes

- At the moment, GDM is consumed in-tree via `add_subdirectory(...)`.
- There is currently no installed package export (`install(EXPORT ...)`) in this repository yet.

