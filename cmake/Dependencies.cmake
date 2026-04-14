get_filename_component(GDM_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(GDM_EXTERNAL_DIR "${GDM_ROOT_DIR}/external" CACHE PATH
    "Directory where GDM external dependencies are stored"
)

function(use_or_fetch_package)
    set(options)
    set(oneValueArgs
        NAME
        VERSION
        GIT_REPOSITORY
        GIT_TAG
        ALIAS_TARGET
    )
    set(multiValueArgs
        CANDIDATE_TARGETS
    )

    cmake_parse_arguments(PKG
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
    )

    if (NOT PKG_NAME OR NOT PKG_VERSION)
        message(FATAL_ERROR
            "[${PROJECT_NAME}] use_or_fetch_package requires NAME, and VERSION"
        )
    endif()

    # external directory name with version number
    set(PKG_EXTERNAL_DIR
        ${GDM_EXTERNAL_DIR}/${PKG_NAME}-${PKG_VERSION}
    )

    set(USE_EXTERNAL_PACKAGE FALSE)

    # 1. Prefer external/<name>-<version>
    if (EXISTS ${PKG_EXTERNAL_DIR}/CMakeLists.txt)
        message(STATUS "[${PROJECT_NAME}] Using external ${PKG_NAME} ${PKG_VERSION}" )
        add_subdirectory(${PKG_EXTERNAL_DIR})
        set(USE_EXTERNAL_PACKAGE TRUE)
    endif()

    # 2. Try installed package
    if (NOT USE_EXTERNAL_PACKAGE)

        find_package(${PKG_NAME} QUIET CONFIG EXACT ${PKG_VERSION})

        if (${PKG_NAME}_FOUND)
            message(STATUS
                "[${PROJECT_NAME}] Found installed ${PKG_NAME} ${${PKG_NAME}_VERSION}"
            )

        else()
            message(STATUS
                "[${PROJECT_NAME}] ${PKG_NAME} version ${PKG_VERSION} not found"
            )
            set(USE_EXTERNAL_PACKAGE TRUE)
        endif()

    endif()

    # 3. FetchContent if not found
    if (USE_EXTERNAL_PACKAGE
        AND NOT EXISTS ${PKG_EXTERNAL_DIR}/CMakeLists.txt)

        message(STATUS
            "[${PROJECT_NAME}] Fetching ${PKG_NAME} ${PKG_GIT_TAG}"
        )

        include(FetchContent)
        set(FETCHCONTENT_UPDATES_DISCONNECTED ON)


        FetchContent_Declare(
            ${PKG_NAME}
            URL ${PKG_GIT_REPOSITORY}/archive/${PKG_GIT_TAG}.zip
            SOURCE_DIR ${PKG_EXTERNAL_DIR}
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )

        FetchContent_MakeAvailable(${PKG_NAME})

    endif()

    # Set target name (alias)
    if (PKG_ALIAS_TARGET)
        if (NOT TARGET ${PKG_ALIAS_TARGET})
            foreach(candidate IN LISTS PKG_CANDIDATE_TARGETS)
                if (TARGET ${candidate})
                    add_library(${PKG_ALIAS_TARGET} ALIAS ${candidate})
                    return()
                endif()
            endforeach()

            message(FATAL_ERROR
                "[${PROJECT_NAME}] Failed to resolve ${PKG_NAME} targets. "
                "Candidates: ${PKG_CANDIDATE_TARGETS}"
            )
        endif()
    endif()
endfunction()
