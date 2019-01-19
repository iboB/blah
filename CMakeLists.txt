cmake_minimum_required(VERSION 3.2.2)

# IDE folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(vr-dvh)

set(CMAKE_CXX_STANDARD 14)

set(GNT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${GNT_ROOT}/cmake
)

include(src_group)

# Determine platform
if (EMSCRIPTEN)
    set(PLATFORM_BROWSER 1)
    set(PLATFORM_EMSCRIPTEN 1)
elseif (WIN32)
    set(PLATFORM_WINDOWS 1)
    set(PLATFORM_DESKTOP 1)
else()
    set(PLATFORM_LINUX 1)
    set(PLATFORM_UNIX 1)
    set(PLATFORM_DESKTOP 1)
endif()

# Third party libraries
add_subdirectory(third_party)

# Main targets
add_subdirectory(src)
