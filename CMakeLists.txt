cmake_minimum_required(VERSION 3.15)
project(elvira C CXX)

# Enable Release by default if nothing is specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Detect ARM architecture bitness and set correct flags
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    message(STATUS "Target is ARM64 (aarch64)")
    set(ARCH_FLAGS "-march=armv8-a+crc+simd -mtune=cortex-a72")
    # No -mfpu needed in 64-bit mode
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "armv7l|armv8l|armhf")
    message(STATUS "Target is ARM32")
    set(ARCH_FLAGS "-march=armv8-a+crc+simd -mtune=cortex-a72 -mfpu=neon-fp-armv8")
else()
    message(WARNING "Unknown architecture, using default flags")
    set(ARCH_FLAGS "")
endif()

# Common fast math and aggressive flags
set(FAST_FLAGS "-O3 -ffast-math -funsafe-math-optimizations -funroll-loops -fomit-frame-pointer -flto=auto")

# Apply to C and C++
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ARCH_FLAGS} ${FAST_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ARCH_FLAGS} ${FAST_FLAGS}")


find_package(PkgConfig REQUIRED)
pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)
pkg_check_modules(LV2 REQUIRED lv2)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(LILV REQUIRED lilv-0)
pkg_check_modules(SUIL REQUIRED suil-0)

add_executable(elvira
    src/elvira.c
    src/ports.c
    src/node.c
    src/host.c
    src/set.c
    src/handler_process.c
    src/handler_destroy.c
    src/handler_param.c
    src/handler_host.c
    src/handler_state.c
    src/handler_registry.c
    src/handler_metadata.c
    src/handler_ui.c
    src/constants.c
    src/runtime.c
)

target_include_directories(elvira PRIVATE
    ${PIPEWIRE_INCLUDE_DIRS}
    ${LV2_INCLUDE_DIRS}
    ${GTK3_INCLUDE_DIRS}
    ${LILV_INCLUDE_DIRS}
    ${SUIL_INCLUDE_DIRS}
    src
)

target_link_libraries(elvira PRIVATE
    ${LILV_LIBRARIES}
    ${SUIL_LIBRARIES}
    ${GTK3_LIBRARIES}
    ${PIPEWIRE_LIBRARIES}
    ${LV2_LIBRARIES}
    dl
)

install(TARGETS elvira DESTINATION bin)
