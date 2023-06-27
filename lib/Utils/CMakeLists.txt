add_library(utils STATIC json_utils.c json_utils.h)
target_include_directories(utils PRIVATE ../../)
# target_link_libraries(utils 
#                     pico_stdlib
#                     pico_cyw43_arch_lwip_threadsafe_background
#                     pico_lwip_http)