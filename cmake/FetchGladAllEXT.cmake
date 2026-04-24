cmake_minimum_required(VERSION 3.20)

include(FetchContent)

# fetch_glad_allEXT(
#   OUT_VAR <var-to-set>
#   GL_VERSION <4.6|3.3|...>
#   GL_PROFILE <core|compatibility>
#   GLAD_OPTIONS <optional; list like DEBUG;MX;...>
# )
#
# What it does (high-level):
#   1) Downloads Khronos GL registry XML (gl.xml), extracts all extension names
#   2) Calls https://gen.glad.sh/generate (generator fixed to "c") with:
#        - api=gl=<GL_VERSION>
#        - profile=gl=<GL_PROFILE>
#        - extensions=<every extension name>   (repeated field; one per extension)
#        - options=<GLAD_OPTIONS>              (repeated field; one per option)
#      Then parses the HTTP "Location:" header
#   3) Downloads the resulting glad.zip via FetchContent and returns its SOURCE_DIR
#
# Notes:
#   - This mirrors the approach used by https://github.com/cmmw/imgui-glfw-glad-glm
#     (POST -> parse Location -> FetchContent URL download).
#   - Passing "all extensions" can be very large; on some systems command-line length
#     limits may be hit. If that happens, the robust solution is local Python generation.

function(fetch_glad_allEXT)
  set(oneValueArgs OUT_VAR GL_VERSION GL_PROFILE)
  set(multiValueArgs GLAD_OPTIONS)
  cmake_parse_arguments(FGAE "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT FGAE_OUT_VAR)
    message(FATAL_ERROR "fetch_glad_allEXT: OUT_VAR is required")
  endif()

  if(NOT FGAE_GL_VERSION)
    message(FATAL_ERROR "fetch_glad_allEXT: GL_VERSION is required (e.g. 4.6)")
  endif()

  if(NOT FGAE_GL_PROFILE)
    message(FATAL_ERROR "fetch_glad_allEXT: GL_PROFILE is required (core or compatibility)")
  endif()

  if(NOT (FGAE_GL_PROFILE STREQUAL "core" OR FGAE_GL_PROFILE STREQUAL "compatibility"))
    message(FATAL_ERROR "fetch_glad_allEXT: GL_PROFILE must be 'core' or 'compatibility' (got '${FGAE_GL_PROFILE}')")
  endif()

  # Generator is fixed to C.
  set(GLAD_GENERATOR "c")

  find_program(CURL_EXECUTABLE NAMES curl curl.exe REQUIRED)

  # Download Khronos GL registry XML (gl.xml)
  set(GL_XML_URL  "https://cvs.khronos.org/svn/repos/ogl/trunk/doc/registry/public/api/gl.xml")
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
    message(FATAL_ERROR "fetch_glad_allEXT: Failed to download gl.xml: ${dl_msg}")
  endif()

  # Parse "all GL extensions" from gl.xml
  file(READ "${GL_XML_PATH}" GL_XML_TEXT)

  # Grab opening tags like: <extension name="GL_ARB_..." supported="gl|glcore" ...>
  string(REGEX MATCHALL "<extension[^>]*>" EXT_TAGS "${GL_XML_TEXT}")

  set(GL_EXTS "")
  foreach(tag IN LISTS EXT_TAGS)
    # Only keep extensions that claim they support GL (not just gles, etc.)
    if(tag MATCHES "supported=\"[^\"]*gl[^\"]*\"")
      # Extract extension name="..."
      if(tag MATCHES "name=\"([^\"]+)\"")
        list(APPEND GL_EXTS "${CMAKE_MATCH_1}")
      endif()
    endif()
  endforeach()

  list(REMOVE_DUPLICATES GL_EXTS)
  list(SORT GL_EXTS)
  list(LENGTH GL_EXTS GL_EXTS_LEN)

  if(GL_EXTS_LEN EQUAL 0)
    message(FATAL_ERROR "fetch_glad_allEXT: Parsed 0 GL extensions from gl.xml (unexpected).")
  endif()

  message(STATUS "fetch_glad_allEXT: Parsed ${GL_EXTS_LEN} GL extensions from Khronos registry")

  # Build curl config file (avoid command-line length limits)
  set(CURL_CFG_PATH "${CMAKE_BINARY_DIR}/fetch_glad_allext_${FGAE_GL_VERSION}_${FGAE_GL_PROFILE}.curl")

  file(WRITE "${CURL_CFG_PATH}" "")
  file(APPEND "${CURL_CFG_PATH}" "silent\n")
  file(APPEND "${CURL_CFG_PATH}" "show-error\n")
  file(APPEND "${CURL_CFG_PATH}" "dump-header = -\n")
  file(APPEND "${CURL_CFG_PATH}" "request = POST\n")
  file(APPEND "${CURL_CFG_PATH}" "url = \"https://gen.glad.sh/generate\"\n")

  file(APPEND "${CURL_CFG_PATH}" "data-urlencode = \"generator=${GLAD_GENERATOR}\"\n")
  file(APPEND "${CURL_CFG_PATH}" "data-urlencode = \"api=gl=${FGAE_GL_VERSION}\"\n")
  file(APPEND "${CURL_CFG_PATH}" "data-urlencode = \"profile=gl=${FGAE_GL_PROFILE}\"\n")

  # Explicitly disable other APIs
  file(APPEND "${CURL_CFG_PATH}" "data-urlencode = \"api=egl=none\"\n")
  file(APPEND "${CURL_CFG_PATH}" "data-urlencode = \"api=gles1=none\"\n")
  file(APPEND "${CURL_CFG_PATH}" "data-urlencode = \"api=gles2=none\"\n")
  file(APPEND "${CURL_CFG_PATH}" "data-urlencode = \"api=glx=none\"\n")
  file(APPEND "${CURL_CFG_PATH}" "data-urlencode = \"api=wgl=none\"\n")

  # Add glad options
  foreach(opt IN LISTS FGAE_GLAD_OPTIONS)
    if(NOT opt STREQUAL "")
      file(APPEND "${CURL_CFG_PATH}" "data-urlencode = \"options=${opt}\"\n")
    endif()
  endforeach()

  # Add all extensions (one repeated field per extension)
  foreach(ext IN LISTS GL_EXTS)
    file(APPEND "${CURL_CFG_PATH}" "data-urlencode = \"extensions=${ext}\"\n")
  endforeach()

  # POST to gen.glad.sh and parse the redirect Location header
  execute_process(
    COMMAND "${CURL_EXECUTABLE}" --config "${CURL_CFG_PATH}"
    OUTPUT_VARIABLE HTTP_OUT
    ERROR_VARIABLE HTTP_ERR
    RESULT_VARIABLE CURL_RES
  )

  if(NOT CURL_RES EQUAL 0)
    message(FATAL_ERROR
      "fetch_glad_allEXT: curl POST to gen.glad.sh failed (code ${CURL_RES}).\n"
      "curl stderr:\n${HTTP_ERR}\n"
      "curl/stdout(response):\n${HTTP_OUT}\n"
      "curl config path:\n${CURL_CFG_PATH}"
    )
  endif()

  # Extract Location header (case-insensitive match)
  string(REGEX MATCH "[Ll][Oo][Cc][Aa][Tt][Ii][Oo][Nn]:[ \t]*([^\r\n]+)" _loc "${HTTP_OUT}")
  set(location "${CMAKE_MATCH_1}")

  if(NOT location OR location STREQUAL "/")
    message(FATAL_ERROR
      "fetch_glad_allEXT: Could not extract Location header from gen.glad.sh response.\n"
      "curl/stdout(response):\n${HTTP_OUT}\n"
      "curl stderr:\n${HTTP_ERR}"
    )
  endif()

  # Normalize and build zip URL
  string(STRIP "${location}" location)
  string(REGEX REPLACE "/$" "" location "${location}")

  set(zip_url "${location}/glad.zip")
  if(NOT zip_url MATCHES "^http")
    set(zip_url "https://gen.glad.sh${zip_url}")
  endif()

  message(STATUS "fetch_glad_allEXT: Downloading glad.zip from ${zip_url}")

  # Download/unpack glad.zip using FetchContent
  string(REPLACE "." "_" _ver "${FGAE_GL_VERSION}")
  set(_fc_name "glad_gl${_ver}_${FGAE_GL_PROFILE}_allEXT")

  if(NOT DEFINED GDM_ROOT_DIR OR GDM_ROOT_DIR STREQUAL "")
    message(FATAL_ERROR "fetch_glad_allEXT: GDM_ROOT_DIR is not defined.")
  endif()

  set(_dest_dir "${GDM_ROOT_DIR}/external/glad_${FGAE_GL_VERSION}_ALLEXT")

  FetchContent_Declare(${_fc_name}
    URL "${zip_url}"
    SOURCE_DIR "${_dest_dir}"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  FetchContent_MakeAvailable(${_fc_name})

  # Return to caller
  set(${FGAE_OUT_VAR} "${${_fc_name}_SOURCE_DIR}" PARENT_SCOPE)
endfunction()