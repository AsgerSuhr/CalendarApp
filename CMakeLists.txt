# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH "/home/asger/Documents/pico/pico-sdk")

set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(CalendarApp C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(CalendarApp CalendarApp.c ImageData.c)

pico_set_program_name(CalendarApp "CalendarApp")
pico_set_program_version(CalendarApp "0.1")

pico_enable_stdio_uart(CalendarApp 1)
pico_enable_stdio_usb(CalendarApp 0)

# Add the standard library to the build
target_link_libraries(CalendarApp
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(CalendarApp PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts or any other standard includes, if required
)

# add a compilation subdirectory
add_subdirectory(lib/Config)
add_subdirectory(lib/e-Paper)
add_subdirectory(lib/Fonts)
add_subdirectory(lib/GUI)
add_subdirectory(lib/HTTP)
add_subdirectory(lib/NTP)

# add a header directory
include_directories(lib/Config)
include_directories(lib/GUI)
include_directories(lib/e-Paper)
include_directories(lib/Fonts)
include_directories(lib/HTTP)
include_directories(lib/NTP)

# Add any user requested libraries
target_link_libraries(CalendarApp 
          pico_cyw43_arch_lwip_threadsafe_background
          pico_lwip_http
          hardware_rtc
          pico_lwip_sntp
          ePaper 
          GUI 
          Fonts 
          Config 
          pico_stdlib
          hardware_spi
          HTTP_pico
          NTP_pico
        )

pico_add_extra_outputs(CalendarApp)

