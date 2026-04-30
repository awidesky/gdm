# glutil - OpenGL utilities library
cmake_minimum_required(VERSION 3.24)
project(glutil)

# TODO : only when test is enabled
file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/test" GLUTIL_TEST_DIR)
file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/example" GLUTIL_EXAMPLE_DIR)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/config.hpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/config.hpp
    @ONLY
)


# Library target
add_library(glutil)

# Use UTF-8 encoding
target_compile_options(glutil PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

# Target configuration for sofre
target_sources(glutil
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES
      include/glutil/shader.hpp
      include/glutil/inspector.hpp
      include/glutil/glutil.hpp
      include/glutil/path.hpp
      include/glutil/math.hpp
      include/glutil/logging.hpp
      include/glutil/texture.hpp
      include/glutil/model.hpp
)

target_sources(glutil
  PRIVATE
    src/shader.cpp
    src/inspector.cpp
    src/glutil.cpp
    src/logging.cpp
    src/texture.cpp
    src/model.cpp

    src/stb_image.h
    src/dds-ktx.h
    src/tiny_obj_loader.h
)

target_include_directories(glutil PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(glutil PRIVATE
  gdm::gdm
)

# C++ standard
target_compile_features(glutil PRIVATE cxx_std_17)

# Tests (only when GDM is standalone project)
if (GDM_BUILD_TESTS)
  if (GDM_WINDOW_BACKEND STREQUAL "glfw" AND GDM_GL_LOADER STREQUAL "glad" AND GDM_USE_GLM)
    add_executable(glutil_shaderTest)
    target_sources(glutil_shaderTest
          PRIVATE test/shaderTest.cpp
    )
    target_include_directories(glutil_shaderTest
          PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
    )
    target_link_libraries(glutil_shaderTest PRIVATE
          glutil
          gdm::gdm
    )
    target_compile_features(glutil_shaderTest PRIVATE cxx_std_17)
    target_compile_options(glutil_shaderTest PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
    set_target_properties(glutil_shaderTest PROPERTIES
          FOLDER "GLUTIL_Tests"
    )
    add_test(NAME ShaderTest COMMAND glutil_shaderTest)

    add_executable(glutil_encodingCheckBenchMark)
    target_sources(glutil_encodingCheckBenchMark
          PRIVATE test/encodingCheck.cpp
    )
    target_include_directories(glutil_encodingCheckBenchMark
          PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
    )
    target_compile_definitions(glutil_encodingCheckBenchMark PRIVATE GLUTIL_ENABLE_BENCHMARKS)
    target_link_libraries(glutil_encodingCheckBenchMark PRIVATE
          glutil gdm::gdm
    )
    target_compile_features(glutil_encodingCheckBenchMark PRIVATE cxx_std_17)
    target_compile_options(glutil_encodingCheckBenchMark PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
    set_target_properties(glutil_encodingCheckBenchMark PROPERTIES
          FOLDER "GLUTIL_Tests"
    )
    add_test(NAME EncodingCheckBenchmark COMMAND glutil_encodingCheckBenchMark)
  else()
    message(STATUS "[glutil] Tests skipped: require GDM_WINDOW_BACKEND=glfw, GDM_GL_LOADER=glad, and GDM_USE_GLM=ON")
  endif()
endif()

# Examples (require GLFW + GLAD + GLM)
if (GDM_WINDOW_BACKEND STREQUAL "glfw" AND GDM_GL_LOADER STREQUAL "glad" AND GDM_USE_GLM)
  add_executable(glutil_textureExample)
  target_sources(glutil_textureExample
        PRIVATE example/textureExample.cpp
  )
  target_include_directories(glutil_textureExample
        PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
  )
  target_link_libraries(glutil_textureExample PRIVATE
        glutil
        gdm::gdm
  )
  target_compile_features(glutil_textureExample PRIVATE cxx_std_17)
  target_compile_options(glutil_textureExample PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
  set_target_properties(glutil_textureExample PROPERTIES
        FOLDER "GLUTIL_Examples"
  )

  add_executable(glutil_modelExample)
  target_sources(glutil_modelExample
        PRIVATE example/modelExample.cpp
  )
  target_include_directories(glutil_modelExample
        PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
  )
  target_link_libraries(glutil_modelExample PRIVATE
        glutil
        gdm::gdm
  )
  target_compile_features(glutil_modelExample PRIVATE cxx_std_17)
  target_compile_options(glutil_modelExample PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
  set_target_properties(glutil_modelExample PROPERTIES
        FOLDER "GLUTIL_Examples"
  )

  add_executable(glutil_manyCubesExample)
  target_sources(glutil_manyCubesExample
        PRIVATE example/manyCubes.cpp
  )
  target_include_directories(glutil_manyCubesExample
        PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
  )
  target_link_libraries(glutil_manyCubesExample PRIVATE
        glutil
        gdm::gdm
  )
  target_compile_features(glutil_manyCubesExample PRIVATE cxx_std_17)
  target_compile_options(glutil_manyCubesExample PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
  set_target_properties(glutil_manyCubesExample PROPERTIES
        FOLDER "GLUTIL_Examples"
  )
endif()
