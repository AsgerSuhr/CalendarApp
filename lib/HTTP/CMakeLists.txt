add_library(HTTP_pico STATIC HTTP_pico.c HTTP_pico.h)
target_include_directories(HTTP_pico PUBLIC ../../)
include_directories(../Utils)
target_link_libraries(HTTP_pico 
                    pico_stdlib
                    pico_cyw43_arch_lwip_threadsafe_background
                    pico_lwip_http
                    utils)