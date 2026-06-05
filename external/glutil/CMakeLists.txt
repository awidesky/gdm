# glutil - OpenGL utilities library
cmake_minimum_required(VERSION 3.24)
project(glutil
	VERSION 1.0.0 
	LANGUAGES C CXX
	HOMEPAGE_URL "https://github.com/awidesky/gdm"
)

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

# Target configuration for glutil
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
      
      include/glutil/gl.hpp
      include/glutil/glToString.hpp
      
      include/glutil/debug.hpp
      include/glutil/debug_callback.hpp
      include/glutil/debug_stacktrace.hpp
      include/glutil/debug_info.hpp
      include/glutil/debug_snapshot.hpp
      include/glutil/debug_tracker.hpp
)

target_sources(glutil
  PRIVATE
    src/shader.cpp
    src/inspector.cpp
    src/glutil.cpp
    src/path.cpp
    src/texture.cpp
    src/model.cpp

    src/debug.cpp
    src/debug_callback.cpp
    src/debug_stacktrace.cpp
    src/debug_info.cpp
    src/debug_snapshot.cpp

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

target_compile_definitions(glutil PUBLIC
    $<$<CONFIG:Debug>:GLUTIL_DEBUG=1>
    $<$<NOT:$<CONFIG:Debug>>:GLUTIL_DEBUG=0>
    
    # Build type as macro
    $<$<CONFIG:Debug>:GLUTIL_BUILD_TYPE_DEBUG>
    $<$<CONFIG:Release>:GLUTIL_BUILD_TYPE_RELEASE>
    $<$<CONFIG:RelWithDebInfo>:GLUTIL_BUILD_TYPE_RELWITHDEBINFO>
    $<$<CONFIG:MinSizeRel>:GLUTIL_BUILD_TYPE_MINSIZEREL>
)
    

function(add_glutil_example TARGET_NAME)
    add_executable(${TARGET_NAME} ${ARGN})

    target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

    target_link_libraries(${TARGET_NAME} PRIVATE
        glutil
        gdm::gdm
    )

    target_compile_features(${TARGET_NAME} PRIVATE cxx_std_17)
    if(MSVC)
        target_compile_options(${TARGET_NAME} PRIVATE
            /W4 /utf-8
        )
    else()
        target_compile_options(${TARGET_NAME} PRIVATE
            -Wall -Wextra -Wpedantic
        )
    endif()

    set_target_properties(${TARGET_NAME} PROPERTIES
        FOLDER "GLUTIL_Examples"
    )
endfunction()

function(add_glutil_test TARGET_NAME TEST_NAME)
    add_executable(${TARGET_NAME} ${ARGN})

    target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

    target_link_libraries(${TARGET_NAME} PRIVATE
        glutil
        gdm::gdm
    )

    target_compile_features(${TARGET_NAME} PRIVATE cxx_std_17)
    if(MSVC)
        target_compile_options(${TARGET_NAME} PRIVATE
            /W4 /utf-8
        )
    else()
        target_compile_options(${TARGET_NAME} PRIVATE
            -Wall -Wextra -Wpedantic
        )
    endif()

    set_target_properties(${TARGET_NAME} PROPERTIES
        FOLDER "GLUTIL_Tests"
    )

    add_test(NAME ${TEST_NAME} COMMAND ${TARGET_NAME})
endfunction()

# Tests (only when GDM is standalone project)
if (GDM_BUILD_TESTS)
  if (GDM_WINDOW_BACKEND STREQUAL "glfw" AND GDM_GL_LOADER STREQUAL "glad" AND GDM_USE_GLM)
    add_glutil_test(glutil_shaderTest
          ShaderTest
          test/shaderTest.cpp
    )

    add_glutil_test(glutil_encodingCheckBenchMark
          EncodingCheckBenchmark
          test/encodingCheck.cpp
    )
    target_compile_definitions(glutil_encodingCheckBenchMark PRIVATE GLUTIL_ENABLE_BENCHMARKS)

    add_glutil_test(glutil_pathResolveTest
          PathResolveTest
          test/pathResolveTest.cpp
    )
  else()
    message(STATUS "[glutil] Tests skipped: require GDM_WINDOW_BACKEND=glfw, GDM_GL_LOADER=glad, and GDM_USE_GLM=ON")
  endif()
endif()

# Examples (require GLFW + GLAD + GLM)
if (GDM_WINDOW_BACKEND STREQUAL "glfw" AND GDM_GL_LOADER STREQUAL "glad" AND GDM_USE_GLM)
  add_glutil_example(glutil_textureExample
        example/texture.cpp
  )

  add_glutil_example(glutil_modelExample
        example/model.cpp
  )

  add_glutil_example(glutil_manyCubesExample
        example/manyCubes.cpp
  )

  add_glutil_example(glutil_normalMappingExample
        example/normalMapping.cpp
  )

  add_glutil_example(glutil_debugLabelExample
        example/debugLabel.cpp
  )

  add_glutil_example(glutil_autoLabelingExample
        example/autoLabeling.cpp
  )

  add_glutil_example(glutil_stacktraceExample
        example/stacktrace.cpp
  )

  add_glutil_example(glutil_snapshotExample
        example/snapshot.cpp
  )

  add_glutil_example(glutil_trackerExample
        example/tracker.cpp
  )
endif()
