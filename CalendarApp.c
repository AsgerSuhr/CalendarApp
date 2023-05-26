#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "secrets.h"

int main()
{
    stdio_init_all();

    if(cyw43_arch_init_with_country(CYW43_COUNTRY_UK)) {
        puts("Failed to initialise\n");
        return 1;
    }

    puts("intialised\n");

    cyw43_arch_enable_sta_mode();

    if (cyw43_arch_wifi_connect_timeout_ms(SSID, PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("failed to connect\n");
        return 1;
    }
    
    printf("connected\n");
    
    while(true) {
        puts("Hello, world!");
        sleep_ms(500);
    }
    

    return 0;
}
