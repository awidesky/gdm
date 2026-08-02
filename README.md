# GDM - OpenGL Dependency Manager

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![CMake](https://img.shields.io/badge/CMake-%3E%3D3.24-blue)](CMakeLists.txt)

**GDM** is a CMake-based OpenGL dependency manager for OpenGL projects.  
Specify your OpenGL library(`glfw`, `freeglut`, `glad`, `glew`, `glm`) and desired version once, and GDM will:
- find installed packages
- download sources if not installed
- and provide clean namespaced targets  
  
so that your dependencies would work in all OS and build system, without adding platform-specific compile/link options or embedding library files inside your project.

## Supported OpenGL Libraries

- **Window backend** (`glfw`, `freeglut`, or `none`)
- **OpenGL function loader** (`glad`, `glew`, `glew-glad`, or `none`)
- **OpenGL Mathematics** (`glm`)
- **OpenGL debugging and resource utility ([GLUtil](#glutil---opengl-utility-library))** (`glutil`)

After you specify the desired libraries and versions, `GDM` searches for installed packages, downloads sources when a package is not already available, and provides targets that resolve to the selected dependencies such as `gdm::window`, `gdm::loader`, `gdm::math`, and `gdm::opengl`.  
The main value is a small set of interface targets that carry link dependencies and compile definitions so application code can resolve OpenGL dependencies across environments and switch providers without changing includes or link lines.

## Usage Examples

### 1. Minimum Example - Project with `glfw`, `glad`, `glm`

The following example searches for the latest installed versions of `glfw` and `glm`.  
If they are not installed, sources are downloaded under `${GDM_EXTERNAL_DIR}` (default: `${CMAKE_SOURCE_DIR}/external`).  
For `glad`, sources are always generated under `${GDM_EXTERNAL_DIR}`, and the default configuration is OpenGL 4.6 Core profile.
```cmake
set(GDM_WINDOW_BACKEND  "glfw"  CACHE STRING "")
set(GDM_GL_LOADER       "glad"  CACHE STRING "")
set(GDM_USE_GLM           ON    CACHE BOOL   "")

add_subdirectory(external/gdm)

add_executable(my_app src/main.cpp)
target_link_libraries(my_app PRIVATE gdm::deps)
```
_It's not even minimum!_ You can omit the part that selects the library name, since the default is `glfw + glad(gl:core=4.6) + glm` combination.  
The `gdm::deps` target links all requested OpenGL dependencies. It can be linked into your project target; see [Generated CMake Targets](#generated-cmake-targets).

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
set(GDM_GLAD_EXTENSION "GL_KHR_debug" CACHE STRING "" FORCE) # semicolon-separated list

# OpenGL Mathematics(glm)
set(GDM_USE_GLM      ON     CACHE BOOL "" FORCE)
set(GDM_GLM_VERSION "1.0.3" CACHE STRING "" FORCE)

# GLUTILL
# GDM_USE_GLUTIL is OFF by default when GDM is a subdirectory (set ON to enable GLUtil)
set(GDM_USE_GLUTIL   ON     CACHE BOOL "" FORCE)

# Downloads the dependencies into /external directory
set(GDM_EXTERNAL_DIR "${CMAKE_SOURCE_DIR}/external" CACHE PATH "" FORCE)

# Fetch gdm from git
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

add_executable(my_app src/main.cpp)
target_link_libraries(my_app PRIVATE gdm::deps gdm::glutil)
```
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

### `gdm::window`

`gdm::window` is an alias for the selected window backend(window creation, OpenGL context management, and input processing) provider.

Set `GDM_WINDOW_BACKEND` to `glfw`, `freeglut`, or `none` (default: `glfw`).  
- `glfw` - Use [GLFW](https://www.glfw.org). Set the version with `GDM_GLFW_VERSION` (default: `3.4.0`).  
- `freeglut` - Use [FreeGLUT](https://freeglut.sourceforge.net). Set the version with `GDM_FREEGLUT_VERSION` (default: `3.8.0`).
- `none` - `gdm::window` links to empty interface target.

### `gdm::loader`

`gdm::loader` is an alias for the selected OpenGL function loader.

Set `GDM_GL_LOADER` to `glad`, `glew`, `glew-glad`, or `none` (default: `glad`).  
- `glad` - [GLAD](https://gen.glad.sh). Generates a loader tailored to your API version, profile, and extension list. Configure with these options:

  | Name | Default |
  |------|---------|
  | `GDM_GLAD_API` | `4.6` |
  | `GDM_GLAD_PROFILE` | `core` |
  | `GDM_GLAD_EXTENSION` | `GL_ARB_debug_output;GL_EXT_debug_label;...` |

  GDM internally creates two static library targets — `gdm_glad_debug` (built with GLAD's `DEBUG` option) and `gdm_glad_release` (built without).
  `gdm::loader` depends on both, and links `gdm_glad_debug` in `Debug`/`RelWithDebInfo` build, and `gdm_glad_release` otherwise.  
  This means [GLAD callback](https://stackoverflow.com/questions/54476931/how-do-i-use-gladcallback) can be used in debug build(when `GDM_DEBUG` and `GLAD_OPTION_GL_DEBUG` macro is defined), and release builds will have no debug callback overhead([example](./external/glutil/example/stacktrace.cpp)).  
  Note that GLAD callback is not [OpenGL debug message callback](https://wikis.khronos.org/opengl/Debug_Output)!
- `glew` - [GLEW](https://glew.sourceforge.net). Set the version with `GDM_GLEW_VERSION` (default: `2.3.1`). Linkage mode controlled by `GDM_GLEW_STATIC` (default: `ON`).  
- `glew-glad` - GLEW API backed by GLAD function loading. This is for legacy projects that already use GLEW, but also need GLUtil's debug features, which require GLAD support. In this mode, GDM generates a small wrapper that keeps GLEW-style headers and calls intact, strips GLEW's direct function declarations from the public header, and redirects loading to GLAD instead. That lets existing GLEW code keep compiling while GLUtil can use GLAD's debug callback path. Example usage: [https://github.com/awidesky/ogl-gdm](https://github.com/awidesky/ogl-gdm). See [glew-glad section](#glew-glad) for details.
- `none` - `gdm::loader` links to empty interface target.

### `gdm::opengl`

`gdm::opengl` provides the system OpenGL library linkage. It resolves to `OpenGL::GL` on desktop platforms (macOS, Linux, Windows) or falls back to `OpenGL::OpenGL` when the modern target is unavailable.

No configuration variables control this target directly - it is always created and always resolves to whatever `find_package(OpenGL)` discovers on the target system.

### `gdm::math`

`gdm::math` is an alias of `glm::glm` - the [GLM](https://github.com/g-truc/glm) header-only math library.

Controlled by `GDM_USE_GLM` (default: `ON`). Set the version with `GDM_GLM_VERSION` (default: `1.0.3`). When `GDM_USE_GLM=OFF`, `gdm::math` links to empty interface target.

### `gdm::defs`

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

### `gdm::deps`

`gdm::deps` bundles all selected GDM-managed dependencies into a single interface target, so you only need one `target_link_libraries(my_app PRIVATE gdm::deps)`.

It links `gdm::defs`, `gdm::opengl`, `gdm::window`, `gdm::loader`, and `gdm::math`.

### `gdm::gdm`

`gdm::gdm` is the main target, currently same as `gdm::deps`. Which one to use is purely a naming preference.

### `gdm::glutil`

`gdm::glutil` is the [GLUtil](#glutil---opengl-utility-library) library - Easy loading for shader, texture, model resources, plus logging, file-path resolution, GL and string helpers, and a comprehensive debug system with error reporting, GL object tracking, labeling, stack traces, runtime info, GL state snapshots, and leak detection. Detailed explanation and usage at [GLUtil - OpenGL Utility Library](#glutil---opengl-utility-library)

Present only when `GDM_USE_GLUTIL=ON`. Defaults to `ON` when GDM is the main project, and `OFF` when GDM is consumed as a subdirectory — set `GDM_USE_GLUTIL=ON` to enable it. This target is **not** included in `gdm::deps` - link it explicitly when you need GLUtil features:

```cmake
target_link_libraries(my_app PRIVATE gdm::deps gdm::glutil)
```

## Additional Configuration Options

| Option | Values | Default | Description |
|--------|--------|---------|-------------|
| `GDM_BUILD_EXAMPLES` | `ON`, `OFF` | `ON` (standalone) | Build examples |
| `GDM_BUILD_TESTS` | `ON`, `OFF` | `ON` (standalone) | Build GLUtil tests |
| `GDM_EXTERNAL_DIR` | path | `${CMAKE_SOURCE_DIR}/external` | External dependency directory |
| `GDM_TLS_VERIFY` | `ON`, `OFF` | `ON` | TLS verification for downloads |
| `GLUTIL_DISABLE_LOG_ON_RELEASE` | `ON`, `OFF` | `OFF` (ON as subproject) | Suppress GLUtil logging in non-debug builds |
  
  

# GLUtil - OpenGL Utility Library
[![C++](https://img.shields.io/badge/C%2B%2B-17-blue)](./external/glutil/CMakeLists.txt)

GLUtil is a built-in utility library shipped under `external/glutil/`. It provides resource wrappers, debug helpers, runtime inspection tools, and a comprehensive debug system for OpenGL applications.

### Core Modules

| Module | Header | Description |
|--------|--------|-------------|
| **Shader** | `glutil/shader.hpp` | File loading with BOM detection (UTF-8/16/32), compilation, linking, and RAII `GLShader` / `GLProgram` wrappers. |
| **Inspector** | `glutil/inspector.hpp` | Structured shader and program inspection via `InspectResult`. |
| **Model** | `glutil/model.hpp` | OBJ loading via `tiny_obj_loader` into CPU `ModelData` or GPU `GLModelData` (VAO/VBO/EBO), with optional vertex deduplication. |
| **Texture** | `glutil/texture.hpp` | Texture loading via `stb_image` and DDS/KTX support, with CPU `TextureImage` / `TextureDDS` and GPU `GLTexture2D`. |
| **Path** | `glutil/path.hpp` | Multi-strategy file resolution (cwd -> executable dir -> project root). |
| **Logging** | `glutil/logging.hpp` | Stream-style logger with `[INFO]`, `[WARNING]`, and `[ERROR]` sinks. |
| **Math** | `glutil/math.hpp` | Predefined vertex types (`VertexP`, `VertexPC`, `VertexPT`, `VertexPNT`, `VertexPNCT`) with compile-time validation and GLM accessors. |
| **OpenGL helpers** | `glutil/gl.hpp`, `glutil/glToString.hpp` | OpenGL convenience wrappers and enum-to-string helpers. |

### Debug System (GDM_DEBUG=1 only)

| Component | Description |
|-----------|-------------|
| `glutil::debug::init()` | Set up behind-the scene debug feature like debug callback, object tracking. |
| `glutil::debug::labelGLobject()` | Label GL objects when debug labeling is available. |
| `glutil::debug::getGLobjectLabel()` | Retrieve an object's debug label. |
| `glutil::debug::isGL_KHR_debugSupported()` | Runtime check for `KHR_debug` availability. |
| `glutil::debug::printRuntimeInfo()` | Print GL version, vendor, renderer, extensions, and debug capabilities. |
| `glutil::debug::printStackTrace()` | Print a stack trace for debug callbacks and error paths. |
| `glutil::debug::availableGLversion()` | Detect the highest GL version supported by the current runtime. |
| `GLStateTracker` | Global singleton tracking GL object creation/destruction. Reports leaked objects on shutdown. |
| `Snapshot` | Full GL state capture: framebuffers, shader programs + uniforms, textures, VAO/VBO/EBO layout, renderer state, and binding state. Supports async output to stream or file. |
| `debug::disableAutoLabelGLObjects` | Global toggle for automatic GL object labeling. |
| `debug::disableAutoInspcector` | Global toggle for automatic inspection hooks. |

#### Automatic debug initialization

When using GLAD with `GDM_DEBUG=1`, GLUtil provides `#define` overrides that hijack `gladLoadGL` / `gladLoadGLUserPtr` / `gladLoaderLoadGL` to automatically call `glutil::debug::init()` after a successful load - no manual init required.

#### GL object leak detection

`GLStateTracker` destructor automatically logs all unreleased GL objects (buffers, textures, programs, etc.) with their debug labels, making resource leak detection trivial in debug builds.

#### Debug helpers

`glutil::debug::debug_callback.hpp` exposes callback plumbing for GLAD and OpenGL debug output, `debug_stacktrace.hpp` adds stack-trace printing for error callbacks, `debug_info.hpp` prints runtime capability summaries, and `debug_snapshot.hpp` captures full renderer state for offline inspection.

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

## glew-glad

Use `GDM_GL_LOADER=glew-glad` when you want the **GLEW API** backed by **GLAD function loading**:

- Programs written against GLEW don't need include changes
- GLAD provides KHR_debug support, letting you use GLUtil's debug features
- At CMake time, a fake `glew.h` is generated that strips GLEW's function pointer declarations and injects GLAD's headers instead
- This enables GLEW-style code to work in GL 4.6 core profile contexts
- This is useful for legacy projects that already use GLEW, but still want GLUtil's debug features without rewriting their OpenGL loader layer

```cmake
set(GDM_GL_LOADER glew-glad CACHE STRING "")
```

Example project: [awidesky/ogl-gdm](https://github.com/awidesky/ogl-gdm)

### GLUtil Examples

The GLUtil examples live under `external/glutil/example/`, not under the top-level `examples/` directory. They cover:

- [texture.cpp](external/glutil/example/texture.cpp) - texture loading and sampling
- [model.cpp](external/glutil/example/model.cpp) - OBJ model loading and GPU upload
- [manyCubes.cpp](external/glutil/example/manyCubes.cpp) - math, transforms, and rendering pipeline usage
- [normalMapping.cpp](external/glutil/example/normalMapping.cpp) - normal mapping and tangent-space setup
- [debugLabel.cpp](external/glutil/example/debugLabel.cpp) - object labeling and debug output
- [autoLabeling.cpp](external/glutil/example/autoLabeling.cpp) - automatic labeling behavior
- [stacktrace.cpp](external/glutil/example/stacktrace.cpp) - GLAD callback and stack trace printing
- [snapshot.cpp](external/glutil/example/snapshot.cpp) - renderer state capture
- [tracker.cpp](external/glutil/example/tracker.cpp) - object tracking and leak reporting

The top-level `examples/01_helloWindow.cpp` is a GDM example that demonstrates how to use `gdm::deps` and `gdm::window` / `gdm::loader` in an application.

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
- Optional GLUtil-assisted shader inspection when `GDM_USE_GLUTIL=ON`
- Basic triangle rendering with VAO/VBO and shader program

Build with: `cmake -B build && cmake --build build`

GLUtil examples are built when `GDM_GL_LOADER=glad` and `GDM_WINDOW_BACKEND=glfw` and `GDM_USE_GLM=ON` - see `external/glutil/example/` for 9 additional examples covering textures, models, transforms, normal mapping, debug labeling, auto-labeling, stacktraces, snapshots, and tracking.

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
