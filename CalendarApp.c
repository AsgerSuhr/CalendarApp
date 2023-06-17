#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "ImageData.h"
#include "Debug.h"
#include <stdlib.h>
#include "EPD_7in5_V2.h"

#include "HTTP_pico.h"
#include "NTP_pico.h"
#include "secrets.h"

uint32_t country = CYW43_COUNTRY_DENMARK;
uint32_t auth = CYW43_AUTH_WPA2_MIXED_PSK;

int parse_json_list(char* list, int8_t* max_dicts, uint8_t *max_dict_len, 
                    char dictionaries[*max_dicts][*max_dict_len]) {
    int dictCount = 0;

    char *start = strchr(list, '{');
    char *end;

    while (start) {
        end = strchr(start, '}');

        if (end) {
            int len = end - start + 1;

            if (len < *max_dict_len) {
                strncpy(dictionaries[dictCount], start, len);
                dictionaries[dictCount][len] = '\0';  // add null terminator

                dictCount++;
            }

            start = strchr(end + 1, '{');
        }
        else {
            // mismatched braces, handle error...
            break;
        }
    }

    *max_dicts = dictCount;

    return 0;
}


bool is_empty(const char *str) {
    return str == NULL || str[0] == '\0';
}

int main()
{
    stdio_init_all();
    rtc_init();

    setup(country, SSID, PASSWORD, auth, "MyPicoW", NULL, NULL, NULL);
    
    sleep_ms(500);
    fetch_ntp_time();
    sleep_ms(2000);

    ip_addr_t ip;
    IP4_ADDR(&ip, 192, 168, 1, 118);
    httpc_connection_t settings;
    settings.result_fn = result;
    settings.headers_done_fn = headers;
    char calendar_id[] = "calendar.calendar";
    char local_calendar_url[] = "/api/calendars";
    err_t err;

    sleep_ms(2000);
    clearBuffer();

    err = httpc_get_file_HA(
            "192.168.1.118",
            8123,
            "/api/calendars",
            ACCESS_TOKEN,
            &settings,
            body,
            NULL,
            NULL
        ); 

    printf("status %d \n", err);

    while (is_empty(myBuff)) sleep_ms(500);
    sleep_ms(5000);
    uint8_t max_dicts = 10;
    uint8_t max_dict_len = 100;
    char dictionaries[max_dicts][max_dict_len];
    parse_json_list(myBuff, &max_dicts, &max_dict_len, dictionaries);

    // print the dictionaries to verify
    for (int i = 0; i < max_dicts; i++) {
        printf("%s  %d\n", dictionaries[i], i);
    }

    // Fetch the time from the Real Time Clock
    char datetime_buf[256];
    char *datetime_str = &datetime_buf[0];
    char events_filter[128];
    datetime_t t;
    rtc_get_datetime(&t);
    datetime_to_str(datetime_str, sizeof(datetime_buf), &t);
    snprintf(events_filter, sizeof(events_filter), 
        "start=%04d-%02d-%02dT%02d:%02d:%02d&end=%04d-%02d-%02dT%02d:%02d:%02d\n", 
        t.year, t.month, t.day, t.hour + TIME_OFFSET, t.min, t.sec,
        t.year, t.month, t.day+1, t.hour + TIME_OFFSET, t.min, t.sec);
    printf("\r%s      %s      ", datetime_str, events_filter);

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
    while (true){
        sleep_ms(500);
    }
}