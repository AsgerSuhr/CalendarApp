add_library(utils STATIC json_utils.c json_utils.h jsmn.h)
target_include_directories(utils PRIVATE ../../)
target_link_libraries(utils 
                    pico_stdlib)