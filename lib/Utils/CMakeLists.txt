# add_library(utils STATIC json_utils.c json_utils.h jsmn.h)
# target_include_directories(utils PUBLIC ../../)
# target_link_libraries(utils 
#                     pico_stdlib)

set(SOURCE_FILES jsmn.h 
                jsmn.c 
                datetimes.c 
                datetimes.h 
                json_utils.c 
                json_utils.h
                events.c 
                events.h)

# add_library(utils ${SOURCE_FILES})
# target_include_directories(utils PUBLIC ../../)
# include_directories(../Config)
# include_directories(../GUI)
# target_include_directories(utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# target_link_libraries(utils 
#                     pico_stdlib
#                     hardware_spi)

add_library(utils STATIC ${SOURCE_FILES})
target_include_directories(utils PUBLIC ../../)
include_directories(../Config)
include_directories(../GUI)
include_directories(../Fonts)
target_link_libraries(utils 
                    pico_stdlib
                    hardware_spi
                    hardware_sleep
                    Fonts)