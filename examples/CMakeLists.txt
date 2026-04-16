cmake_minimum_required(VERSION 3.24)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Examples in this folder currently require GLFW + GLAD + GLM.
# Skip examples (without failure) when dependency selections differ.
if (NOT GDM_WINDOW_BACKEND STREQUAL "glfw"
    OR NOT GDM_GL_LOADER STREQUAL "glad"
    OR NOT GDM_USE_GLM)
    message(STATUS
        "[GDM] Skipping examples: requires GDM_WINDOW_BACKEND=glfw, "
        "GDM_GL_LOADER=glad, and GDM_USE_GLM=ON"
    )
    return()
endif()

# Example 1: Hello Window + OpenGL dependency check
add_executable(gdm_example_1_HelloWindow
    01_helloWindow.cpp
)

set_target_properties(gdm_example_1_HelloWindow PROPERTIES
    FOLDER "GDM_Examples"
)

target_link_libraries(gdm_example_1_HelloWindow
    PRIVATE
        gdm
)

if (TARGET glutil)
    target_link_libraries(gdm_example_1_HelloWindow
        PRIVATE
            glutil
    )

    target_compile_definitions(gdm_example_1_HelloWindow
        PRIVATE
            GDM_HAS_GLUTIL=1
    )
else()
    target_compile_definitions(gdm_example_1_HelloWindow
        PRIVATE
            GDM_HAS_GLUTIL=0
    )
endif()

target_compile_features(gdm_example_1_HelloWindow PRIVATE cxx_std_17)
target_compile_options(gdm_example_1_HelloWindow PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
