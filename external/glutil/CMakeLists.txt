# glutil - OpenGL utilities library
cmake_minimum_required(VERSION 3.24)
project(glutil)

# Library target
add_library(glutil)


# Target configuration for sofre
target_sources(glutil
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES
      include/glutil/inspector.hpp
      include/glutil/glutil.hpp
)

target_sources(glutil
  PRIVATE
    src/inspector.cpp
    src/glutil.cpp
)

target_include_directories(glutil PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(glutil PRIVATE
  gdm::gdm
)

# C++ standard
target_compile_features(glutil PRIVATE cxx_std_17)