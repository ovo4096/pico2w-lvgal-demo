# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(pico2w-lvgl-demo C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# ==================== LVGL Configuration ====================
# Set LVGL configuration path
set(LV_CONF_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h CACHE STRING "" FORCE)

# Enable demos (benchmark) and disable examples
set(CONFIG_LV_BUILD_DEMOS ON CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# Add LVGL as a subdirectory
add_subdirectory(lvgl-9.4.0)

# Remove Helium assembly files from lvgl target (not supported on RP2350/Cortex-M33)
get_target_property(LVGL_SOURCES lvgl SOURCES)
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*helium.*\\.S$")
set_target_properties(lvgl PROPERTIES SOURCES "${LVGL_SOURCES}")

# ==================== Main Application ====================
# Add executable. Default name is the project name, version 0.1

add_executable(pico2w-lvgl-demo main.c ft6336u.c)

pico_set_program_name(pico2w-lvgl-demo "pico2w-lvgl-demo")
pico_set_program_version(pico2w-lvgl-demo "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(pico2w-lvgl-demo 1)
pico_enable_stdio_usb(pico2w-lvgl-demo 1)

# Add the standard library to the build
target_link_libraries(pico2w-lvgl-demo
        pico_stdlib
        pico_multicore
        hardware_spi
        hardware_i2c
        hardware_gpio
        hardware_dma
        lvgl)

# Add the standard include files to the build
target_include_directories(pico2w-lvgl-demo PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/lvgl-9.4.0
)

pico_add_extra_outputs(pico2w-lvgl-demo)

