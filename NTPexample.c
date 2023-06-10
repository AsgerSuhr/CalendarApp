#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/http_client.h"
#include "hardware/rtc.h"
#include <lwip/apps/sntp.h>
#include "pico/util/datetime.h"
#include "secrets.h"


void set_system_time(u32_t secs){
    // minus difference between 01-01-1970 and 01-01-1900
    // in seconds (70 years = 2208988800 seconds)
    secs -= 2208988800;
    // adding two hours for timezone difference
    secs += 7200;
    time_t epoch = secs;
    struct tm *time = gmtime(&epoch);

    datetime_t datetime = {
    .year = (int16_t) (time->tm_year + 1900),
    .month = (int8_t) (time->tm_mon + 1),
    .day = (int8_t) time->tm_mday,
    .hour = (int8_t) time->tm_hour,
    .min = (int8_t) time->tm_min,
    .sec = (int8_t) time->tm_sec,
    .dotw = (int8_t) time->tm_wday,
    };

    rtc_set_datetime(&datetime);
};


int main() 
{
    stdio_init_all();

    rtc_init();
    
    char datetime_buf[256];
    char *datetime_str = &datetime_buf[0];
    datetime_t t = {
            .year  = 1970,
            .month = 1,
            .day   = 1,
            .dotw  = 0, // 0 is Sunday, so 5 is Friday
            .hour  = 0,
            .min   = 0,
            .sec   = 4
    };

    if (cyw43_arch_init_with_country(CYW43_COUNTRY_DENMARK)) {
    printf("failed to initialise\n");
    return 1;
    }

    cyw43_arch_enable_sta_mode();

    if (cyw43_arch_wifi_connect_timeout_ms(SSID, PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
    printf("failed to connect\n");
    return 1;
    }

    //start getting the time
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_init();

    sleep_ms(2000);
    // Print the time
    while (true) {
        rtc_get_datetime(&t);
        datetime_to_str(datetime_str, sizeof(datetime_buf), &t);
        printf("\r%s      ", datetime_str);
        sleep_ms(100);
    }
}