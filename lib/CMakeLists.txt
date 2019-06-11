#
# Configure libs
#

# Library: concurrentqueue {{{

add_library(moodycamel INTERFACE)
target_include_directories(moodycamel SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/concurrentqueue/include)

# }}}
# Library: xpp {{{

set(XCB_PROTOS xproto)

if(WITH_XRANDR)
  list(APPEND XCB_PROTOS randr)
endif()
if(WITH_XCOMPOSITE)
  list(APPEND XCB_PROTOS composite)
endif()
if(WITH_XKB)
  list(APPEND XCB_PROTOS xkb)
endif()

add_subdirectory(xpp)

# }}}
# Library: i3ipcpp {{{

if(ENABLE_I3)
  add_subdirectory(i3ipcpp)
  set_property(TARGET i3ipc++ PROPERTY POSITION_INDEPENDENT_CODE ON)
  if(TARGET jsoncpp_lib_static)
    set_property(TARGET jsoncpp_lib_static PROPERTY POSITION_INDEPENDENT_CODE ON)
  endif()
endif()

# }}}

set(libs ${libs} PARENT_SCOPE)
