#
# Collection of cmake utility functions
#

# message_colored {{{

function(message_colored message_level text color)
  string(ASCII 27 esc)
  message(${message_level} "${esc}[${color}m${text}${esc}[0m")
endfunction()

# }}}
# colored_option {{{

function(colored_option text flag)
  # Append version of option, if ${flag}_VERSION is set
  set(version ${${flag}_VERSION})

  if(NOT "${version}" STREQUAL "")
    set(text "${text} (${version})")
  endif()

  if(${flag})
    message_colored(STATUS "[X]${text}" "32;1")
  else()
    message_colored(STATUS "[ ]${text}" "37;2")
  endif()
endfunction()

# }}}

# queryfont {{{

function(queryfont output_variable fontname)
  set(multi_value_args FIELDS)
  cmake_parse_arguments(ARG "" "" "${multi_value_args}" ${ARGN})

  find_program(BIN_FCLIST fc-list)
  if(NOT BIN_FCLIST)
    message_colored(WARNING "Failed to locate `fc-list`" "33;1")
    return()
  endif()

  string(REPLACE ";" " " FIELDS "${ARG_FIELDS}")
  if(NOT FIELDS)
    set(FIELDS family)
  endif()

  execute_process(
    COMMAND sh -c "${BIN_FCLIST} : ${FIELDS}"
    RESULT_VARIABLE status
    OUTPUT_VARIABLE output
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
  STRING(REGEX REPLACE ";" "\\\\;" output "${output}")
  STRING(REGEX REPLACE "\n" ";" output "${output}")
  STRING(TOLOWER "${output}" output)

  foreach(match LISTS ${output})
    if(${match} MATCHES ".*${fontname}.*$")
      list(APPEND matches ${match})
    endif()
  endforeach()

  if(matches)
    list(GET matches 0 fst_match)
    set(${output_variable} "${fst_match}" PARENT_SCOPE)
    message(STATUS "Found font: ${fst_match}")
  else()
    message_colored(STATUS "Font not found: ${fontname}" "33;1")
  endif()
endfunction()

# checklib {{{

function(checklib flag type pkg)
  if(NOT DEFINED ${flag})
    if(${type} STREQUAL "cmake")
      find_package(${pkg} QUIET)
      set(${flag} ${${pkg}_FOUND} CACHE BOOL "")
    elseif(${type} STREQUAL "pkg-config")
      find_package(PkgConfig REQUIRED)
      pkg_check_modules(PKG_${flag} QUIET ${pkg})
      set(${flag} ${PKG_${flag}_FOUND} CACHE BOOL "")
    elseif(${type} STREQUAL "binary")
      find_program(BIN_${flag} ${pkg})
      set(${flag} ${BIN_${flag}} CACHE BOOL "")
    else()
      message(FATAL_ERROR "Invalid lookup type '${type}'")
    endif()
  endif()
endfunction()

# }}}
