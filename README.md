# GDM - OpenGL Dependency Manager

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![CMake](https://img.shields.io/badge/CMake-%3E%3D3.24-blue)](CMakeLists.txt)

**GDM** is a CMake-based OpenGL dependency manager for OpenGL projects.  

Specify your OpenGL libraries(`glfw`, `freeglut`, `glad`, `glew`, `glm`, etc) and desired version, and GDM will:
- Find installed packages(via `find_package`)
- Download sources under `${GDM_EXTERNAL_DIR}` if not installed
- Provide clean namespaced targets to link  
  
so that your dependencies would work..  
- in all OS and build system that cmake supports,
- regardless of which package managers you used to install the packages,
- or even when you did not install the required package,
- without adding platform-specific compile/link options,
- or embedding library files inside your project,

without any external tools.

---

## Table of Contents

- [GDM - OpenGL Dependency Manager](#gdm---opengl-dependency-manager)
  - [Table of Contents](#table-of-contents)
  - [Supported OpenGL Libraries](#supported-opengl-libraries)
  - [Usage Examples](#usage-examples)
    - [1. Minimum Example - Project with `glfw`, `glad`, `glm`](#1-minimum-example---project-with-glfw-glad-glm)
    - [2. Detailed Example - Specifying versions and options](#2-detailed-example---specifying-versions-and-options)
    - [OpenGL Application Examples](#opengl-application-examples)
  - [Dependency Resolution](#dependency-resolution)
    - [Advantages over plain `FetchContent`](#advantages-over-plain-fetchcontent)
  - [Generated CMake Targets](#generated-cmake-targets)
    - [What `gdm::window` does](#what-gdmwindow-does)
    - [What `gdm::loader` does](#what-gdmloader-does)
    - [What `gdm::opengl` does](#what-gdmopengl-does)
    - [What `gdm::math` does](#what-gdmmath-does)
    - [What `gdm::defs` does](#what-gdmdefs-does)
    - [What `gdm::deps` does](#what-gdmdeps-does)
    - [What `gdm::gdm` does](#what-gdmgdm-does)
    - [What `gdm::glutil` does](#what-gdmglutil-does)
  - [How To Use](#how-to-use)
    - [1) Add GDM as a subdirectory](#1-add-gdm-as-a-subdirectory)
    - [Linking selectively](#linking-selectively)
  - [CMake Targets](#cmake-targets)
    - [Feature macros (`gdm::defs`)](#feature-macros-gdmdefs)
  - [Configuration Options](#configuration-options)
    - [Per-provider version options](#per-provider-version-options)
- [GLUtil - OpenGL Utility Library](#glutil---opengl-utility-library)
  - [Core Modules](#core-modules)
    - [Shader example](#shader-example)
  - [Debug System Overview](#debug-system-overview)
    - [Automatic debug initialization](#automatic-debug-initialization)
    - [GL object leak detection](#gl-object-leak-detection)
    - [Automatic GL object labeling](#automatic-gl-object-labeling)
    - [GL Error logging with stack trace](#gl-error-logging-with-stack-trace)
    - [Debug info functions](#debug-info-functions)
    - [GL state snapshots](#gl-state-snapshots)
  - [glew-glad Hybrid Mode](#glew-glad-hybrid-mode)
  - [Project Structure](#project-structure)
  - [License](#license)

## Supported OpenGL Libraries

- **Window backend** (`glfw`, `freeglut`, or `none`)
- **OpenGL function loader** (`glad`, `glew`, `glew-glad`, or `none`)
- **OpenGL Mathematics** (`glm`)
- **OpenGL debugging & resource utility(optional, explained later)** (`glutil`)

After you specified the desired library and versions, `GDM` will search for installed libraries, download the source if installed package does not exist, and provide targets that resolve to specified dependencies (like `gdm::window`, `gdm::loader`, `gdm::math`, `gdm::opengl`) without changing includes or link lines.

## Usage Examples

### 1. Minimum Example - Project with `glfw`, `glad`, `glm`

Following example will search for installed latest version of `glfw` and `glm`(assumes you already have `GDM` at `external/gdm` directory).  
If not installed, download the sources in `${GDM_EXTERNAL_DIR}`, which defaults to `${CMAKE_SOURCE_DIR}/external`, and configuration is OpenGL 4.6 Core profile. [Per-provider version options](#per-provider-version-options)
```cmake
set(GDM_WINDOW_BACKEND  "glfw"  CACHE STRING "")
set(GDM_GL_LOADER       "glad"  CACHE STRING "")
set(GDM_USE_GLM           ON    CACHE BOOL   "")

add_subdirectory(external/gdm)

add_executable(my_app src/main.cpp)
target_link_libraries(my_app PRIVATE gdm::deps)
```
_It's not even minimum!_ You can omit the part that selects the library name, since the default is `glfw + glad(gl:core=4.6) + glm` combination.  
The `gdm::deps` target links all OpenGL dependencies requested. It can be linked into your project target [Generated CMake Targets](#generated-cmake-targets)

### 2. Detailed Example - Specifying versions and options
Following example 
```cmake
# Windows backend(glfw, freeglut)
set(GDM_WINDOW_BACKEND "glfw"  CACHE STRING "" FORCE)
set(GDM_GLFW_VERSION   "3.4.0" CACHE STRING "" FORCE)

# OpenGL function loader(glad, glew)
set(GDM_GL_LOADER      "glad"         CACHE STRING "" FORCE)
set(GDM_GLAD_API       "4.6"          CACHE STRING "" FORCE)
set(GDM_GLAD_PROFILE   "core"         CACHE STRING "" FORCE)
set(GDM_GLAD_EXTENSION "GL_KHR_debug" CACHE STRING "" FORCE)

# OpenGL Mathematics(glm)
set(GDM_USE_GLM      ON     CACHE BOOL "" FORCE)
set(GDM_GLM_VERSION "1.0.3" CACHE STRING "" FORCE)

# GLUTILL
set(GDM_USE_GLUTIL   ON     CACHE BOOL "" FORCE)

# Downloads the dependencies into /external directory
set(GDM_EXTERNAL_DIR "${CMAKE_SOURCE_DIR}/external" CACHE PATH "" FORCE)

# Fetch gdm
include(FetchContent)
FetchContent_Declare(
    gdm
    GIT_REPOSITORY https://github.com/awidesky/gdm.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
    GIT_REMOTE_UPDATE_STRATEGY CHECKOUT
    SOURCE_DIR     "external/gdm"
)
FetchContent_MakeAvailable(gdm)
```

### OpenGL Application Examples

There's a simple OpenGL program [`examples/01_helloWindow.cpp`](examples/01_helloWindow.cpp) that demonstrates:

- Conditional includes via `GDM_HAS_*` macros
- Window creation OpenGL function loading
- GLUtil-aware shader compilation with automatic error reporting
- `GLUtil`'s debug information logging if `GLUtil` is used

Build with: `cmake -B build && cmake --build build`


## Dependency Resolution

Every provider is pinned to one concrete version before anything is downloaded. If you don't set a version explicitly (e.g. `GDM_GLFW_VERSION`), GDM queries the GitHub API for that provider's latest release tag and uses it, so a bare GDM setup always resolves to the newest `glfw`, `glm`, `glew`, etc.

The resolved version is baked into the naming of the downloaded sources: each dependencies live under `${GDM_EXTERNAL_DIR}/<name>-<version>/`, e.g. `external/glfw-3.4.0/`. Since the directory name encodes both the library and its exact version, several versions can sit side by side without interfering, and bumping a version simply downloads into a new directory rather than overwriting the old one.

At configure time GDM resolves each provider in this order:

1. **External sources** — if `${GDM_EXTERNAL_DIR}/<name>-<version>/` already exists and contains a `CMakeLists.txt`, it is used as-is via `add_subdirectory()` and nothing is re-downloaded.
2. **Installed package** — otherwise GDM runs `find_package(<name> CONFIG EXACT <version>)`. If a system-installed library with required version exists, it will be used.
3. **Download from GitHub** — otherwise GDM fetches the release tarball (`<repo>/archive/<tag>.tar.gz`), extracts it into `${GDM_EXTERNAL_DIR}/<name>-<version>/`, and uses it the same way. The raw archive is cached under `<build>/_gdm_downloads/`, so re-configuring after a failed or interrupted extraction reuses the cached tarball instead of re-downloading.

From the consumer's point of view, the source of a dependency (on-disk dir, installed package, or download) doesn't matter: every resolved dependency is exposed through the same namespaced `gdm::*` targets, so your link lines stay identical either way.

### Advantages over plain `FetchContent`

GDM's fetching strategy is essentially "FetchContent, but with a stable, versioned, project-level source cache". Compared to wiring up `FetchContent` yourself for each dependency, GDM gives you:

- **Sources survive outside the build tree** — `FetchContent` keeps sources under `<build>/_deps/`, so deleting or recreating the build directory forces a full re-fetch. GDM keeps them in `external/<name>-<version>/`, which persists across build directories and build configurations (Debug, Release, ...), so a fresh build is configured from already-downloaded sources without touching the network.
- **One download, many consumers** — `GDM_EXTERNAL_DIR` can point to a shared location (e.g. a parent project's `external/`), so several projects or build trees reuse the same sources instead of each re-downloading its own private copy into `_deps/`.
- **Multiple versions coexist** — because the directory name encodes name + version, you can keep e.g. `external/glfw-3.4.0/` and `external/glfw-3.5.0/` side by side and flip a cache variable to switch, without clobbering the other.
- **Manual/offline population** — dropping sources into `external/<name>-<version>/` yourself lets GDM use them directly (its first resolution step), enabling fully offline or air-gapped builds. `FetchContent` always expects to fetch from the network.
- **Installed-package fallback** — GDM tries `find_package` for an exact-version system install before downloading; `FetchContent` never checks for installed packages and always goes to the network.
- **No per-dependency sub-build** — `FetchContent` wraps each dependency in an extra `*-subbuild` superbuild step. GDM adds the dependency directly via `add_subdirectory()`, with its binary dir isolated under `<build>/_gdm_deps/<name>-<version>/`, keeping the build graph simpler.
- **Resumable downloads** — archives are cached under `<build>/_gdm_downloads/` with stable names, so an interrupted extraction doesn't force a re-download on the next configure.

## Generated CMake Targets

GDM provides the following namespaced dependency targets:

- `gdm::window`: selected window backend provider(`glfw`, `freeglut`, ...)
- `gdm::loader`: selected OpenGL function loader target(`glew`, `glad`, ...)
- `gdm::opengl`: system OpenGL (`OpenGL::GL` if possible, else `OpenGL::OpenGL`)
- `gdm::math`: alias of `glm::glm`
- `gdm::defs`: compile-time macros like (`GDM_HAS_*`, `GDM_DEBUG`, `GDM_BUILD_TYPE_*`)
- `gdm::deps`: bundle target that links `gdm::defs`, `gdm::opengl`, `gdm::window`, `gdm::loader`, and `gdm::math`
- `gdm::gdm`: meta target that links `gdm::deps`
- `gdm::glutil`: the [GLUtil](#glutil---opengl-utility-library). Present only when `GDM_USE_GLUTIL=ON`

Using imported target names like `glm::glm` or `glfw::glfw` should work because every imported target's `IMPORTED_GLOBAL` property is
set to `ON`, but using the generated alias targets is more flexible and recommended for portability.

### What `gdm::window` does

`gdm::window` is an alias for the selected window backend(window creation, OpenGL context management, and input processing) provider.

Set `GDM_WINDOW_BACKEND` to `glfw`, `freeglut`, or `none` (default: `glfw`).  
- `glfw` - Use [GLFW](https://www.glfw.org). Set the version with `GDM_GLFW_VERSION` (default: `3.4.0`).  
- `freeglut` - Use [FreeGLUT](https://freeglut.sourceforge.net). Set the version with `GDM_FREEGLUT_VERSION` (default: `3.8.0`).
- `none` - `gdm::window` links to empty interface target. No dependency is used.

### What `gdm::loader` does

`gdm::loader` is an alias for the selected OpenGL function loader.

Set `GDM_GL_LOADER` to `glad`, `glew`, `glew-glad`, or `none` (default: `glad`).  
- `glad` - [GLAD](https://gen.glad.sh). Generates a loader tailored to your API version, profile, and extension list. Configure with these options:

  | Name | Default |
  |------|---------|
  | `GDM_GLAD_API` | `4.6` |
  | `GDM_GLAD_PROFILE` | `core` |
  | `GDM_GLAD_EXTENSION` | `GL_KHR_debug;GL_EXT_texture_compression_s3tc` |

  GDM internally creates two static library targets — `gdm_glad_debug` (built with GLAD's `DEBUG` option) and `gdm_glad_release` (built without).
  `gdm::loader` depends on both, and links `gdm_glad_debug` in `Debug`/`RelWithDebInfo` build, and `gdm_glad_release` otherwise.  
  This means [GLAD callback](https://stackoverflow.com/questions/54476931/how-do-i-use-gladcallback) can be used in debug build(when `GDM_DEBUG` and `GLAD_OPTION_GL_DEBUG` macro is defined), and release builds will have no debug callback overhead([example](./external/glutil/example/stacktrace.cpp)).  
  Note that GLAD callback is not [OpenGL debug message callback](https://wikis.khronos.org/opengl/Debug_Output)!
- `glew` - [GLEW](https://glew.sourceforge.net). Set the version with `GDM_GLEW_VERSION` (default: `2.3.1`). Linkage mode controlled by `GDM_GLEW_STATIC` (default: `ON`).  
- `glew-glad` - GLEW API backed by GLAD function loading. This is for legacy projects that already use GLEW, but also need GLUtil's debug features, which require GLAD support. In this mode, GDM generates a small wrapper that keeps GLEW-style headers and calls intact, strips GLEW's direct function declarations from the public header, and redirects loading to GLAD instead. That lets existing GLEW code keep compiling while GLUtil can use GLAD's debug callback path. Example usage: [https://github.com/awidesky/ogl-gdm](https://github.com/awidesky/ogl-gdm). See [glew-glad Hybrid Mode](#glew-glad-hybrid-mode) for details.
- `none` - `gdm::loader` links to empty interface target. No dependency is used.

### What `gdm::opengl` does

`gdm::opengl` provides the system OpenGL library linkage. It resolves to `OpenGL::GL` on desktop platforms (macOS, Linux, Windows) or falls back to `OpenGL::OpenGL` when the modern target is unavailable.

No configuration variables control this target directly - it is always created and always resolves to whatever `find_package(OpenGL)` discovers on the target system.

### What `gdm::math` does

`gdm::math` is an alias of `glm::glm` - the [GLM](https://github.com/g-truc/glm) header-only math library.

Controlled by `GDM_USE_GLM` (default: `ON`). Set the version with `GDM_GLM_VERSION` (default: `1.0.3`). When `GDM_USE_GLM=OFF`, `gdm::math` links to empty interface target.

### What `gdm::defs` does

`gdm::defs` is a compile-time-definitions-only interface target. It defines macros about used dependencies configurations and build types.

Always linked by `gdm::deps`. The exact set of macros depends on your configuration:

- `GDM_HAS_GLFW` / `GDM_HAS_FREEGLUT` / `GDM_HAS_WINDOW_NONE`
  * Exactly one of these is defined according to `GDM_WINDOW_BACKEND`
- `GDM_HAS_GLAD` / `GDM_HAS_GLEW` / `GDM_HAS_GLEW_GLAD` / `GDM_HAS_LOADER_NONE`
  * These are defined according to `GDM_GL_LOADER`. When `glew-glad` is selected, all of `GDM_HAS_GLAD`, `GDM_HAS_GLEW`, and `GDM_HAS_GLEW_GLAD` are defined.
- `GDM_HAS_OPENGL`
  * Always defined when the project configures successfully; CMake will fail if no usable system OpenGL target is found.
- `GDM_HAS_GLM`
  * Defined when `GDM_USE_GLM=ON`
- `GDM_HAS_GLUTIL`
  * Defined when `GDM_USE_GLUTIL=ON`
- `GDM_BUILD_TYPE_DEBUG` / `GDM_BUILD_TYPE_RELEASE` / `GDM_BUILD_TYPE_RELWITHDEBINFO` / `GDM_BUILD_TYPE_MINSIZEREL`
  * Exactly one of these is defined depending on the active CMake configuration.
- `GDM_DEBUG`
  * Always defined. Value is `1` for `Debug` and `RelWithDebInfo` build, and `0` otherwise. Used by [GLUtil](#glutil---opengl-utility-library)'s debug functionality.

### What `gdm::deps` does

`gdm::deps` bundles all selected GDM-managed dependencies into a single interface target, so you only need one `target_link_libraries(my_app PRIVATE gdm::deps)`.

It links `gdm::defs`, `gdm::opengl`, `gdm::window`, `gdm::loader`, and `gdm::math`.

### What `gdm::gdm` does

`gdm::gdm` is the main target, currently same as `gdm::deps`. Which one to use is purely a naming preference.

### What `gdm::glutil` does

`gdm::glutil` is the [GLUtil](#glutil---opengl-utility-library) library - Easy loading for shader, texture, model resources, plus logging, file-path resolution, GL and string helpers, and a comprehensive debug system with error reporting, GL object tracking, labeling, stack traces, runtime info, GL state snapshots, and leak detection. Detailed explanation and usage at [GLUtil - OpenGL Utility Library](#glutil---opengl-utility-library)

Present only when `GDM_USE_GLUTIL=ON`. Defaults to `ON` when GDM is the main project, and `OFF` when GDM is consumed as a subdirectory — set `GDM_USE_GLUTIL=ON` to enable it. This target is **not** included in `gdm::deps` - link it explicitly when you need GLUtil features:

```cmake
target_link_libraries(my_app PRIVATE gdm::deps gdm::glutil)
```

## How To Use

### 1) Add GDM as a subdirectory

```cmake
set(GDM_WINDOW_BACKEND glfw CACHE STRING "")
set(GDM_GL_LOADER     glad CACHE STRING "")
set(GDM_USE_GLM       ON   CACHE BOOL   "")
set(GDM_USE_GLUTIL    OFF CACHE BOOL   "")

add_subdirectory(external/gdm)

add_executable(my_app src/main.cpp)
target_link_libraries(my_app PRIVATE gdm::deps)
```

### Linking selectively

```cmake
# Window backend only
target_link_libraries(my_tool PRIVATE gdm::window)

# Full stack (equivalent to gdm::deps)
target_link_libraries(my_app PRIVATE gdm::gdm)
```

## CMake Targets

| Target | Description |
|--------|-------------|
| `gdm::window` | Selected window backend (GLFW / FreeGLUT / none) |
| `gdm::loader` | Selected OpenGL loader (GLAD / GLEW / glew-glad / none) |
| `gdm::opengl` | System OpenGL bridge (`OpenGL::GL` or `OpenGL::OpenGL`) |
| `gdm::math` | GLM alias target (only when `GDM_USE_GLM=ON`) |
| `gdm::defs` | Compile-time feature macros |
| `gdm::deps` | **Recommended consumer target** - bundles defs + opengl + window + loader + math |
| `gdm::gdm` | Meta alias for `gdm::deps` |
| `gdm::glutil` | GLUtil library (only when `GDM_USE_GLUTIL=ON`) |

Every imported target's `IMPORTED_GLOBAL` is set to `TRUE`, so original target names like `glfw::glfw` or `glm::glm` are also visible. Using the `gdm::*` aliases is recommended for portability.

### Feature macros (`gdm::defs`)

Link `gdm::defs` (or any target that depends on it) to get these compile definitions:

```cpp
// Window backend
#ifdef GDM_HAS_GLFW       // GLFW selected
#ifdef GDM_HAS_FREEGLUT   // FreeGLUT selected

// OpenGL loader
#ifdef GDM_HAS_GLAD       // GLAD selected
#ifdef GDM_HAS_GLEW       // GLEW or glew-glad selected
#ifdef GDM_HAS_GLEW_GLAD  // glew-glad hybrid mode

// Optional modules
#ifdef GDM_HAS_GLM        // GLM enabled
#ifdef GDM_HAS_GLUTIL     // GLUtil enabled

// Build type
#if GDM_DEBUG             // Debug or RelWithDebInfo configuration
#ifdef GDM_BUILD_TYPE_DEBUG
#ifdef GDM_BUILD_TYPE_RELEASE
```

Example usage:

```cpp
#ifdef GDM_HAS_GLAD
#include <glad/gl.h>
#endif
#ifdef GDM_HAS_GLFW
#include <GLFW/glfw3.h>
#elif defined(GDM_HAS_FREEGLUT)
#include <GL/freeglut.h>
#endif
```

`GDM_EXTERNAL_DIR` is set to `${CMAKE_SOURCE_DIR}/external`

## Configuration Options

| Option | Values | Default | Description |
|--------|--------|---------|-------------|
| `GDM_WINDOW_BACKEND` | `none`, `glfw`, `freeglut` | `glfw` | Window system backend |
| `GDM_GL_LOADER` | `none`, `glad`, `glew`, `glew-glad` | `glad` | OpenGL loader |
| `GDM_USE_GLM` | `ON`, `OFF` | `ON` | Enable GLM math library |
| `GDM_USE_GLUTIL` | `ON`, `OFF` | `ON` | Enable GLUtil utility library |
| `GDM_BUILD_EXAMPLES` | `ON`, `OFF` | `ON` (standalone) | Build examples |
| `GDM_BUILD_TESTS` | `ON`, `OFF` | `ON` (standalone) | Build GLUtil tests |
| `GDM_EXTERNAL_DIR` | path | `${CMAKE_SOURCE_DIR}/external` | External dependency directory |
| `GDM_TLS_VERIFY` | `ON`, `OFF` | `ON` | TLS verification for downloads (overrided by `CMAKE_TLS_VERIFY` if it's defined) |
| `GDM_FORCE_GEN_GLAD` | `ON`, `OFF` | `OFF` | Force re-generation of GLAD sources even if cached (useful when `gen.glad.sh` API changes) |
| `GLUTIL_DISABLE_LOG_ON_RELEASE` | `ON`, `OFF` | `OFF` (ON as subproject) | Suppress GLUtil logging in non-debug builds |

### Per-provider version options

These are created on-demand based on your provider selections:

| Option | Applies when | Default |
|--------|-------------|---------|
| `GDM_GLFW_VERSION` | `GDM_WINDOW_BACKEND=glfw` | `3.4.0` |
| `GDM_FREEGLUT_VERSION` | `GDM_WINDOW_BACKEND=freeglut` | `3.8.0` |
| `GDM_GLM_VERSION` | `GDM_USE_GLM=ON` | `1.0.3` |
| `GDM_GLAD_API` | `GDM_GL_LOADER=glad` | `4.6` |
| `GDM_GLAD_PROFILE` | `GDM_GL_LOADER=glad` | `core` |
| `GDM_GLAD_EXTENSION` | `GDM_GL_LOADER=glad` | `GL_KHR_debug;GL_EXT_texture_compression_s3tc` |
| `GDM_GLEW_VERSION` | `GDM_GL_LOADER=glew` or `glew-glad` | `2.3.1` |
| `GDM_GLEW_STATIC` | `GDM_GL_LOADER=glew` | `ON` |

# GLUtil - OpenGL Utility Library
[![C++](https://img.shields.io/badge/C%2B%2B-17-blue)](CMakeLists.txt)

GLUtil is a built-in utility library shipped under `external/glutil/`. It provides shader/texture/model wrappers, loaders, and a comprehensive debug system for OpenGL applications.

## Core Modules

| Module | Header | Description | Example |
|--------|--------|-------------|---------|
| **Shader** | [glutil/shader.hpp](external/glutil/include/glutil/shader.hpp) | File loading with BOM detection (UTF-8/16/32), compilation, linking. RAII `GLShader`/`GLProgram` wrappers. | [Shader example](#shader-example) |
| **Inspector** | [glutil/inspector.hpp](external/glutil/include/glutil/inspector.hpp) | Query shader compile and program link results with structured `InspectResult`. | [Shader example](#shader-example)(it also has `Inspecter` usage) |
| **Model** | [glutil/model.hpp](external/glutil/include/glutil/model.hpp) | OBJ loading via `tiny_obj_loader` -> CPU `ModelData` or GPU `GLModelData` (VAO/VBO/EBO). Optional vertex deduplication. | [model.cpp](external/glutil/example/model.cpp) |
| **Texture** | [glutil/texture.hpp](external/glutil/include/glutil/texture.hpp) | Image loading via `stb_image` (common formats) + DDS/KTX compressed textures. CPU `TextureImage`/`TextureDDS` and GPU `GLTexture2D`. | [texture.cpp](external/glutil/example/texture.cpp) |
| **Path** | [glutil/path.hpp](external/glutil/include/glutil/path.hpp) | File path resolution for relative paths. Search the file in working dir -> executable dir -> project root dir. | [pathResolveTest.cpp](external/glutil/test/pathResolveTest.cpp) (test) |
| **Logging** | [glutil/logging.hpp](external/glutil/include/glutil/logging.hpp) | Stream-style logger with `[INFO]` / `[WARNING]` / `[ERROR]` severity levels and stdout/stderr sinks. | [Shader example](#shader-example)(it also has logging usage) |
| **Math** | [glutil/math.hpp](external/glutil/include/glutil/math.hpp) | Predefined vertex types (`VertexP`, `VertexPC`, `VertexPT`, `VertexPNT`, `VertexPNCT`) with compile-time validation, GLM accessors, and attribute extractor. | [manyCubes.cpp](external/glutil/example/manyCubes.cpp), [Vertex attribute extracting](https://github.com/awidesky/gdm/blob/master/external/glutil/example/manyCubes.cpp#L780) |


GLUtil examples are built when `GDM_GL_LOADER=glad`, `GDM_WINDOW_BACKEND=glfw` and `GDM_USE_GLM=ON` - see `external/glutil/example/` for examples covering textures/model/shader loading, auto debug labeling, stacktraces, snapshots, tracking, etc.


### Shader example

Example usage of [glutil/shader.hpp](external/glutil/include/glutil/shader.hpp) API (plus [glutil/inspector.hpp](external/glutil/include/glutil/inspector.hpp)).

```cpp
#include <glutil/shader.hpp>
#include <glutil/inspector.hpp>
#include <glutil/glToString.hpp>
#include <glutil/logging.hpp>

// 1. Load raw source from disk.
// After reading the content, it's checked if there's any Non-ASCII characters.
// - UTF-8 BOM: replaced with spaces. If the GLSL version is >= 4.2 (isGLSLSupportUTF8()),
//   non-ASCII characters are allowed inside comments, so they are kept as-is.
//   Otherwise (ASCII-only GLSL), all non-ASCII bytes are replaced with spaces.
// - UTF-16 / UTF-32 (with BOM): converted to ASCII.
// - Unknown charset with non-ASCII bytes: replaced with spaces
//   (unless ShaderLoader::replaceUnknownNonASCII is set to false).
glutil::ShaderLoadResult src = glutil::ShaderLoader::loadFile("shaders/triangle.frag");
if (!src.ok) {
    LOG_ERROR() << "load failed: " << src.error;
    // glutil's logging object stores messages into internal buffer, and flushes into output stream
    // with prefix and line terminator when object is destroyed.
    return 1;
}

// 2. Compile manually using raw GL functions
GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
glShaderSource(frag, 1, src.string(), src.lengthPtr());
glCompileShader(frag);

// 3. Inspect the fragment shader's compile status
const glutil::InspectResult compileResult = glutil::Inspector::shaderCompileResult(frag);
LOG_INFO() << "compile ok: " << compileResult.ok << '\n' << compileResult.message;
glDeleteShader(frag); // manual GL object: clean up explicitly

// 4. Compile a shader from file (loadFile + glCreateShader + glShaderSource + glCompileShader)
glutil::GLShader vert = glutil::ShaderLoader::loadShaderToGL(GL_VERTEX_SHADER, "shaders/triangle.vert");
if (!vert.ok) {
    LOG_ERROR() << "compile failed: " << vert.error;
    return 1;
}
// vert.id is the GL shader handle (glCreateShader result), vert.type is the stage enum.
LOG_INFO() << "vert id=" << vert.id << " type=" << glutil::glShaderTypeToString(vert.type);
// Expected output : vert id=1 type=GL_VERTEX_SHADER (vs)

// 5. Link a program from two files (loadShaderToGL x2 + link + Inspector validation)
glutil::GLProgram program = glutil::ShaderLoader::loadProgramToGL("shaders/triangle.vert", "shaders/triangle.frag");
if (!program.ok) {
    LOG_ERROR() << "link failed: " << program.error;
    return 1;
}

// 6. Inspect a raw program's link status
const glutil::InspectResult linkResult = glutil::Inspector::programLinkResult(program.id);
LOG_INFO() << "link ok: " << linkResult.ok << '\n' << linkResult.message;

// 7. Encoding helpers
if (glutil::isGLSLSupportUTF8())
    LOG_INFO() << "GLSL >= 4.2: UTF-8 source allowed";

char bytes[] = "abc\x80xyz";
if (glutil::hasNonASCII(bytes, sizeof(bytes) - 1))
    glutil::replaceNonASCIIWithSpace(bytes, sizeof(bytes) - 1);

// 8. Toggle encoding behavior
// run BOM detection / charset conversion when loading (default true).
glutil::ShaderLoader::checkEncoding = true;
// when true, unknown non-ASCII bytes are replaced with spaces, set false to keep bytes as-is.
// (contents may corrupt when encoding is Shift-JIS/GBK/Big5/UTF-16/32-without-BOM);
glutil::ShaderLoader::replaceUnknownNonASCII = true;

// GLShader / GLProgram free their GL handles automatically on scope exit.
```

## Debug System Overview

The philosophy of GLUtil's debug system is mostly 'behind the scene' work that give handful of useful information when an OpenGL error has occured, 
and some help functions that give system/OpenGL informations.

| Component | Description |
|-----------|-------------|
| `glutil::debug::init()` | Install `GL_KHR_debug` callback, set up GL object tracking, print GL runtime info |
| `glutil::debug::labelGLobject()` | Label GL objects (requires `GL_KHR_debug` or GL 4.3+) |
| `glutil::debug::getGLobjectLabel()` | Retrieve GL object label |
| `glutil::debug::isGL_KHR_debugSupported()` | Runtime check for `GL_KHR_debug` availability |
| `glutil::debug::printRuntimeInfo()` | Print GL version, vendor, renderer, extensions, and capability limits |
| `glutil::debug::printGpuMemoryInfo()` | Print GPU memory usage (vendor-specific extensions) |
| `glutil::debug::hasGLExtension()` / `getGLExtensions()` | Check for / list supported GL extensions |
| `glutil::debug::availableGLversion()` | Find highest available OpenGL version by creating test contexts |
| `glutil::debug::GLStateTracker` | Global singleton tracking GL object creation/destruction. Reports leaked objects on destruction. |
| `glutil::debug::Snapshot` | Full GL state capture: framebuffers, shader programs + uniforms, textures, VAO/VBO/EBO layout, renderer state, binding state. Supports async output to stream or file. |
| `glutil::debug::disableAutoLabelGLObjects` | Global toggle for automatic GL object labeling (default: `false`, labeling enabled) |
| `glutil::debug::disableAutoInspector` | Global toggle for automatic inspection hooks (default: `false`) |


### Automatic debug initialization

When using GLAD with `GDM_DEBUG=1`, GLUtil provides `#define` overrides that hijack `gladLoadGL` / `gladLoadGLUserPtr` / `gladLoaderLoadGL` to automatically call `glutil::debug::init()` after a successful load - no manual init required. This is designed to drop into legacy codebases transparently.

1. Init debug system manually
```cpp
// 1. Manual init: include glfw + glad headers, init OpenGL, then call glutil::debug::init() yourself.
#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <glutil/debug.hpp>

glfwInit();
GLFWwindow* window = glfwCreateWindow(800, 600, "manual debug init", nullptr, nullptr);
glfwMakeContextCurrent(window);
if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
    return -1; // glad load failed
}
glutil::debug::init(); // manual debug init

// The debug callback catches the GL error and prints a stack trace automatically.
glBindTexture(GL_TEXTURE_2D, 99999);
```

2. Init debug system automatically, by including `glutil/gl.hpp`
In [`glutil/gl.hpp`](https://github.com/awidesky/gdm/blob/master/external/glutil/include/glutil/gl.hpp#L16), the `gladLoadGL` function is overloaded so that it'll call `glutil::debug::init()` after the GL functions are loaded successfully.  
Also, `glutil/gl.hpp` includes all headers of selected [Window backend and OpenGL function loader](#supported-opengl-libraries) in correct order.
  
```cpp
// 2. Automatic init: include glutil/gl.hpp only. It pulls in the glfw + glad headers,
//    and hijacks gladLoadGL() to call glutil::debug::init() in debug build.
#include <glutil/gl.hpp>

glfwInit();
GLFWwindow* window = glfwCreateWindow(800, 600, "auto debug init", nullptr, nullptr);
glfwMakeContextCurrent(window);
if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) { // actually calls glutil_gladLoadGL()
    return -1; // glad load failed
}
// glutil::debug::init() was already called by the hijacked gladLoadGL.

// Same erroneous call, same output as above (debug is already active).
glBindTexture(GL_TEXTURE_2D, 99999);
```

<details>
<summary>Expected output</summary>

<pre>
[ERROR] OpenGL debug message callback invoked!
---------------------gldebugCallback-start----------------
Message: Error has been generated. GL error GL_INVALID_OPERATION in (null): (ID: 173538523) Generic error
ID: 1282
Source: API
Type: ERROR
Severity: HIGH
---------------------gldebugCallback-end------------------
[ERROR] [GL Error] GL_INVALID_OPERATION(1282)
In function glBindTexture
#0 0x00007ff6c51b1a56 in main at stacktrace.cpp:55:18
      53:     glBindTexture(GL_TEXTURE_2D, tex);
      54:
    &gt; 55:     glBindTexture(GL_TEXTURE_2D, 99999); // should generate an error
              ^
      56:     
#1 0x00007ff6c51b13b9 at glutil_stacktraceExample.exe
#2 0x00007ff6c51b107f at glutil_stacktraceExample.exe
#3 0x00007ff8ad1d4033 in BaseThreadInitThunk at KERNEL32.DLL
#4 0x00007ff8ad633690 in RtlUserThreadStart at ntdll.dll

[ERROR] [ErrorSnapshot] You tried to bind texture #99999, Label : (none) to target GL_TEXTURE_2D
[ERROR] [ErrorSnapshot] Check following snapshot to see loaded/bound texture(s).

========================================================
               glutil::debug::Snapshot
========================================================

============================== Texture Info ==============================

  Unit  0  [ 2D ]  ID : 1, Label : "Tex2D#1(stacktrace.cpp:53 -&gt; glBindTexture(GL_TEXTURE_2D, tex);)"
                             Size=0x0  Format=GL_RGBA

[TIMER] TextureInfo: 0.078004 ms

============================== Bound Info ==============================

  GL_VERTEX_ARRAY_BINDING             : 0
  GL_ARRAY_BUFFER_BINDING             : 0
  GL_ELEMENT_ARRAY_BUFFER_BINDING     : 0
  GL_UNIFORM_BUFFER_BINDING           : 0
  GL_SHADER_STORAGE_BUFFER_BINDING    : 0
  GL_PIXEL_PACK_BUFFER_BINDING        : 0
  GL_PIXEL_UNPACK_BUFFER_BINDING      : 0
  GL_TEXTURE_BINDING_2D               : 1, Label : "Tex2D#1(stacktrace.cpp:53 -&gt; glBindTexture(GL_TEXTURE_2D, tex);)"
  GL_SAMPLER_BINDING                  : 0
  GL_CURRENT_PROGRAM                  : 0
  GL_RENDERBUFFER_BINDING             : 0
  GL_FRAMEBUFFER_BINDING              : 0

[TIMER] BoundInfo: 0.069108 ms


[TIMER] Entire Snapshot took: 0.160798 ms
========================================================
                     Snapshot end
========================================================

[ERROR] ---- End of "GL_INVALID_OPERATION(1282) in function glBindTexture"

[INFO] === OpenGL Object Leak Check ===
[ERROR] [LEAK] Object id=1 type=GL_TEXTURE label=Tex2D#1(stacktrace.cpp:53 -&gt; glBindTexture(GL_TEXTURE_2D, tex);)
</pre>
</details>


### GL object leak detection

`GLStateTracker` singleton object tracks OpenGL objects. In it's destructor it logs all unreleased GL objects(buffers, textures, programs, etc.) with their debug labels, making resource leak detection trivial in debug builds.
  
When the program is being terminated, Info of unreleased GL objects will be logged. You can get the `GLStateTracker` object by `glutil::debug::GLStateTracker::instance()` and see information of currently tracked objects.

See example: [example/tracker.cpp](./external/glutil/example/tracker.cpp)

### Automatic GL object labeling

The debug callback automatically labels every GL object (buffers, textures, VAOs, shaders, programs, etc.) right after it is created, using `glObjectLabel` (requires `GL_KHR_debug` or GL 4.3+). Labels include the object type, id, and the line of code that created it, e.g. `VBO#3(glBufferData)` or `Shader#2(glCreateShader)`. Tex2D#1(stacktrace.cpp:53 -> glBindTexture(GL_TEXTURE_2D, tex);)

If you want auto-labeling turned off, use the global variable `glutil::debug::disableAutoLabelGLObjects = true;`.

See example: [example/autoLabeling.cpp](./external/glutil/example/autoLabeling.cpp)

### GL Error logging with stack trace

When a GL call fails, the debug callback logs the error information and a C++ stack trace (via [`cpptrace`](https://github.com/jeremy-rifkin/cpptrace)), so you can see exactly which call in your code triggered it:

```
[ERROR] [GL Error] GL_INVALID_OPERATION(1282)
In function glBindTexture
#0 0x00000001022622ff in main at stacktrace.cpp:57:5
      55: #endif
      56: 
    > 57:     glBindTexture(GL_TEXTURE_2D, 99999);
              ^
      58: 
      59: // check if GLAD debug callback is enabled by GLAD_OPTION_GL_DEBUG macro.
#1 0x00000001948d2b97 in start + 6075 at dyld
```

When an erroneous GL function call is invoked inside of rendering loop, the error log will printed every frame, spamming the console.
To avoid this, repeated identical errors are aggregated: only the first occurrence of a burst is reported with a full stack trace. Further identical errors within the burst window (default 3 seconds) only bump a counter, and are summarized as `occurred N times in S seconds` when the burst is flushed.
```
[ERROR] [GL Error] GL_INVALID_OPERATION(1282)
In function glDrawArrays
#0 0x000000010256636f in main at stacktrace.cpp:65:9
      64:         glBindVertexArray(0);
    > 65:         glDrawArrays(GL_TRIANGLES, 0, 3);
                  ^
      66: 
      67:         glfwSwapBuffers(window);
#1 0x00000001948d2b97 in start + 6075 at dyld
[ERROR] ---- End of "GL_INVALID_OPERATION(1282) in function glDrawArrays"

[ERROR] [GL Error] GL_INVALID_OPERATION(1282) occurred 235 times in 3 seconds.
In function glDrawArrays
#0 0x000000010256636f in main at stacktrace.cpp:65:9
      64:         glBindVertexArray(0);
    > 65:         glDrawArrays(GL_TRIANGLES, 0, 3);
                  ^
      66: 
      67:         glfwSwapBuffers(window);
#1 0x00000001948d2b97 in start + 6075 at dyld
[ERROR] ---- End of "GL_INVALID_OPERATION(1282) in function glDrawArrays"
```

See example: [example/stacktrace.cpp](./external/glutil/example/stacktrace.cpp)

### Debug info functions

`glutil::debug` provides helpers to query and print OpenGL runtime information:

- `printRuntimeInfo(verbose, os)` — print GL version, vendor, renderer, and (optionally) extended capability limits.
- `printGpuMemoryInfo(os)` — GPU memory usage(it uses naive vendor-specific extensions, providing just little available informations).
- `hasGLExtension(name)` / `getGLExtensions()` — check for / list supported extensions.
- `availableGLversion()` : Find highest avilable OpenGL version by creating contexts with selected [window backend provider](#supported-opengl-libraries). The return type is `GLVersion`; it's safely comparable against version strings (e.g. `if (version >= "4.2")`).

See example: [example/debugLabel.cpp](./external/glutil/example/debugLabel.cpp)

### GL state snapshots

`Snapshot` captures the full current GL state in one shot: framebuffer status, shader programs + active uniform values, texture bindings (per active texture unit), VAO/VBO/EBO layouts (+ raw data), renderer pipeline state, and the set of currently bound objects.

```cpp
glutil::debug::Snapshot snap(true); // enable all categories
snap.shaderUniform(true)            // dump active uniforms
    .bufferVAOInfo(true, true)      // dump VAO/VBO layout + raw data
    .rendererState(true)            // dump pipeline state
    .capture(std::cerr);            // output to stderr (synchronous)
// or capture to directory:
snap.capture("/tmp/gl_snapshot", /*dumpVertexData=*/true);
```

A `Snapshot` created with default options captures **only once**: after the first `capture()`, subsequent calls return early. This prevents the (large) dump from flooding the log when it is invoked every frame or on every error — the same anti-spam idea used for error reports.

`capture()` also supports **async** output: the snapshot is written by a background worker thread through an internal queue, so the render loop is not blocked. Grab the returned `SnapshotAsyncHandle` and call `wait()` on it if you need to be sure the output is complete before continuing.

See example: [example/snapshot.cpp](./external/glutil/example/snapshot.cpp)

## glew-glad Hybrid Mode

When your original codebase uses `GLEW`, but you need `GLUtil`'s debug feature without minimal code change,

Set `GDM_GL_LOADER=glew-glad` to get the **GLEW API** backed by **GLAD function loading**:

- Programs written against GLEW don't need include changes
- GLAD provides GL function post/precallback support, making GLUtil's debug features available
- At CMake time, a fake `glew.h` is generated that strips GLEW's function pointer declarations and injects GLAD's headers instead
- This enables GLEW-based codebase to use debug `GLUtil`'s debug feature without changing the existing code.

```cmake
set(GDM_GL_LOADER glew-glad CACHE STRING "")
```

One of the actual use cases of `glew-glad` mode is [ogl-gdm](http://github.com/awidesky/ogl-gdm/tree/test) - GDM port of [opengl-tutorial]().

---

## Project Structure

```
gdm/
├── CMakeLists.txt              # Main build (targets, options, provider logic)
├── cmake/
│   ├── Dependencies.cmake      # use_or_fetch_package() - find or download dependencies
│   └── FetchGlad.cmake         # GLAD code generation via gen.glad.sh(curl is required!)
├── src/
│   └── glewToGlad.cpp          # glew-glad bridge implementation
├── external/
│   └── glutil/                 # Built-in GLUtil library
│       ├── CMakeLists.txt       # GLUtil build + tests + examples
│       ├── include/glutil/      # Public headers
│       ├── src/                 # Implementation + stb_image + tiny_obj_loader + dds-ktx
│       ├── example/             # Examples (texture, model loading, debug, etc.)
│       └── test/                # Unit tests (shader encoding test, benchmark, path resolve test)
├── examples/
│   └── 01_helloWindow.cpp      # Simple example: triangle with window backend + loader + system information
├── LICENSE                     # MIT
└── README.md                   # What your're seeing right now :D
```

## License

MIT - see [LICENSE](LICENSE).
