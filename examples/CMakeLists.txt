cmake_minimum_required(VERSION 3.24)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

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
endif()

target_compile_features(gdm_example_1_HelloWindow PRIVATE cxx_std_17)
target_compile_options(gdm_example_1_HelloWindow PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
