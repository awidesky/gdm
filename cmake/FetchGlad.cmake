cmake_minimum_required(VERSION 3.20)

include(FetchContent)
# fetch_glad(
#   OUT_VAR    <var-to-set>             -- receives the output directory path
#   BASE_DIR   <parent directory>       -- e.g. external/; output is BASE_DIR/glad-<config>
#   [DEST_DIR  <explicit dir>]          -- if set, use this dir directly (skip CONFIG logic)
#   LANG       <api language>           -- e.g. "gl", "egl", "vulkan" (used as prefix)
#   APIS       <list: 4.6;3.3;...>      -- version strings (without lang= prefix)
#   PROFILES   <list: core;compatible>  -- profile strings (without lang= prefix)
#   OPTIONS    <list: DEBUG;MX;...>           # optional
#   EXTENSIONS <list: GL_EXT_xxx;...>         # optional
# )
#
# The output directory is: ${BASE_DIR}/glad-<LANG><APIS><PROFILES>[_<OPTIONS>]
# A CONFIG file is written inside recording all parameters.  If the directory
# already exists but its CONFIG differs, a numeric suffix (_2, _3, …) is
# appended so that different configurations always use distinct directories.
function(fetch_glad)
  set(oneValueArgs OUT_VAR BASE_DIR DEST_DIR LANG)
  set(multiValueArgs APIS PROFILES OPTIONS EXTENSIONS)
  cmake_parse_arguments(FG "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT FG_OUT_VAR)
    message(FATAL_ERROR "[${PROJECT_NAME}] fetch_glad: OUT_VAR is required")
  endif()
  if(NOT FG_LANG)
    message(FATAL_ERROR "[${PROJECT_NAME}] fetch_glad: LANG is required (ex: gl)")
  endif()
  if(NOT FG_APIS)
    message(FATAL_ERROR "[${PROJECT_NAME}] fetch_glad: APIS is required (ex: 4.6)")
  endif()

  # Build a canonical config string for the CONFIG file and for comparison.
  # Use CONCAT to avoid trailing newline issues on older CMake.
  string(CONCAT _fg_config_string
    "LANG=${FG_LANG}\n"
    "APIS=${FG_APIS}\n"
    "PROFILES=${FG_PROFILES}\n"
    "OPTIONS=${FG_OPTIONS}\n"
    "EXTENSIONS=${FG_EXTENSIONS}\n"
  )

  # Build the human-readable config suffix used in the folder name.
  # It shows only partial of glad config. Full config info will saved CONFIG file in glad folder.
  set(_fg_suffix "")
  foreach(api IN LISTS FG_APIS)
    string(APPEND _fg_suffix "${FG_LANG}${api}")
  endforeach()
  foreach(profile IN LISTS FG_PROFILES)
    string(APPEND _fg_suffix "${profile}")
  endforeach()
  if(FG_OPTIONS)
    string(REPLACE ";" "_" _fg_opt_str "${FG_OPTIONS}")
    string(APPEND _fg_suffix "_${_fg_opt_str}")
  endif()

  # The folder name must be less than (around) 200 chars.
  # Truncate the suffix if it's too long; we keep the first 190 chars and
  # append a hash of the tail so different configs still map to different folders.
  string(LENGTH "${_fg_suffix}" _fg_suffix_len)
  if(_fg_suffix_len GREATER 190)
    string(SUBSTRING "${_fg_suffix}" 0 190 _fg_suffix_trunc)
    string(SUBSTRING "${_fg_suffix}" 190 -1 _fg_suffix_tail)
    string(MD5 _fg_tail_hash "${_fg_suffix_tail}")
    string(SUBSTRING "${_fg_tail_hash}" 0 8 _fg_tail_hash_short)
    set(_fg_suffix "${_fg_suffix_trunc}_${_fg_tail_hash_short}")
    message(STATUS "[${PROJECT_NAME}] fetch_glad: folder name too long; truncated to about 200 chars")
  endif()

  # Decide output directory
  if(FG_DEST_DIR)
    # Trust pre-selected directory(used in fetch_glad_allEXT).
    set(_fg_out_dir "${FG_DEST_DIR}")
  else()
    if(NOT FG_BASE_DIR)
      message(FATAL_ERROR "[${PROJECT_NAME}] fetch_glad: BASE_DIR required when DEST_DIR is not given")
    endif()

    set(_fg_base_dir "${FG_BASE_DIR}/glad-${_fg_suffix}")

    # Find a directory slot:
    #   a) does not exist yet --> use it to download glad
    #   b) exists and CONFIG matches --> reuse extisting glad installation, return early
    #   c) exists but CONFIG differs --> try _2, _3, ...
    set(_fg_out_dir "${_fg_base_dir}")
    set(_fg_suffix_num 0)
    while(EXISTS "${_fg_out_dir}")
      if(EXISTS "${_fg_out_dir}/CONFIG")
        file(READ "${_fg_out_dir}/CONFIG" _fg_existing_config)
        normalizeCheckStr("${_fg_existing_config}" "${_fg_config_string}" _same)
        if(_same)
          file(RELATIVE_PATH _fg_rel_dir "${FG_BASE_DIR}" "${_fg_out_dir}")
          message(STATUS "[${PROJECT_NAME}] Found installed ${_fg_rel_dir}")
          set(${FG_OUT_VAR} "${_fg_out_dir}" PARENT_SCOPE)
          return()
        endif()
      endif()
      math(EXPR _fg_suffix_num "${_fg_suffix_num} + 1")
      set(_fg_out_dir "${_fg_base_dir}_${_fg_suffix_num}")
    endwhile()
  endif()

  # log
  if(FG_PROFILES)
    set(_fg_profiles "${FG_PROFILES}")
  else()
    set(_fg_profiles "(default)")
  endif()
  if(FG_OPTIONS)
    set(_fg_options "${FG_OPTIONS}")
  else()
    set(_fg_options "(no option)")
  endif()

  list(LENGTH FG_EXTENSIONS _fg_ext_len)
  if(_fg_ext_len GREATER 3)
      set(_fg_ext_print "${_fg_ext_len} extensions...")
  else()
      set(_fg_ext_print "${FG_EXTENSIONS}")
  endif()
  message(STATUS
      "[${PROJECT_NAME}] glad-${_fg_suffix} : ${FG_LANG}=${FG_APIS}, ${_fg_profiles}, ${_fg_options}, Extensions=${_fg_ext_print}"
  )

  unset(_fg_profiles)
  unset(_fg_options)

  find_program(CURL_EXECUTABLE NAMES curl curl.exe REQUIRED)

  string(MAKE_C_IDENTIFIER "${_fg_suffix}" _cfg_tag)
  set(CURL_CFG_PATH "${CMAKE_BINARY_DIR}/fetch_glad_${_cfg_tag}.curl")

  file(WRITE "${CURL_CFG_PATH}" "")
  file(APPEND "${CURL_CFG_PATH}" "silent\n")
  file(APPEND "${CURL_CFG_PATH}" "show-error\n")
  file(APPEND "${CURL_CFG_PATH}" "dump-header = -\n")
  file(APPEND "${CURL_CFG_PATH}" "request = POST\n")
  file(APPEND "${CURL_CFG_PATH}" "url = \"https://gen.glad.sh/generate\"\n")
  file(APPEND "${CURL_CFG_PATH}" "data-urlencode = \"generator=c\"\n")

  foreach(api IN LISTS FG_APIS)
    if(NOT api STREQUAL "")
      file(APPEND "${CURL_CFG_PATH}" "data-urlencode = \"api=${FG_LANG}=${api}\"\n")
    endif()
  endforeach()

  foreach(profile IN LISTS FG_PROFILES)
    if(NOT profile STREQUAL "")
      file(APPEND "${CURL_CFG_PATH}" "data-urlencode = \"profile=${FG_LANG}=${profile}\"\n")
    endif()
  endforeach()

  foreach(opt IN LISTS FG_OPTIONS)
    if(NOT opt STREQUAL "")
      file(APPEND "${CURL_CFG_PATH}" "data-urlencode = \"options=${opt}\"\n")
    endif()
  endforeach()

  foreach(ext IN LISTS FG_EXTENSIONS)
    if(NOT ext STREQUAL "")
      file(APPEND "${CURL_CFG_PATH}" "data-urlencode = \"extensions=${ext}\"\n")
    endif()
  endforeach()

  # Time the curl POST request to gen.glad.sh
  string(TIMESTAMP _start_time "%s" UTC)

  execute_process(
    COMMAND "${CURL_EXECUTABLE}" --config "${CURL_CFG_PATH}"
    OUTPUT_VARIABLE HTTP_OUT
    ERROR_VARIABLE HTTP_ERR
    RESULT_VARIABLE CURL_RES
  )

  string(TIMESTAMP _end_time "%s" UTC)
  math(EXPR _elapsed_time "${_end_time} - ${_start_time}")
  message(STATUS "[${PROJECT_NAME}] curl POST to gen.glad.sh completed in ${_elapsed_time}s")

  if(NOT CURL_RES EQUAL 0)
    message(FATAL_ERROR
      "[${PROJECT_NAME}] fetch_glad: curl POST to gen.glad.sh failed (code ${CURL_RES}).\n"
      "curl stderr:\n${HTTP_ERR}\n"
      "curl/stdout(response):\n${HTTP_OUT}\n"
      "curl config path:\n${CURL_CFG_PATH}"
    )
  endif()

  string(REGEX MATCH "[Ll][Oo][Cc][Aa][Tt][Ii][Oo][Nn]:[ \t]*([^\r\n]+)" _loc "${HTTP_OUT}")
  set(location "${CMAKE_MATCH_1}")

  if(NOT location OR location STREQUAL "/")
    message(FATAL_ERROR
      "[${PROJECT_NAME}] fetch_glad: Could not extract Location header from gen.glad.sh response.\n"
      "curl/stdout(response):\n${HTTP_OUT}\n"
      "curl stderr:\n${HTTP_ERR}\n"
      "curl config path:\n${CURL_CFG_PATH}"
    )
  endif()

  string(STRIP "${location}" location)
  string(REGEX REPLACE "/$" "" location "${location}")

  set(zip_url "${location}/glad.zip")
  if(NOT zip_url MATCHES "^http")
    set(zip_url "https://gen.glad.sh${zip_url}")
  endif()

  message(STATUS "[${PROJECT_NAME}] Downloading ${zip_url}")
  message(STATUS "[${PROJECT_NAME}]   into ${_fg_out_dir}")

  # Download
  set(_zip_path "${CMAKE_BINARY_DIR}/_gdm_downloads/${_cfg_tag}_glad.zip")
  string(TIMESTAMP _fetch_start_time "%s" UTC)
  file(DOWNLOAD "${zip_url}" "${_zip_path}"
    STATUS _dl_status
    TLS_VERIFY ${GDM_TLS_VERIFY}
  )
  list(GET _dl_status 0 _dl_code)
  if(NOT _dl_code EQUAL 0)
    file(REMOVE "${_zip_path}")
    list(GET _dl_status 1 _dl_msg)
    message(FATAL_ERROR "[${PROJECT_NAME}] fetch_glad: Failed to download glad.zip: ${_dl_msg}\nURL=${zip_url}")
  endif()

  # Extract
  file(MAKE_DIRECTORY "${_fg_out_dir}")
  file(ARCHIVE_EXTRACT
    INPUT "${_zip_path}"
    DESTINATION "${_fg_out_dir}"
  )

  string(TIMESTAMP _fetch_end_time "%s" UTC)
  math(EXPR _fetch_elapsed_time "${_fetch_end_time} - ${_fetch_start_time}")
  file(WRITE "${_fg_out_dir}/CONFIG" "${_fg_config_string}")
  message(STATUS "[${PROJECT_NAME}] DOWNLOAD/EXTRACT for glad-${_fg_suffix} completed in ${_fetch_elapsed_time}s")

  set(${FG_OUT_VAR} "${_fg_out_dir}" PARENT_SCOPE)
endfunction()

# fetch_glad_allEXT(
#   OUT_VAR      <var-to-set>        -- receives the output directory path
#   BASE_DIR     <parent directory>  -- output -> BASE_DIR/glad-<config>_allExt
#   GLAD_OPTIONS <optional; list like STATUS;MX;...>
# )
# Fixed:
#   api=gl=4.6
#   profile=gl=core
#   generator=c
#
# The output directory is ${BASE_DIR}/glad-gl4.6core_allExt[_<OPTIONS>]
# with CONFIG matching like fetch_glad().
# The extension list from gl.xml is NOT included in the config, and saved as EXTENSIONS=ALLEXT.
# So even after gl.xml is updated in remote registry, the downloaded glad will reused.
# Delete the glad directory manually to force a fresh fetch.
function(fetch_glad_allEXT)
  set(oneValueArgs OUT_VAR BASE_DIR)
  set(multiValueArgs GLAD_OPTIONS)
  cmake_parse_arguments(FGAE "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT FGAE_OUT_VAR)
    message(FATAL_ERROR "[${PROJECT_NAME}] fetch_glad_allEXT: OUT_VAR is required")
  endif()
  if(NOT FGAE_BASE_DIR)
    message(FATAL_ERROR "[${PROJECT_NAME}] fetch_glad_allEXT: BASE_DIR is required")
  endif()

  # Build config string (extensions excluded — they are dynamic)
  string(CONCAT _fg_config_string
    "LANG=gl\n"
    "APIS=4.6\n"
    "PROFILES=core\n"
    "OPTIONS=${FGAE_GLAD_OPTIONS}\n"
    "EXTENSIONS=ALLEXT\n"
  )

  # Build a human-readable base name
  set(_fg_suffix "gl4.6core_allExt")
  if(FGAE_GLAD_OPTIONS)
    string(REPLACE ";" "_" _fg_opt_str "${FGAE_GLAD_OPTIONS}")
    string(APPEND _fg_suffix "_${_fg_opt_str}")
  endif()

  # The folder name must be less than (around) 200 chars.
  # Truncate the suffix if it's too long; we keep the first 190 chars and
  # append a hash of the tail so different configs still map to different folders.
  string(LENGTH "${_fg_suffix}" _fg_suffix_len)
  if(_fg_suffix_len GREATER 190)
    string(SUBSTRING "${_fg_suffix}" 0 190 _fg_suffix_trunc)
    string(SUBSTRING "${_fg_suffix}" 190 -1 _fg_suffix_tail)
    string(MD5 _fg_tail_hash "${_fg_suffix_tail}")
    string(SUBSTRING "${_fg_tail_hash}" 0 8 _fg_tail_hash_short)
    set(_fg_suffix "${_fg_suffix_trunc}_${_fg_tail_hash_short}")
    message(STATUS "[${PROJECT_NAME}] fetch_glad_allEXT: folder name too long; truncated to about 200 chars")
  endif()
  set(_fg_base_dir "${FGAE_BASE_DIR}/glad-${_fg_suffix}")

  # Find a directory slot (same logic as fetch_glad)
  set(_fg_out_dir "${_fg_base_dir}")
  set(_fg_suffix_num 0)
  while(EXISTS "${_fg_out_dir}")
    if(EXISTS "${_fg_out_dir}/CONFIG")
      file(READ "${_fg_out_dir}/CONFIG" _fg_existing_config)
      normalizeCheckStr("${_fg_existing_config}" "${_fg_config_string}" _same)
      if(_same)
        file(RELATIVE_PATH _fg_rel_dir "${FGAE_BASE_DIR}" "${_fg_out_dir}")
        message(STATUS "[${PROJECT_NAME}] Found installed ${_fg_rel_dir}")
        set(${FGAE_OUT_VAR} "${_fg_out_dir}" PARENT_SCOPE)
        return()
      endif()
    endif()
    math(EXPR _fg_suffix_num "${_fg_suffix_num} + 1")
    set(_fg_out_dir "${_fg_base_dir}_${_fg_suffix_num}")
  endwhile()

  set(GL_XML_URL "https://cvs.khronos.org/svn/repos/ogl/trunk/doc/registry/public/api/gl.xml")
  set(GL_XML_PATH "${CMAKE_BINARY_DIR}/_gdm_downloads/khronos_gl.xml")

  # Time the download of gl.xml from Khronos
  string(TIMESTAMP _xml_start_time "%s" UTC)

  file(DOWNLOAD
    "${GL_XML_URL}"
    "${GL_XML_PATH}"
    STATUS dl_status
    TLS_VERIFY ${GDM_TLS_VERIFY}
  )
  list(GET dl_status 0 dl_code)
  if(NOT dl_code EQUAL 0)
    file(REMOVE "${GL_XML_PATH}")
    list(GET dl_status 1 dl_msg)
    message(FATAL_ERROR "[${PROJECT_NAME}] fetch_glad_allEXT: Failed to download gl.xml: ${dl_msg}")
  endif()

  string(TIMESTAMP _xml_end_time "%s" UTC)
  math(EXPR _xml_elapsed_time "${_xml_end_time} - ${_xml_start_time}")
  message(STATUS "[${PROJECT_NAME}] gl.xml download completed in ${_xml_elapsed_time}s")

  file(READ "${GL_XML_PATH}" GL_XML_TEXT)
  string(REGEX MATCHALL "<extension[^>]*>" EXT_TAGS "${GL_XML_TEXT}")

  set(GL_EXTS "")
  foreach(tag IN LISTS EXT_TAGS)
    if(tag MATCHES "supported=\"[^\"]*gl[^\"]*\"" AND tag MATCHES "name=\"([^\"]+)\"")
      list(APPEND GL_EXTS "${CMAKE_MATCH_1}")
    endif()
  endforeach()

  list(REMOVE_DUPLICATES GL_EXTS)
  list(SORT GL_EXTS)
  list(LENGTH GL_EXTS GL_EXTS_LEN)

  if(GL_EXTS_LEN EQUAL 0)
    message(FATAL_ERROR "[${PROJECT_NAME}] Parsed 0 GL extensions from gl.xml.")
  endif()

  message(STATUS "[${PROJECT_NAME}] Parsed ${GL_EXTS_LEN} GL extensions from Khronos registry")

  fetch_glad(
    OUT_VAR _glad_src
    DEST_DIR "${_fg_out_dir}"
    LANG "gl"
    APIS "4.6"
    PROFILES "core"
    OPTIONS ${FGAE_GLAD_OPTIONS}
    EXTENSIONS ${GL_EXTS}
  )

  # Overwrite CONFIG with our own (which marks EXTENSIONS=ALLEXT instead of
  # the actual parsed list, so the directory is stable across gl.xml updates).
  # TODO_think this will not update when new extension is added in OpenGL registry.
  # but fetchting 2.5MB of xml & checking 13KB of string each configuaring would be waste of time.
  file(WRITE "${_fg_out_dir}/CONFIG" "${_fg_config_string}")

  set(${FGAE_OUT_VAR} "${_glad_src}" PARENT_SCOPE)
endfunction()

# Compare string after normalizing whitespaces like line ending
function(normalizeCheckStr STR1 STR2 OUT_VAR)
  set(_str1 "${STR1}")
  set(_str2 "${STR2}")

  # Normalize line endings: CRLF/CR -> LF
  string(REPLACE "\r\n" "\n" _str1 "${_str1}")
  string(REPLACE "\r"   "\n" _str1 "${_str1}")
  string(REPLACE "\r\n" "\n" _str2 "${_str2}")
  string(REPLACE "\r"   "\n" _str2 "${_str2}")

  # Remove trailing whitespace
  string(REGEX REPLACE "[[:space:]]+$" "" _str1 "${_str1}")
  string(REGEX REPLACE "[[:space:]]+$" "" _str2 "${_str2}")

  if("${_str1}" STREQUAL "${_str2}")
    set(${OUT_VAR} TRUE PARENT_SCOPE)
  else()
    set(${OUT_VAR} FALSE PARENT_SCOPE)
  endif()
endfunction()
