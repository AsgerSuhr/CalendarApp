/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "ImageData.h"
#include "Debug.h"
#include <stdlib.h>
#include "EPD_7in5_V2.h"

#include "HTTP_pico.h"
#include "NTP_pico.h"
#include "secrets.h"
#define COUNTRY CYW43_COUNTRY_DENMARK
#define AUTH CYW43_AUTH_WPA2_MIXED_PSK

void split_string(char* string) {
    char* token = strtok(string, ",");

    while(token != NULL) {
        printf("%s\n", token);
        token = strtok(NULL, ",");
    }
}

bool is_empty(const char *str) {
    return str == NULL || str[0] == '\0';
}

char* get_json_value(const char* json_string, const char* key) {
    // Prepare the key pattern for search
    char key_pattern[50];
    sprintf(key_pattern, "\"%s\": \"", key);

    // Search for the key pattern in the JSON string
    char* key_start = strstr(json_string, key_pattern);
    if (key_start == NULL) {
        return NULL;
    }

    // Move the pointer to the start of the value
    char* value_start = key_start + strlen(key_pattern);

    // Copy the value into a separate buffer
    char value[50];
    sscanf(value_start, "%[^\"]", value);

    // Return a copy of the value (caller is responsible for freeing it)
    return strdup(value);
}

int main() {

    // Initializing modules
    stdio_init_all();
    rtc_init();

    // preparing data
    char datetime_buf[256];
    char *datetime_str = &datetime_buf[0];
    char events_filter[128];
    datetime_t t;
    char calendar_id[] = "calendar.calendar";
    char local_calendar_url[] = "/api/calendars";

    // connecting to the internet
    setup(COUNTRY, SSID, PASSWORD, AUTH, "picoCalendar", NULL, NULL, NULL);

    // Fetch date and time from NTP server and update the Real Time Clock
    fetch_ntp_time();
    sleep_ms(2000);

    // Fetch the time from the Real Time Clock
    rtc_get_datetime(&t);
    datetime_to_str(datetime_str, sizeof(datetime_buf), &t);
    snprintf(events_filter, sizeof(events_filter), 
        "start=%04d-%02d-%02dT%02d:%02d:%02d&end=%04d-%02d-%02dT%02d:%02d:%02d\n", 
        t.year, t.month, t.day, t.hour + TIME_OFFSET, t.min, t.sec,
        t.year, t.month, t.day+1, t.hour + TIME_OFFSET, t.min, t.sec);
    printf("\r%s      %s      ", datetime_str, events_filter);

    ip_addr_t ip;
    IP4_ADDR(&ip, 192, 168, 1, 118);

    uint16_t port = 80;
    httpc_connection_t settings;
    settings.result_fn = result;
    settings.headers_done_fn = headers;
    err_t err;
    
    // err = httpc_get_file_dns(
    //         "example.com",
    //         80,
    //         "/index.html",
    //         &settings,
    //         body,
    //         NULL,
    //         NULL
    //     ); 

    printf("status %d \n", err);
    sleep_ms(2000);

    err = httpc_get_file_HA_IP(
            &ip,
            8123,
            local_calendar_url,
            ACCESS_TOKEN,
            &settings,
            body,
            NULL,
            NULL); 

    printf("status %d \n", err);
    sleep_ms(2000);

    // while (local_httpc_result != 0) 
    // {
    //     if (local_httpc_result >= 1)
    //     {
    //         cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    //         sleep_ms(1000);
    //         cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    //         sleep_ms(1000);
    //         err = httpc_get_file_HA_IP(
    //                 &ip,
    //                 8123,
    //                 local_calendar_url,
    //                 ACCESS_TOKEN,
    //                 &settings,
    //                 body,
    //                 NULL,
    //                 NULL); 

    //         printf("status %d \n", err);
    //         local_httpc_result = -1;
    //         sleep_ms(2000);
    //     } else 
    //     {
    //         sleep_ms(500);
    //     }
    // }

    while (is_empty(myBuff)) sleep_ms(500);

    split_string(myBuff);

    if(DEV_Module_Init()!=0){
        return -1;
    }

    // print to E-Paper screen
    printf("e-Paper Init and Clear...\r\n");
    EPD_7IN5_V2_Init();

    EPD_7IN5_V2_Clear();
    DEV_Delay_ms(500);

    //Create a new image cache
    UBYTE *BlackImage;
    /* you have to edit the startup_stm32fxxx.s file and set a big enough heap size */
    UWORD Imagesize = ((EPD_7IN5_V2_WIDTH % 8 == 0)? (EPD_7IN5_V2_WIDTH / 8 ): (EPD_7IN5_V2_WIDTH / 8 + 1)) * EPD_7IN5_V2_HEIGHT;
    if((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) {
        printf("Failed to apply for black memory...\r\n");
        return -1;
    }
    printf("Paint_NewImage\r\n");
    Paint_NewImage(BlackImage, EPD_7IN5_V2_WIDTH, EPD_7IN5_V2_HEIGHT, 0, WHITE);
    
    //1.Select Image
    printf("SelectImage:BlackImage\r\n");
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);

    // 2.Drawing on the image
    printf("Drawing:BlackImage\r\n");
    Paint_DrawString_EN(10, 0, datetime_str, &Font16, BLACK, WHITE);
    Paint_DrawLine(5, 20, 400, 20, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawString_EN(20, 0, myBuff, &Font16, BLACK, WHITE);

    printf("EPD_Display\r\n");
    EPD_7IN5_V2_Display(BlackImage);
    DEV_Delay_ms(2000);

    // printf("Clear...\r\n");
    // EPD_7IN5_V2_Clear();

    printf("Goto Sleep...\r\n");
    EPD_7IN5_V2_Sleep();
    free(BlackImage);
    BlackImage = NULL;
    DEV_Delay_ms(2000);//important, at least 2s

    // close 5V
    printf("close 5V, Module enters 0 power consumption ...\r\n");
    DEV_Module_Exit();

    cyw43_arch_deinit();
   return 0;
}