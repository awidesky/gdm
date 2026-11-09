# gdm_get_latest_version(
#   REPO        <owner>/<repo>    -- repository whose tags feed lists the versions
#   TAG_RE      <regex>           -- one capture group: the version inside a tag
#   OUT_VERSION <var>             -- newest version, to be used as the default
# )
#
# Reads https://github.com/<REPO>/tags.atom: a plain Atom feed, so no GitHub
# API, no rate limit and no authentication. The feed holds the 10 most recently
# created tags; the newest one *by version* wins (not the first one), so
# repositories that push tags out of order are fine. A two-component version is
# padded to three, to match the version the package reports itself (GLFW tags
# 3.4, but installs as 3.4.0).
#
# This only provides the value for an option that the user did not set; call it
# from inside an `if(NOT DEFINED <option>)` guard.
function(gdm_get_latest_version)
    set(options)
    set(oneValueArgs REPO TAG_RE OUT_VERSION)
    cmake_parse_arguments(GV "${options}" "${oneValueArgs}" "" ${ARGN})

    if(NOT GV_REPO OR NOT GV_TAG_RE OR NOT GV_OUT_VERSION)
        message(FATAL_ERROR
            "[${PROJECT_NAME}] gdm_get_latest_version requires REPO, TAG_RE and OUT_VERSION"
        )
    endif()

    string(REGEX REPLACE "[^A-Za-z0-9._-]" "_" _slug "${GV_REPO}")
    set(_feed_file "${CMAKE_BINARY_DIR}/_gdm_downloads/${_slug}_tags.atom")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/_gdm_downloads")
    file(DOWNLOAD "https://github.com/${GV_REPO}/tags.atom" "${_feed_file}"
        STATUS _dl_status
        TLS_VERIFY ${GDM_TLS_VERIFY}
        INACTIVITY_TIMEOUT 10
    )
    list(GET _dl_status 0 _dl_code)
    if(NOT _dl_code EQUAL 0)
        list(GET _dl_status 1 _dl_msg)
        message(FATAL_ERROR
            "[${PROJECT_NAME}] Failed to read the tags feed of ${GV_REPO}: ${_dl_msg}\n"
            "[${PROJECT_NAME}] URL: https://github.com/${GV_REPO}/tags.atom\n"
            "[${PROJECT_NAME}] Set ${GV_OUT_VERSION} manually and re-configure."
        )
    endif()

    file(READ "${_feed_file}" _feed)
    string(REGEX MATCHALL "Repository/[0-9]+/[^<]+</id>" _feed_ids "${_feed}")
    set(_versions "")
    foreach(_id IN LISTS _feed_ids)
        string(REGEX REPLACE "^Repository/[0-9]+/(.*)</id>$" "\\1" _tag "${_id}")
        if(_tag MATCHES "${GV_TAG_RE}")
            list(APPEND _versions "${CMAKE_MATCH_1}")
        endif()
    endforeach()
    if(NOT _versions)
        message(FATAL_ERROR
            "[${PROJECT_NAME}] No tag of ${GV_REPO} matches '${GV_TAG_RE}'\n"
            "[${PROJECT_NAME}] Set ${GV_OUT_VERSION} manually and re-configure."
        )
    endif()
    list(SORT _versions COMPARE NATURAL) # 2.3.10 sorts above 2.3.9
    list(REVERSE _versions)
    list(GET _versions 0 _version)

    # A two-component version is padded to three, to match the version the
    # package reports itself (GLFW tags 3.4, but installs as 3.4.0).
    string(REGEX MATCHALL "\\." _dots "${_version}")
    list(LENGTH _dots _dot_count)
    if(_dot_count EQUAL 1)
        set(_version "${_version}.0")
    endif()

    set(${GV_OUT_VERSION} "${_version}" PARENT_SCOPE)
endfunction()


# How long (seconds) to wait for the GDM dependency lock before failing.
# Cache variable so it is visible to consumers calling the public API from their
# own scope (gdm_lock_guard is a macro, so it expands into the caller's scope).
set(GDM_LOCK_TIMEOUT 60 CACHE STRING
    "Seconds to wait for the GDM dependency lock before failing"
)

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


function(gdm_use_or_fetch_package)
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
            "[${PROJECT_NAME}] gdm_use_or_fetch_package requires NAME and VERSION"
        )
    endif()

    # CANDIDATE_TARGETS is optional. It is only needed when ALIAS_TARGET is set,
    # to know which of the resolved targets the alias should point to.
    if (PKG_ALIAS_TARGET AND NOT PKG_CANDIDATE_TARGETS)
        message(FATAL_ERROR
            "[${PROJECT_NAME}] gdm_use_or_fetch_package: ALIAS_TARGET '${PKG_ALIAS_TARGET}' "
            "requires CANDIDATE_TARGETS to resolve which target to alias."
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

    # Download to a staging dir first, then rename into place (atomic like gdm_use_or_fetch_package).
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


# gdm_installed_glew_version(
#   TARGET <imported GLEW target>
# )
#
# Prints which GLEW the found target is:
#
#     [GDM] Found installed GLEW 2.3.1 target: GLEW::glew
#     [GDM] Found installed GLEW 2.3.1 (from headers) target: GLEW::glew
#
# A GLEW config package carries no version file, so find_package cannot answer
# "which version" the way it does for glfw3 or glm. The version is learned from
# the most trustworthy source possible, in order:
#
#   1. the location of the linked library (libGLEW.so.2.3.1, libGLEW.2.3.1.dylib):
#      the artifact that is linked, and the only source that describes it.
#      Silent where the name carries no version: Windows (GLEW32.lib),
#      vcpkg's debug half, and a static libGLEW.a.
#   2. the VERSION block of GL/glew.h, read off the target's own include path:
#      the API compiled against. Silent without dev headers.
#   3. glew.pc's Version:, through pkg-config: the package's own declaration,
#      which points at its own prefix rather than at this target - hence last,
#      and only reached when 1 and 2 both came up empty.
#
# CMake's own FindGLEW is not a source in that list: it reads the same header,
# but only in module mode, because it short-circuits on a config package - which
# exists everywhere this matters.
#
# 1 and 2 are both about this installation, so when both know a version they are
# compared: disagreement means a stale or half-upgraded GLEW, and the direction
# decides the wording. 3 is never compared - a value reached only by 3 has
# nothing about this target to disagree with.
function(gdm_installed_glew_version)
    set(options)
    set(oneValueArgs TARGET)
    cmake_parse_arguments(GV "${options}" "${oneValueArgs}" "" ${ARGN})

    if(NOT GV_TARGET OR NOT TARGET ${GV_TARGET})
        message(FATAL_ERROR
            "[${PROJECT_NAME}] gdm_installed_glew_version requires TARGET <imported target>")
    endif()

    # The configuration whose file this build links: the build type for Make or Ninja,
    # empty on the multi-config generators, which choose it at build time.
    set(_cfg "RELEASE")
    if(CMAKE_BUILD_TYPE)
        string(TOUPPER "${CMAKE_BUILD_TYPE}" _cfg)
    endif()

    # 1. the linked library. Candidates, most specific first: 
    #    no configuration, the one this build links, then release, then debug. 
    #    The last two cover a multi-config configure, and a single-config when only one
    #    build type of GLEW is provided(e.g. Homebrew ships no debug GLEW, only release).
    #    Release first: the debug half is optional. The config part of the property name
    #    is case-exact - RELEASE, never Release. LOCATION is not used:
    #    for an imported target CMake picks the configuration itself,
    #    and on Visual Studio it may pick the debug half.
    set(_lib_props IMPORTED_LOCATION)
    if(CMAKE_BUILD_TYPE)
        list(APPEND _lib_props "IMPORTED_LOCATION_${_cfg}")
    endif()
    list(APPEND _lib_props IMPORTED_LOCATION_RELEASE IMPORTED_LOCATION_DEBUG)
    set(_lib "")
    foreach(_lib_prop IN LISTS _lib_props)
        get_target_property(_lib ${GV_TARGET} ${_lib_prop})
        if(_lib)
            break()
        endif()
    endforeach()
    unset(_lib_props)
    unset(_lib_prop)
    set(_lib_version "")
    if(_lib MATCHES "[0-9]+\\.[0-9]+(\\.[0-9]+)*")
        set(_lib_version "${CMAKE_MATCH_0}")
    endif()

    # 2. the header we use. Always read when present, not only as a fallback:
    #    it is what makes the check at the end possible. GLEW's build
    #    writes its version into GL/glew.h as a "VERSION 2.3.1" line; reading it
    #    here also covers the platforms whose library name has no version.
    set(_header_version "")
    get_target_property(_incs ${GV_TARGET} INTERFACE_INCLUDE_DIRECTORIES)
    foreach(_inc IN LISTS _incs)
        if(NOT _header_version AND EXISTS "${_inc}/GL/glew.h")
            file(STRINGS "${_inc}/GL/glew.h" _glew_version_lines
                 REGEX "^VERSION [0-9]+\\.[0-9]+(\\.[0-9]+)?$")
            if(_glew_version_lines)
                list(GET _glew_version_lines 0 _glew_version_line)
                string(REGEX MATCH "[0-9]+\\.[0-9]+(\\.[0-9]+)*"
                       _header_version "${_glew_version_line}")
            endif()
        endif()
    endforeach()
    unset(_glew_version_line)
    unset(_glew_version_lines)

    # 3. pkg-config, only when 1 and 2 both came up empty. The prefix is
    #    deliberately not GDM_GLEW: pkg_check_modules would set GDM_GLEW_VERSION
    #    in the caller's scope and shadow gdm's own cache option from then on.
    set(_pkgconf_version "")
    if(NOT _lib_version AND NOT _header_version)
        find_package(PkgConfig QUIET)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(GDM_GLEW_PKG QUIET glew)
            if(GDM_GLEW_PKG_VERSION)
                set(_pkgconf_version "${GDM_GLEW_PKG_VERSION}")
            endif()
        endif()
    endif()

    set(_version "")
    set(_from "")
    if(_lib_version)
        set(_version "${_lib_version}")
    elseif(_header_version)
        set(_version "${_header_version}")
        set(_from " (from headers)")
    elseif(_pkgconf_version)
        set(_version "${_pkgconf_version}")
        set(_from " (from pkg-config)")
    endif()

    if(_version)
        message(STATUS "[GDM] Found installed GLEW ${_version}${_from} target: ${GV_TARGET}")
    else()
        # Nothing knows: a Windows GLEW32.lib with no dev headers on the path,
        # or a digitless static name. The message reads as it did before.
        message(STATUS "[GDM] Found installed GLEW target: ${GV_TARGET}")
    endif()

    if(_lib_version AND _header_version AND NOT _lib_version VERSION_EQUAL _header_version)
        if(_header_version VERSION_GREATER _lib_version)
            message(AUTHOR_WARNING
                "[GDM] GLEW headers ${_header_version} are newer than the library linked "
                "(${_lib_version}): entry points declared after ${_lib_version} will not link. "
                "Install a GLEW whose dev package matches its library.")
        else()
            message(AUTHOR_WARNING
                "[GDM] GLEW library ${_lib_version} is newer than its headers ${_header_version}: "
                "this builds, but the newer extensions are unavailable at compile time. "
                "Upgrade the dev package to match.")
        endif()
    endif()
endfunction()
