# GDM - OpenGL Dependency Manager

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![CMake](https://img.shields.io/badge/CMake-%3E%3D3.24-blue)](CMakeLists.txt)
[![C++](https://img.shields.io/badge/C%2B%2B-17-blue)](CMakeLists.txt)

**GDM** is a CMake-first OpenGL dependency manager for OpenGL projects. Specify your OpenGL library once, and GDM finds installed packages, downloads sources if not exist, and provides clean namespaced targets so your code works everywhere without changing includes or link lines.

## Supported OpenGL Libraries

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
    GIT_TAG        master
    GIT_SHALLOW    TRUE
    GIT_REMOTE_UPDATE_STRATEGY CHECKOUT
    SOURCE_DIR     "external/gdm"
)
set(GDM_WINDOW_BACKEND glfw CACHE STRING "" FORCE)
set(GDM_GL_LOADER     glad CACHE STRING "" FORCE)
set(GDM_USE_GLM       ON   CACHE BOOL   "" FORCE)
set(GDM_USE_GLUTIL    ON   CACHE BOOL   "" FORCE)
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
| `GDM_TLS_VERIFY` | `ON`, `OFF` | `ON` | TLS verification for downloads |
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
| `GDM_GLAD_EXTENSION` | `GDM_GL_LOADER=glad` | `GL_ARB_debug_output;GL_EXT_debug_label;...` |
| `GDM_GLEW_VERSION` | `GDM_GL_LOADER=glew` or `glew-glad` | `2.3.1` |
| `GDM_GLEW_STATIC` | `GDM_GL_LOADER=glew` | `ON` |

## GLUtil - OpenGL Utility Library

GLUtil is a built-in utility library shipped under `external/glutil/`. It provides RAII wrappers, loaders, and a comprehensive debug system for OpenGL applications.

### Core Modules

| Module | Header | Description |
|--------|--------|-------------|
| **Shader** | `glutil/shader.hpp` | File loading with BOM detection (UTF-8/16/32), compilation, linking. RAII `GLShader`/`GLProgram` wrappers. |
| **Inspector** | `glutil/inspector.hpp` | Query shader compile and program link results with structured `InspectResult`. |
| **Model** | `glutil/model.hpp` | OBJ loading via `tiny_obj_loader` -> CPU `ModelData` or GPU `GLModelData` (VAO/VBO/EBO). Optional vertex deduplication. |
| **Texture** | `glutil/texture.hpp` | Image loading via `stb_image` (common formats) + DDS/KTX compressed textures. CPU `TextureImage`/`TextureDDS` and GPU `GLTexture2D`. |
| **Path** | `glutil/path.hpp` | Multi-strategy file resolution (cwd -> executable dir -> project root). |
| **Logging** | `glutil/logging.hpp` | Stream-style logger with `[INFO]` / `[WARNING]` / `[ERROR]` severity levels and stdout/stderr sinks. |
| **Math** | `glutil/math.hpp` | Predefined vertex types (`VertexP`, `VertexPC`, `VertexPT`, `VertexPNT`, `VertexPNCT`) with compile-time validation and GLM accessors. |

### Debug System (GDM_DEBUG=1 only)

| Component | Description |
|-----------|-------------|
| `glutil::debug::init()` | Install `GL_KHR_debug` callback, set up GL object tracking, print GL runtime info |
| `glutil::debug::labelGLobject()` | Label GL objects (requires `KHR_debug` support) |
| `glutil::debug::getGLobjectLabel()` | Retrieve GL object label |
| `glutil::debug::isGL_KHR_debugSupported()` | Runtime check for `KHR_debug` availability |
| `glutil::debug::printRuntimeInfo()` | Print GL version, vendor, renderer, extensions |
| `GLStateTracker` | Global singleton tracking GL object creation/destruction. Reports leaked objects on destruction. |
| `Snapshot` | Full GL state capture: framebuffers, shader programs + uniforms, textures, VAO/VBO/EBO layout, renderer state, binding state. Supports async output to stream or file. |
| `debug::disableAutoLabelGLObjects` | Global toggle for automatic GL object labeling |
| `debug::disableAutoInspcector` | Global toggle for automatic inspection hooks |

#### Automatic debug initialization

When using GLAD with `GDM_DEBUG=1`, GLUtil provides `#define` overrides that hijack `gladLoadGL` / `gladLoadGLUserPtr` / `gladLoaderLoadGL` to automatically call `glutil::debug::init()` after a successful load - no manual init required. This is designed to drop into legacy codebases transparently.

#### GL object leak detection

`GLStateTracker` destructor automatically logs all unreleased GL objects (buffers, textures, programs, etc.) with their debug labels, making resource leak detection trivial in debug builds.

#### GL state snapshots

```cpp
glutil::debug::Snapshot snap(true); // enable all categories
snap.shaderUniform(true)            // dump active uniforms
    .bufferVAOInfo(true, true)      // dump VAO/VBO layout + raw data
    .rendererState(true)            // dump pipeline state
    .capture(std::cerr);            // output to stderr (synchronous)
// or capture to directory:
snap.capture("/tmp/gl_snapshot", /*dumpVertexData=*/true);
```

## glew-glad Hybrid Mode

Set `GDM_GL_LOADER=glew-glad` to get the **GLEW API** backed by **GLAD function loading**:

- Programs written against GLEW don't need include changes
- GLAD provides KHR_debug support, letting you use GLUtil's debug features
- At CMake time, a fake `glew.h` is generated that strips GLEW's function pointer declarations and injects GLAD's headers instead
- This enables GLEW-style code to work in GL 4.6 core profile contexts

```cmake
set(GDM_GL_LOADER glew-glad CACHE STRING "")
```

## Project Structure

```
gdm/
├── CMakeLists.txt              # Main build (targets, options, provider logic)
├── cmake/
│   ├── Dependencies.cmake      # use_or_fetch_package() - find or download dependencies
│   └── FetchGlad.cmake         # GLAD code generation via gen.glad.sh
├── src/
│   └── glewToGlad.cpp          # glew-glad bridge implementation
├── external/
│   ├── glutil/                 # Built-in utility library
│   │   ├── CMakeLists.txt       # GLUtil build + tests + examples
│   │   ├── include/glutil/      # 16 public headers
│   │   ├── src/                 # Implementation + stb_image + tiny_obj_loader + dds-ktx
│   │   ├── example/             # 9 GLUtil examples (texture, model, debug, etc.)
│   │   └── test/                # Unit tests
│   └── ...                      # Fetched dependencies (glfw, glm, glad, glew-cmake, cpptrace)
├── examples/
│   └── 01_helloWindow.cpp      # Working example: triangle with window backend + loader
├── LICENSE                     # MIT
└── README.md
```

## Examples

The example at `examples/01_helloWindow.cpp` demonstrates:

- Conditional includes via `GDM_HAS_*` macros
- Window creation with GLFW or FreeGLUT (compile-time switch)
- GLAD loading via `glfwGetProcAddress` or `glutGetProcAddress`
- GLUTIL-aware shader compilation with automatic error reporting
- Basic triangle rendering with VAO/VBO and shader program

Build with: `cmake -B build && cmake --build build`

GLUtil examples are built when `GDM_GL_LOADER=glad` and `GDM_WINDOW_BACKEND=glfw` and `GDM_USE_GLM=ON` - see `external/glutil/example/` for 9 additional examples covering textures, models, normal mapping, debug labeling, auto-labeling, stacktraces, snapshots, and tracking.

## Building & Testing

```sh
# Standalone build
cmake -B build -DGDM_BUILD_TESTS=ON -DGDM_BUILD_EXAMPLES=ON
cmake --build build

# Run tests
ctest --test-dir build
```

When GDM is included as a subdirectory in another project, `GDM_BUILD_EXAMPLES` and `GDM_BUILD_TESTS` default to `OFF`.

## CMake Modules

### `use_or_fetch_package()`

Defined in [`cmake/Dependencies.cmake`](cmake/Dependencies.cmake). Resolution order:

1. Check `external/<name>-<version>/` for pre-populated sources
2. Search for an installed `Config` package via `find_package`
3. Download tarball from GitHub, extract to `external/<name>-<version>/`

### `fetch_glad()`

Defined in [`cmake/FetchGlad.cmake`](cmake/FetchGlad.cmake). Uses curl to POST to `gen.glad.sh` with your API/profile/extension specifications, downloads the generated zip, and extracts it. Supports optional `DEBUG` flag for debug mode GLAD output.

### `fetch_glad_allEXT()`

Used by `glew-glad` mode. Downloads `gl.xml` from the Khronos registry, parses all supported GL extensions, and passes them all to `fetch_glad()` - generating GLAD bindings for every extension.

## License

MIT - see [LICENSE](LICENSE).
