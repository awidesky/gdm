cmake_minimum_required(VERSION 3.20)

include(FetchContent)

# fetch_glad(
#   OUT_VAR    <var-to-set>
#   NAME       <fetchcontent-name>
#   DEST_DIR   <output-dir>
#   APIS       <list: gl=4.6;egl=1.5;vulkan=1.3;...>
#   PROFILES   <list: gl=core;...>            # optional
#   OPTIONS    <list: DEBUG;MX;...>           # optional
#   EXTENSIONS <list: GL_EXT_xxx;...>         # optional
# )
function(fetch_glad)
  set(oneValueArgs OUT_VAR NAME DEST_DIR)
  set(multiValueArgs APIS PROFILES OPTIONS EXTENSIONS)
  cmake_parse_arguments(FG "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT FG_OUT_VAR)
    message(FATAL_ERROR "[${PROJECT_NAME}] fetch_glad: OUT_VAR is required")
  endif()
  if(NOT FG_NAME)
    message(FATAL_ERROR "[${PROJECT_NAME}] fetch_glad: NAME is required")
  endif()
  if(NOT FG_DEST_DIR)
    message(FATAL_ERROR "[${PROJECT_NAME}] fetch_glad: DEST_DIR is required")
  endif()
  if(NOT FG_APIS)
    message(FATAL_ERROR "[${PROJECT_NAME}] fetch_glad: APIS is required (ex: gl=4.6)")
  endif()

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

  message(STATUS
    "[${PROJECT_NAME}] ${FG_NAME} : ${FG_APIS}, ${_fg_profiles}, ${_fg_options}, Extensions=${FG_EXTENSIONS}"
  )

  unset(_fg_profiles)
  unset(_fg_options)

  find_program(CURL_EXECUTABLE NAMES curl curl.exe REQUIRED)

  string(MAKE_C_IDENTIFIER "${FG_NAME}" _cfg_tag)
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
      file(APPEND "${CURL_CFG_PATH}" "data-urlencode = \"api=${api}\"\n")
    endif()
  endforeach()

  foreach(profile IN LISTS FG_PROFILES)
    if(NOT profile STREQUAL "")
      file(APPEND "${CURL_CFG_PATH}" "data-urlencode = \"profile=${profile}\"\n")
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

  execute_process(
    COMMAND "${CURL_EXECUTABLE}" --config "${CURL_CFG_PATH}"
    OUTPUT_VARIABLE HTTP_OUT
    ERROR_VARIABLE HTTP_ERR
    RESULT_VARIABLE CURL_RES
  )

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
  message(STATUS "[${PROJECT_NAME}] into ${FG_DEST_DIR}")

  FetchContent_Declare(${FG_NAME}
    URL "${zip_url}"
    SOURCE_DIR "${FG_DEST_DIR}"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  FetchContent_MakeAvailable(${FG_NAME})

  set(${FG_OUT_VAR} "${${FG_NAME}_SOURCE_DIR}" PARENT_SCOPE)
endfunction()

# fetch_glad_allEXT(
#   OUT_VAR      <var-to-set>
#   NAME         <fetchcontent-name>
#   DEST_DIR     <output-dir>
#   GLAD_OPTIONS <optional; list like DEBUG;MX;...>
# )
# Fixed:
#   api=gl=4.6
#   profile=gl=core
#   generator=c
function(fetch_glad_allEXT)
  set(oneValueArgs OUT_VAR NAME DEST_DIR)
  set(multiValueArgs GLAD_OPTIONS)
  cmake_parse_arguments(FGAE "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT FGAE_OUT_VAR)
    message(FATAL_ERROR "[${PROJECT_NAME}] fetch_glad_allEXT: OUT_VAR is required")
  endif()
  if(NOT FGAE_NAME)
    message(FATAL_ERROR "[${PROJECT_NAME}] fetch_glad_allEXT: NAME is required")
  endif()
  if(NOT FGAE_DEST_DIR)
    message(FATAL_ERROR "[${PROJECT_NAME}] fetch_glad_allEXT: DEST_DIR is required")
  endif()

  if(FGAE_GLAD_OPTIONS)
    set(_fgae_options "${FGAE_GLAD_OPTIONS}")
  else()
    set(_fgae_options "(none)")
  endif()
  message(STATUS
    "[${PROJECT_NAME}] ${FGAE_NAME} : gl=4.6:core, ${_fgae_options}, Extensions=ALL"
  )
  unset(_fgae_options)

  set(GL_XML_URL "https://cvs.khronos.org/svn/repos/ogl/trunk/doc/registry/public/api/gl.xml")
  set(GL_XML_PATH "${CMAKE_BINARY_DIR}/khronos_gl.xml")

  file(DOWNLOAD
    "${GL_XML_URL}"
    "${GL_XML_PATH}"
    STATUS dl_status
    TLS_VERIFY ON
  )
  list(GET dl_status 0 dl_code)
  if(NOT dl_code EQUAL 0)
    list(GET dl_status 1 dl_msg)
    message(FATAL_ERROR "[${PROJECT_NAME}] fetch_glad_allEXT: Failed to download gl.xml: ${dl_msg}")
  endif()

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
    NAME "${FGAE_NAME}"
    DEST_DIR "${FGAE_DEST_DIR}"
    APIS "gl=4.6"
    PROFILES "gl=core"
    OPTIONS ${FGAE_GLAD_OPTIONS}
    EXTENSIONS ${GL_EXTS}
  )

  set(${FGAE_OUT_VAR} "${_glad_src}" PARENT_SCOPE)
endfunction()