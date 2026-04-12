# glutil - OpenGL utilities library
cmake_minimum_required(VERSION 3.24)
project(glutil)

file(TO_NATIVE_PATH "${CMAKE_SOURCE_DIR}" GLUTIL_ROOT_DIR)
# TODO : only when test is enabled
file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/test" GLUTIL_TEST_DIR)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/config.hpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/config.hpp
    @ONLY
)


# Library target
add_library(glutil)


# Target configuration for sofre
target_sources(glutil
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES
      include/glutil/shader.hpp
      include/glutil/inspector.hpp
      include/glutil/glutil.hpp
      include/glutil/logging.hpp
)

target_sources(glutil
  PRIVATE
    src/shader.cpp
    src/inspector.cpp
    src/glutil.cpp
    src/logging.cpp
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
  add_test(NAME EncodingCheckBenchmark COMMAND glutil_encodingCheckBenchMark)
endif()