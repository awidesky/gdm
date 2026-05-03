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

    # 3. Download+extract if not found
    if (USE_EXTERNAL_PACKAGE
        AND NOT EXISTS ${PKG_EXTERNAL_DIR}/CMakeLists.txt)

        message(STATUS
            "[${PROJECT_NAME}] Fetching ${PKG_NAME} ${PKG_GIT_TAG}"
        )

        # Time the download+extract operation
        string(TIMESTAMP _pkg_start_time "%s" UTC)

        set(_pkg_url "${PKG_GIT_REPOSITORY}/archive/${PKG_GIT_TAG}.tar.gz")
        # Where to store downloaded archives (per build dir)
        set(_pkg_dl_dir "${CMAKE_BINARY_DIR}/_gdm_downloads")
        file(MAKE_DIRECTORY "${_pkg_dl_dir}")
        # Stable filename keyed by (name, tag).
        set(_pkg_archive "${_pkg_dl_dir}/${PKG_NAME}-${PKG_GIT_TAG}.tar.gz")

        # Extract to temp dir first (GitHub tarballs have a top-level folder)
        set(_pkg_extract_dir "${CMAKE_BINARY_DIR}/_gdm_extract/${PKG_NAME}-${PKG_GIT_TAG}")
        file(REMOVE_RECURSE "${_pkg_extract_dir}")
        file(MAKE_DIRECTORY "${_pkg_extract_dir}")

        # Download (skip if already downloaded in this build dir)
        if(NOT EXISTS "${_pkg_archive}")

            file(DOWNLOAD "${_pkg_url}" "${_pkg_archive}"
                STATUS _dl_status
                TLS_VERIFY ${GDM_TLS_VERIFY}
            )
            list(GET _dl_status 0 _dl_code)
            if(NOT _dl_code EQUAL 0)
                file(REMOVE "${_pkg_archive}") 
                list(GET _dl_status 1 _dl_msg)
                message(FATAL_ERROR
                    "[${PROJECT_NAME}] Failed to download ${PKG_NAME} ${PKG_GIT_TAG}: ${_dl_msg}\n"
                    "URL: ${_pkg_url}\n"
                    "Archive: ${_pkg_archive}"
                )
            endif()
        else()
            message(STATUS "[${PROJECT_NAME}] Using cached archive: ${_pkg_archive}")
        endif()

        # Extract archive to temp
        file(ARCHIVE_EXTRACT
            INPUT "${_pkg_archive}"
            DESTINATION "${_pkg_extract_dir}"
        )

        # Find extracted top-level directory (usually exactly one)
        file(GLOB _pkg_children LIST_DIRECTORIES true "${_pkg_extract_dir}/*")
        set(_pkg_root "")
        foreach(p IN LISTS _pkg_children)
            if(IS_DIRECTORY "${p}")
                set(_pkg_root "${p}")
                break()
            endif()
        endforeach()

        if(NOT _pkg_root)
            message(FATAL_ERROR
                "[${PROJECT_NAME}] ${PKG_NAME}: could not find extracted root directory in ${_pkg_extract_dir}"
            )
        endif()

        # Replace any partial/old directory and move extracted root into place
        file(REMOVE_RECURSE "${PKG_EXTERNAL_DIR}")
        file(RENAME "${_pkg_root}" "${PKG_EXTERNAL_DIR}")
        file(REMOVE_RECURSE "${_pkg_extract_dir}")

        if(NOT EXISTS "${PKG_EXTERNAL_DIR}/CMakeLists.txt")
            message(FATAL_ERROR
                "[${PROJECT_NAME}] ${PKG_NAME}: extracted directory does not contain CMakeLists.txt: ${PKG_EXTERNAL_DIR}"
            )
        endif()
        add_subdirectory("${PKG_EXTERNAL_DIR}")

        string(TIMESTAMP _pkg_end_time "%s" UTC)
        math(EXPR _pkg_elapsed_time "${_pkg_end_time} - ${_pkg_start_time}")
        message(STATUS "[${PROJECT_NAME}] Download+extract for ${PKG_NAME} completed in ${_pkg_elapsed_time}s")

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
