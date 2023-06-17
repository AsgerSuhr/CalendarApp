add_library(NTP_pico STATIC NTP_pico.c NTP_pico.h)
target_include_directories(NTP_pico PRIVATE ../../)
target_link_libraries(NTP_pico 
                    pico_stdlib
                    pico_cyw43_arch_lwip_threadsafe_background
                    pico_lwip_http
                    hardware_rtc)