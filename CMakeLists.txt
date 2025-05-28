cmake_minimum_required(VERSION 3.15)
project(RPi4Optimized C CXX)

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
set(FAST_FLAGS "-O3 -ffast-math -funsafe-math-optimizations -funroll-loops -fomit-frame-pointer -flto")

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
    src/set/set.c
    src/controller/controller.c
    src/engine/engine.c
    src/engine/ports.c
    src/engine/host.c
    src/engine/node.c
    src/engine/ui.c
    src/utils/constants.c
    src/utils/cJSON.c
    src/utils/stb_ds.c
    src/utils/util.c
)

target_include_directories(elvira PRIVATE
    ${PIPEWIRE_INCLUDE_DIRS}
    ${LV2_INCLUDE_DIRS}
    ${GTK3_INCLUDE_DIRS}
    ${LILV_INCLUDE_DIRS}
    ${SUIL_INCLUDE_DIRS}
    src/set
    src/controller
    src/engine
    src/utils
)

target_link_libraries(elvira PRIVATE
    ${LILV_LIBRARIES}
    ${SUIL_LIBRARIES}
    ${GTK3_LIBRARIES}
    ${PIPEWIRE_LIBRARIES}
    ${LV2_LIBRARIES}
    dl
)

