# add_library(utils STATIC json_utils.c json_utils.h jsmn.h)
# target_include_directories(utils PUBLIC ../../)
# target_link_libraries(utils 
#                     pico_stdlib)

set(SOURCE_FILES jsmn.h jsmn.c json_utils.c json_utils.h)

add_library(utils ${SOURCE_FILES})

target_include_directories(utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})