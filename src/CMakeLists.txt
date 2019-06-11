#
# Configure src
#

get_filename_component(include_dir ${CMAKE_SOURCE_DIR}/include ABSOLUTE)
get_filename_component(generated_includes ${CMAKE_BINARY_DIR}/generated-sources/ ABSOLUTE)
get_filename_component(src_dir ${CMAKE_CURRENT_SOURCE_DIR} ABSOLUTE)

get_filename_component(adapters_dir ${src_dir}/adapters ABSOLUTE)
get_filename_component(components_dir ${src_dir}/components ABSOLUTE)
get_filename_component(cairo_dir ${src_dir}/cairo ABSOLUTE)
get_filename_component(drawtypes_dir ${src_dir}/drawtypes ABSOLUTE)
get_filename_component(events_dir ${src_dir}/events ABSOLUTE)
get_filename_component(modules_dir ${src_dir}/modules ABSOLUTE)
get_filename_component(utils_dir ${src_dir}/utils ABSOLUTE)
get_filename_component(x11_dir ${src_dir}/x11 ABSOLUTE)

if (WITH_XCOMPOSITE)
  add_library(poly_composite_extension STATIC ${x11_dir}/extensions/composite.cpp)
  set_target_properties(poly_composite_extension PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
  target_include_directories(poly_composite_extension PUBLIC ${include_dir} ${generated_includes})
  target_link_libraries(poly_composite_extension PUBLIC xpp Xcb::COMPOSITE)
endif()

add_library(poly_xrandr_extension STATIC ${x11_dir}/extensions/randr.cpp)
set_target_properties(poly_xrandr_extension PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_include_directories(poly_xrandr_extension PUBLIC ${include_dir} ${generated_includes})
target_link_libraries(poly_xrandr_extension PUBLIC xpp Xcb::RANDR)

if (WITH_XRM)
  add_library(poly_xrm_extension STATIC ${x11_dir}/xresources.cpp)
  set_target_properties(poly_xrm_extension PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
  target_include_directories(poly_xrm_extension PUBLIC ${include_dir} ${generated_includes})
  target_link_libraries(poly_xrm_extension PUBLIC xpp Xcb::XRM)
endif()

if (WITH_XCURSOR)
  add_library(poly_cursor_extension STATIC ${x11_dir}/cursor.cpp)
  set_target_properties(poly_cursor_extension PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
  target_include_directories(poly_cursor_extension PUBLIC ${include_dir} ${generated_includes})
  target_link_libraries(poly_cursor_extension PUBLIC xpp Xcb::CURSOR)
endif()

add_library(poly STATIC
  ${cairo_dir}/utils.cpp

  ${components_dir}/bar.cpp
  ${components_dir}/builder.cpp
  ${components_dir}/command_line.cpp
  ${components_dir}/config.cpp
  ${components_dir}/controller.cpp
  ${components_dir}/ipc.cpp
  ${components_dir}/logger.cpp
  ${components_dir}/parser.cpp
  ${components_dir}/renderer.cpp
  ${components_dir}/screen.cpp
  ${components_dir}/taskqueue.cpp

  ${drawtypes_dir}/animation.cpp
  ${drawtypes_dir}/iconset.cpp
  ${drawtypes_dir}/label.cpp
  ${drawtypes_dir}/progressbar.cpp
  ${drawtypes_dir}/ramp.cpp

  ${events_dir}/signal_emitter.cpp
  ${events_dir}/signal_receiver.cpp

  ${modules_dir}/meta/base.cpp

  ${utils_dir}/bspwm.cpp
  ${utils_dir}/command.cpp
  ${utils_dir}/concurrency.cpp
  ${utils_dir}/env.cpp
  ${utils_dir}/factory.cpp
  ${utils_dir}/file.cpp
  ${utils_dir}/http.cpp
  ${utils_dir}/i3.cpp
  ${utils_dir}/inotify.cpp
  ${utils_dir}/io.cpp
  ${utils_dir}/process.cpp
  ${utils_dir}/socket.cpp
  ${utils_dir}/string.cpp
  ${utils_dir}/throttle.cpp

  ${x11_dir}/atoms.cpp
  ${x11_dir}/background_manager.cpp
  ${x11_dir}/connection.cpp
  #${x11_dir}/cursor.cpp # Should not be in the main list
  ${x11_dir}/ewmh.cpp
  ${x11_dir}/icccm.cpp
  ${x11_dir}/registry.cpp
  ${x11_dir}/tray_client.cpp
  ${x11_dir}/tray_manager.cpp
  ${x11_dir}/window.cpp
  ${x11_dir}/winspec.cpp
  ${x11_dir}/xembed.cpp
  #${x11_dir}/xresources.cpp # Should not be in the main list
)

target_include_directories(poly PUBLIC ${include_dir} ${generated_includes})
set_target_properties(poly PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_compile_options(poly PUBLIC $<$<CXX_COMPILER_ID:GNU>:$<$<CONFIG:MinSizeRel>:-flto>>)
target_link_libraries(poly PUBLIC xpp moodycamel
  CairoFc::CairoFc i3ipc++ Threads::Threads
  poly_xrandr_extension $<$<TARGET_EXISTS:poly_composite_extension>:poly_composite_extension>
  $<$<TARGET_EXISTS:poly_xrm_extension>:poly_xrm_extension> $<$<TARGET_EXISTS:poly_cursor_extension>:poly_cursor_extension>
)

set(POLYBAR_MODULES
  alsa
  backlight
  battery
  bspwm
  counter
  cpu
  date
  fs
  github
  i3
  ipc
  memory
  menu
  mpd
  network
  pulseaudio
  script
  systray
  temperature
  text
  xbacklight
  xkeyboard
  xwindow
  xworkspaces)

if (ENABLE_ALSA)
  add_library(poly_alsa_module SHARED ${modules_dir}/alsa.cpp ${adapters_dir}/alsa/control.cpp ${adapters_dir}/alsa/mixer.cpp)
  target_include_directories(poly_alsa_module PRIVATE ${include_dir} ${generated_includes})
  target_link_libraries(poly_alsa_module PRIVATE poly Threads::Threads ALSA::ALSA)
endif()

add_library(poly_backlight_module SHARED ${modules_dir}/backlight.cpp)
target_include_directories(poly_backlight_module PRIVATE ${include_dir} ${generated_includes})
target_link_libraries(poly_backlight_module PRIVATE poly Threads::Threads)

add_library(poly_battery_module SHARED ${modules_dir}/battery.cpp)
target_include_directories(poly_battery_module PRIVATE ${include_dir} ${generated_includes})
target_link_libraries(poly_battery_module PRIVATE poly Threads::Threads)

add_library(poly_bspwm_module SHARED ${modules_dir}/bspwm.cpp)
target_include_directories(poly_bspwm_module PRIVATE ${include_dir} ${generated_includes})
target_link_libraries(poly_bspwm_module PRIVATE poly Threads::Threads)

add_library(poly_counter_module SHARED ${modules_dir}/counter.cpp)
target_include_directories(poly_counter_module PRIVATE ${include_dir} ${generated_includes})
target_link_libraries(poly_counter_module PRIVATE poly Threads::Threads)

add_library(poly_cpu_module SHARED ${modules_dir}/cpu.cpp)
target_include_directories(poly_cpu_module PRIVATE ${include_dir} ${generated_includes})
target_link_libraries(poly_cpu_module PRIVATE poly Threads::Threads)

add_library(poly_date_module SHARED ${modules_dir}/date.cpp)
target_include_directories(poly_date_module PRIVATE ${include_dir} ${generated_includes})
target_link_libraries(poly_date_module PRIVATE poly Threads::Threads)

add_library(poly_fs_module SHARED ${modules_dir}/fs.cpp)
target_include_directories(poly_fs_module PRIVATE ${include_dir} ${generated_includes})
target_link_libraries(poly_fs_module PRIVATE poly Threads::Threads)

if (ENABLE_CURL)
  add_library(poly_github_module SHARED ${modules_dir}/github.cpp)
  target_include_directories(poly_github_module PRIVATE ${include_dir} ${generated_includes})
  target_link_libraries(poly_github_module PRIVATE poly Threads::Threads CURL::libcurl)
endif()

if (ENABLE_I3)
  add_library(poly_i3_module SHARED ${modules_dir}/i3.cpp)
  target_include_directories(poly_i3_module PRIVATE ${include_dir} ${generated_includes})
  target_link_libraries(poly_i3_module PRIVATE poly Threads::Threads i3ipc++)
endif()

add_library(poly_ipc_module SHARED ${modules_dir}/ipc.cpp)
target_include_directories(poly_ipc_module PRIVATE ${include_dir} ${generated_includes})
target_link_libraries(poly_ipc_module PRIVATE poly Threads::Threads)

add_library(poly_memory_module SHARED ${modules_dir}/memory.cpp)
target_include_directories(poly_memory_module PRIVATE ${include_dir} ${generated_includes})
target_link_libraries(poly_memory_module PRIVATE poly Threads::Threads)

add_library(poly_menu_module SHARED ${modules_dir}/menu.cpp)
target_include_directories(poly_menu_module PRIVATE ${include_dir} ${generated_includes})
target_link_libraries(poly_menu_module PRIVATE poly Threads::Threads)

if (ENABLE_MPD)
  add_library(poly_mpd_module SHARED ${modules_dir}/mpd.cpp ${adapters_dir}/mpd.cpp)
  target_include_directories(poly_mpd_module PRIVATE ${include_dir} ${generated_includes})
  target_link_libraries(poly_mpd_module PRIVATE poly Threads::Threads LibMPDClient::LibMPDClient)
endif()

if (ENABLE_NETWORK)
  add_library(poly_network_module SHARED ${modules_dir}/network.cpp ${adapters_dir}/net.cpp)
  target_sources(poly_network_module PRIVATE
    $<IF:${WITH_LIBNL},${adapters_dir}/net_nl.cpp,${adapters_dir}/net_iw.cpp>)
  target_include_directories(poly_network_module PRIVATE ${include_dir} ${generated_includes})
  target_link_libraries(poly_network_module PRIVATE poly Threads::Threads
    $<$<TARGET_EXISTS:Libiw::Libiw>:Libiw::Libiw>
    $<$<TARGET_EXISTS:LibNlGenl3::LibNlGenl3>:LibNlGenl3::LibNlGenl3>)
endif()

if (ENABLE_PULSEAUDIO)
  add_library(poly_pulseaudio_module SHARED ${modules_dir}/pulseaudio.cpp ${adapters_dir}/pulseaudio.cpp)
  target_include_directories(poly_pulseaudio_module PRIVATE ${include_dir} ${generated_includes})
  target_link_libraries(poly_pulseaudio_module PRIVATE poly Threads::Threads LibPulse::LibPulse)
endif()

add_library(poly_script_module SHARED ${modules_dir}/script.cpp)
target_include_directories(poly_script_module PRIVATE ${include_dir} ${generated_includes})
target_link_libraries(poly_script_module PRIVATE poly Threads::Threads)

add_library(poly_systray_module SHARED ${modules_dir}/systray.cpp)
target_include_directories(poly_systray_module PRIVATE ${include_dir} ${generated_includes})
target_link_libraries(poly_systray_module PRIVATE poly Threads::Threads)

add_library(poly_temperature_module SHARED ${modules_dir}/temperature.cpp)
target_include_directories(poly_temperature_module PRIVATE ${include_dir} ${generated_includes})
target_link_libraries(poly_temperature_module PRIVATE poly Threads::Threads)

add_library(poly_text_module SHARED ${modules_dir}/text.cpp)
target_include_directories(poly_text_module PRIVATE ${include_dir} ${generated_includes})
target_link_libraries(poly_text_module PRIVATE poly Threads::Threads)

if (WITH_XRANDR)
  add_library(poly_xbacklight_module SHARED ${modules_dir}/xbacklight.cpp)
  target_include_directories(poly_xbacklight_module PRIVATE ${include_dir} ${generated_includes})
  target_link_libraries(poly_xbacklight_module PRIVATE poly Threads::Threads Xcb::RANDR)
endif()

if (WITH_XKB)
  add_library(poly_xkeyboard_module SHARED ${modules_dir}/xkeyboard.cpp ${x11_dir}/extensions/xkb.cpp)
  target_include_directories(poly_xkeyboard_module PRIVATE ${include_dir} ${generated_includes})
  target_link_libraries(poly_xkeyboard_module PRIVATE poly Threads::Threads Xcb::XKB)
endif()

add_library(poly_xwindow_module SHARED ${modules_dir}/xwindow.cpp)
target_include_directories(poly_xwindow_module PRIVATE ${include_dir} ${generated_includes})
target_link_libraries(poly_xwindow_module PRIVATE poly Threads::Threads)

add_library(poly_xworkspaces_module SHARED ${modules_dir}/xworkspaces.cpp)
target_include_directories(poly_xworkspaces_module PRIVATE ${include_dir} ${generated_includes})
target_link_libraries(poly_xworkspaces_module PRIVATE poly Threads::Threads)

# Target: polybar {{{

add_executable(polybar main.cpp)
target_link_libraries(polybar PRIVATE poly)
foreach(module ${POLYBAR_MODULES})
  target_link_libraries(polybar PRIVATE $<$<TARGET_EXISTS:poly_${module}_module>:poly_${module}_module>)
endforeach()

install(TARGETS polybar
  DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT runtime)

# }}}
# Target: polybar-msg {{{

if(BUILD_IPC_MSG)
  add_executable(polybar-msg
    ipc.cpp
    utils/env.cpp
    utils/file.cpp
    utils/string.cpp)
  target_include_directories(polybar-msg PRIVATE ${dirs})
  target_compile_options(polybar-msg PUBLIC $<$<CXX_COMPILER_ID:GNU>:$<$<CONFIG:MinSizeRel>:-flto>>)

  install(TARGETS polybar-msg
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT runtime)

endif()

# }}}

# Export source file list so that it can be used for test compilation
set(files ${files} PARENT_SCOPE)
