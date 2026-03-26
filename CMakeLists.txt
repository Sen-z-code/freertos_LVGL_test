cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME FreeRTOS_LVGL_test_1)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# LVGL sources (auto-collect to avoid maintaining hundreds of file paths manually)
file(GLOB_RECURSE LVGL_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/lvgl/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lvgl/porting/*.c
)

# Exclude desktop/host-specific and test/demo sources for STM32 bare-metal build
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/demos/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/debugging/test/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/drivers/windows/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/drivers/x11/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/drivers/wayland/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/drivers/opengles/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/drivers/sdl/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/drivers/qnx/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/drivers/uefi/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/drivers/libinput/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/drivers/evdev/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/drivers/nuttx/")

# Further trim LVGL for STM32 + SW rendering + custom ST7789/XPT2046 ports
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/drivers/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/draw/eve/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/draw/vg_lite/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/draw/nanovg/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/draw/nema_gfx/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/draw/nxp/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/draw/opengles/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/draw/sdl/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/draw/dma2d/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/draw/espressif/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/libs/FT800-FT813/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/libs/ffmpeg/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/libs/freetype/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/libs/gltf/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/libs/gstreamer/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/libs/nanovg/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/libs/rlottie/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/libs/svg/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/libs/tiny_ttf/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/libs/vg_lite_driver/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/libs/libjpeg_turbo/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/libs/libpng/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/libs/libwebp/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/libs/lodepng/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/osal/lv_windows\\.c$")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/osal/lv_sdl2\\.c$")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/osal/lv_pthread\\.c$")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/osal/lv_rtthread\\.c$")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/osal/lv_linux\\.c$")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/osal/lv_mqx\\.c$")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/lvgl/src/osal/lv_cmsis_rtos2\\.c$")

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # User custom sources (persistent across CubeMX re-generation)
    Core/APP/LVGLTask.c
    Core/APP/st7789.c
    Core/APP/xpt2046.c
    ${LVGL_SOURCES}
    Core/UI/myui.c
    Core/UI/Sen_notosanssc_16.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # User custom include paths (persistent across CubeMX re-generation)
    Core/APP
    lvgl
    lvgl/src
    lvgl/porting
    Core/UI
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
