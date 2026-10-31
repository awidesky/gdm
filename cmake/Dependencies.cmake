# get_latest_version(owner repo result_var)
# Fetches the latest release tag from a GitHub repository.
# Normalizes the version: strips leading 'v', appends '.0' if only two components (e.g. 3.4 -> 3.4.0).
# On failure, aborts configuration with FATAL_ERROR.
function(get_latest_version owner repo result_var)
    set(API_URL "https://api.github.com/repos/${owner}/${repo}/releases/latest")
    set(json_file "${CMAKE_BINARY_DIR}/_gdm_downloads/${repo}_latest_tag.json")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/_gdm_downloads")

    file(DOWNLOAD "${API_URL}" "${json_file}"
        STATUS _dl_status
        TLS_VERIFY ${GDM_TLS_VERIFY}
        INACTIVITY_TIMEOUT 10
    )

    list(GET _dl_status 0 _dl_code)
    if(NOT _dl_code EQUAL 0)
        list(GET _dl_status 1 _dl_msg)
        message(FATAL_ERROR
            "[${PROJECT_NAME}] Failed to get latest version for ${owner}/${repo}: ${_dl_msg}\n"
            "[${PROJECT_NAME}] Set ${result_var} manually and re-configure."
        )
    endif()

    file(READ "${json_file}" _json)
    string(JSON _tag ERROR_VARIABLE _json_err GET "${_json}" tag_name)
    if(_json_err OR _tag STREQUAL "")
        message(FATAL_ERROR
            "[${PROJECT_NAME}] Failed to parse latest release tag for ${owner}/${repo}\n"
            "[${PROJECT_NAME}] Set ${result_var} manually and re-configure."
        )
    endif()

    # Strip leading 'v' if present
    string(REGEX REPLACE "^v" "" _ver "${_tag}")

    # If version has only one dot (e.g. 3.4), append .0
    string(REGEX MATCHALL "\\." _dots "${_ver}")
    list(LENGTH _dots _dot_count)
    if(_dot_count EQUAL 1)
        set(_ver "${_ver}.0")
    endif()

    set(${result_var} "${_ver}" PARENT_SCOPE)
endfunction()


# How long (seconds) to wait for the GDM dependency lock before failing.
set(GDM_LOCK_TIMEOUT 60)

# gdm_lock_guard(<lock_file>)
# Expands to a file(LOCK ... GUARD FUNCTION) call so the lock is released automatically when function returns.
# Serializes dependency fetch/extract/rename against other CMake processes sharing the same _gdm_lock_dir.
# On failure (lock held past GDM_LOCK_TIMEOUT), aborts with error message.
macro(gdm_lock_guard lock_file)
    get_filename_component(_gdm_lock_dir "${lock_file}" DIRECTORY)
    file(MAKE_DIRECTORY "${_gdm_lock_dir}")
    file(LOCK "${lock_file}"
        GUARD FUNCTION
        TIMEOUT ${GDM_LOCK_TIMEOUT}
        RESULT_VARIABLE _gdm_lock_result
    )
    if(_gdm_lock_result)
        message(FATAL_ERROR
            "[${PROJECT_NAME}] Failed to acquire dependency lock '${lock_file}': ${_gdm_lock_result}\n"
            "Another CMake configure may be working on the same directory(${_gdm_lock_dir}),\n"
            "or there's no sufficient permissions in the external directory.\n"
            "If no other configuration is active, delete the lock file and re-configure."
        )
    endif()
endmacro()


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

    if (NOT PKG_NAME OR NOT PKG_VERSION OR NOT PKG_CANDIDATE_TARGETS)
        message(FATAL_ERROR
            "[${PROJECT_NAME}] use_or_fetch_package requires NAME, VERSION, and CANDIDATE_TARGETS"
        )
    endif()

    # Serialize the existence check + fetch/extract/rename against other
    # configure processes sharing the same GDM_EXTERNAL_DIR.
    gdm_lock_guard("${GDM_EXTERNAL_DIR}/.gdm.lock")

    # external directory name with version number
    set(PKG_EXTERNAL_DIR
        ${GDM_EXTERNAL_DIR}/${PKG_NAME}-${PKG_VERSION}
    )

    # When dependency sources live outside the current source tree 
    # (e.g. GDM_EXTERNAL_DIR points to a parent project's external/),
    # CMake requires an explicit binary directory argument to add_subdirectory().
    # Keep builds isolated per-dependency in the top-level build tree.
    set(_gdm_pkg_binary_dir "${CMAKE_BINARY_DIR}/_gdm_deps/${PKG_NAME}-${PKG_VERSION}")

    set(USE_EXTERNAL_PACKAGE FALSE)

    # 1. Prefer external/<name>-<version>
    if (EXISTS ${PKG_EXTERNAL_DIR}/CMakeLists.txt)
        message(STATUS "[${PROJECT_NAME}] Found external ${PKG_NAME} ${PKG_VERSION}" )
        add_subdirectory("${PKG_EXTERNAL_DIR}" "${_gdm_pkg_binary_dir}")
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
            file(REMOVE "${_pkg_archive}")
            file(REMOVE_RECURSE "${_pkg_extract_dir}")
            message(FATAL_ERROR
                "[${PROJECT_NAME}] ${PKG_NAME}: could not find extracted root directory in ${_pkg_extract_dir}"
            )
        endif()

        # Replace any partial/old directory and move extracted root into place
        file(REMOVE_RECURSE "${PKG_EXTERNAL_DIR}")
        file(RENAME "${_pkg_root}" "${PKG_EXTERNAL_DIR}")
        file(REMOVE_RECURSE "${_pkg_extract_dir}")

        if(NOT EXISTS "${PKG_EXTERNAL_DIR}/CMakeLists.txt")
            file(REMOVE "${_pkg_archive}")
             file(REMOVE_RECURSE "${PKG_EXTERNAL_DIR}")
            message(FATAL_ERROR
                "[${PROJECT_NAME}] ${PKG_NAME}: extracted directory does not contain CMakeLists.txt: ${PKG_EXTERNAL_DIR}"
            )
        endif()
        add_subdirectory("${PKG_EXTERNAL_DIR}" "${_gdm_pkg_binary_dir}")

        string(TIMESTAMP _pkg_end_time "%s" UTC)
        math(EXPR _pkg_elapsed_time "${_pkg_end_time} - ${_pkg_start_time}")
        message(STATUS "[${PROJECT_NAME}] Download+extract for ${PKG_NAME} completed in ${_pkg_elapsed_time}s")

    endif()

    # Set IMPORTED_GLOBAL to true, so that the parent project can see the original target.
    foreach(candidate IN LISTS PKG_CANDIDATE_TARGETS)
        if (TARGET ${candidate})
            get_target_property(_gdm_candidate_imported ${candidate} IMPORTED)
            if (_gdm_candidate_imported)
                set_property(TARGET ${candidate} PROPERTY IMPORTED_GLOBAL TRUE)
            endif()
        endif()
    endforeach()

    # Set target name (alias)
    if (PKG_ALIAS_TARGET)
        if (NOT TARGET ${PKG_ALIAS_TARGET})
            foreach(candidate IN LISTS PKG_CANDIDATE_TARGETS)
                if (TARGET ${candidate})
                    get_target_property(_gdm_candidate_imported ${candidate} IMPORTED)
                    if (_gdm_candidate_imported)
                        string(REPLACE "::" "_" _gdm_alias_impl "${PKG_ALIAS_TARGET}")
                        if (NOT TARGET ${_gdm_alias_impl})
                            add_library(${_gdm_alias_impl} INTERFACE)
                            target_link_libraries(${_gdm_alias_impl} INTERFACE ${candidate})
                        endif()
                        add_library(${PKG_ALIAS_TARGET} ALIAS ${_gdm_alias_impl})
                    else()
                        add_library(${PKG_ALIAS_TARGET} ALIAS ${candidate})
                    endif()
                    return()
                endif()
            endforeach()

            message(FATAL_ERROR
                "[${PROJECT_NAME}] Failed to resolve ${PKG_NAME} targets. "
                "Candidates: ${PKG_CANDIDATE_TARGETS}"
            )
        endif()
    endif()

    unset(_gdm_pkg_binary_dir)
endfunction()


# -----------------------
# Utility Functions
# -----------------------

# gdm_fetch_files(NAME TAG REPOSITORY <url> FILES <file>... OUT_DIR <var>)
#
# Downloads one or more files from a GitHub repository (raw.githubusercontent.com)
# into ${GDM_EXTERNAL_DIR}/<name>-<tag>/, keyed by (name, tag).
# Each file is stored under its base name.
# No CMake target is created; use OUT_DIR to consume the fetched files
# (e.g. target_include_directories() for headers).
# TAG may be a branch, tag, or commit hash; 40-char commit hashes are shortened
# to 7 characters (git short-hash style) for the on-disk directory name.
#
# On success, sets OUT_DIR to the fetched directory.
# If all requested files already exist, returns immediately.
function(gdm_fetch_files)
    set(options)
    set(oneValueArgs
        NAME
        TAG
        REPOSITORY
        OUT_DIR
    )
    set(multiValueArgs
        FILES
    )

    cmake_parse_arguments(FF
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
    )

    if (NOT FF_NAME OR NOT FF_TAG OR NOT FF_FILES OR NOT FF_REPOSITORY OR NOT FF_OUT_DIR)
        message(FATAL_ERROR
            "[${PROJECT_NAME}] gdm_fetch_files requires NAME, TAG, REPOSITORY, FILES and OUT_DIR"
        )
    endif()

    # Shorten full 40-char commit hashes to git-style 7-char short hashes
    # for the on-disk directory name (keeps paths readable).
    # NOTE: length is checked with string(LENGTH) because CMake's MATCHES
    # does not support {n} repetition (it silently fails to match).
    set(_ff_tag_dir "${FF_TAG}")
    string(LENGTH "${FF_TAG}" _ff_tag_len)
    if (_ff_tag_len EQUAL 40 AND FF_TAG MATCHES "^[0-9a-fA-F]+$")
        string(SUBSTRING "${FF_TAG}" 0 7 _ff_tag_dir)
    endif()

    set(_ff_dir "${GDM_EXTERNAL_DIR}/${FF_NAME}-${_ff_tag_dir}")

    # Destination path for each file (stored under its base name).
    set(_ff_dst_files "")
    foreach(_ff_file IN LISTS FF_FILES)
        get_filename_component(_ff_filename "${_ff_file}" NAME)
        list(APPEND _ff_dst_files "${_ff_dir}/${_ff_filename}")
    endforeach()

    # Every requested file must be present to reuse the cached directory.
    set(_ff_all_exist TRUE)
    foreach(_ff_dst IN LISTS _ff_dst_files)
        if (NOT EXISTS "${_ff_dst}")
            set(_ff_all_exist FALSE)
        endif()
    endforeach()

    if (_ff_all_exist)
        message(STATUS "[${PROJECT_NAME}] Found external files ${FF_NAME} ${_ff_tag_dir}")
        set(${FF_OUT_DIR} "${_ff_dir}" PARENT_SCOPE)
        return()
    endif()

    # Serialize the download against other configure processes sharing GDM_EXTERNAL_DIR.
    gdm_lock_guard("${GDM_EXTERNAL_DIR}/.gdm.lock")

    # Re-check inside the lock (another process may have completed the download).
    set(_ff_all_exist TRUE)
    foreach(_ff_dst IN LISTS _ff_dst_files)
        if (NOT EXISTS "${_ff_dst}")
            set(_ff_all_exist FALSE)
        endif()
    endforeach()
    if (_ff_all_exist)
        message(STATUS "[${PROJECT_NAME}] Found external files ${FF_NAME} ${_ff_tag_dir} after sync")
        set(${FF_OUT_DIR} "${_ff_dir}" PARENT_SCOPE)
        return()
    endif()

    string(REGEX REPLACE "^https://github.com/" "https://raw.githubusercontent.com/" _ff_raw_repo "${FF_REPOSITORY}")
    set(_ff_root "${_ff_raw_repo}/${FF_TAG}")

    # Download to a staging dir first, then rename into place (atomic like use_or_fetch_package).
    set(_ff_staging_dir "${CMAKE_BINARY_DIR}/_gdm_extract/${FF_NAME}-${FF_TAG}")
    file(REMOVE_RECURSE "${_ff_staging_dir}")
    file(MAKE_DIRECTORY "${_ff_staging_dir}")

    message(STATUS "[${PROJECT_NAME}] Fetching files ${FF_NAME} ${FF_TAG}")

    foreach(_ff_file IN LISTS FF_FILES)
        get_filename_component(_ff_filename "${_ff_file}" NAME)
        set(_ff_dst "${_ff_staging_dir}/${_ff_filename}")

        file(DOWNLOAD "${_ff_root}/${_ff_file}" "${_ff_dst}"
            STATUS _dl_status
            TLS_VERIFY ${GDM_TLS_VERIFY}
        )
        list(GET _dl_status 0 _dl_code)
        if(NOT _dl_code EQUAL 0)
            list(GET _dl_status 1 _dl_msg)
            file(REMOVE_RECURSE "${_ff_staging_dir}")
            message(FATAL_ERROR
                "[${PROJECT_NAME}] Failed to download file ${_ff_file} (${FF_NAME} ${FF_TAG}):\n"
                "URL: ${_ff_root}/${_ff_file}\n"
                "Reason: ${_dl_msg}"
            )
        endif()
    endforeach()

    # Replace any partial/old directory and move the staging dir into place
    file(REMOVE_RECURSE "${_ff_dir}")
    file(RENAME "${_ff_staging_dir}" "${_ff_dir}")
    file(REMOVE_RECURSE "${_ff_staging_dir}")

    message(STATUS "[${PROJECT_NAME}] Fetched files ${FF_NAME} ${_ff_tag_dir} -> ${_ff_dir}")
    set(${FF_OUT_DIR} "${_ff_dir}" PARENT_SCOPE)
endfunction()
