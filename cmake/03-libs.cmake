#
# Check libraries
#

find_package(Threads REQUIRED)
find_package(CairoFc REQUIRED)

if (ENABLE_ALSA)
  find_package(ALSA REQUIRED)
endif()

if (ENABLE_CURL)
  find_package(CURL REQUIRED)
endif()

if (ENABLE_MPD)
  find_package(MPDClient REQUIRED)
endif()

if(WITH_LIBNL)
  find_package(LibNlGenl3 REQUIRED)
else()
  find_package(Libiw REQUIRED)
endif()

if (ENABLE_PULSEAUDIO)
  find_package(LibPulse REQUIRED)
endif()

set(XORG_EXTENSIONS RANDR)
if (WITH_XCOMPOSITE)
  set(XORG_EXTENSIONS ${XORG_EXTENSIONS} COMPOSITE)
endif()
if (WITH_XKB)
  set(XORG_EXTENSIONS ${XORG_EXTENSIONS} XKB)
endif()
if (WITH_XCURSOR)
  set(XORG_EXTENSIONS ${XORG_EXTENSIONS} CURSOR)
endif()
if (WITH_XRM)
  set(XORG_EXTENSIONS ${XORG_EXTENSIONS} XRM)
endif()

find_package(Xcb 1.12 REQUIRED COMPONENTS ${XORG_EXTENSIONS})

# FreeBSD Support
if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
  find_package(LibInotify REQUIRED)
endif()
