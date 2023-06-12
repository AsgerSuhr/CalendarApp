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

add_executable(CalendarApp CalendarApp.c ImageData.c HTTP_pico.c)

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

# add_definitions(
# -DSNTP_SERVER_DNS=1
# -DSNTP_SERVER_ADDRESS="pool.ntp.org"
# -DSNTP_SET_SYSTEM_TIME_NTP=set_system_time
# -DSNTP_STARTUP_DELAY=0
# )

# add a compilation subdirectory
add_subdirectory(lib/Config)
add_subdirectory(lib/e-Paper)
add_subdirectory(lib/Fonts)
add_subdirectory(lib/GUI)

# add a header directory
include_directories(lib/Config)
include_directories(lib/GUI)
include_directories(lib/e-Paper)
# include_directories(lib/Fonts)

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
        )

pico_add_extra_outputs(CalendarApp)

