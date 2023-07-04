#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "ImageData.h"
#include "Debug.h"
#include <stdlib.h>
#include "EPD_7in5_V2.h"

#include "HTTP_pico.h"
#include "NTP_pico.h"
#include "secrets.h"
#include "json_utils.h"
#define JSMN_HEADER
#include "jsmn.h"
#include "CalendarApp.h"
#include "datetimes.h"
#include "events.h"

uint32_t country = CYW43_COUNTRY_DENMARK;
uint32_t auth = CYW43_AUTH_WPA2_MIXED_PSK;

int main()
{
    stdio_init_all();
    rtc_init();

    // first connect to the internet
    setup(country, SSID, PASSWORD, auth, DEVICE_NAME, NULL, NULL, NULL);
    
    // fetch date and time from NTP server, it will also set the RTC on the Pico 
    sleep_ms(500);
    fetch_ntp_time();
    sleep_ms(500);

    clearBuffer();

    // Fetch the time from the Real Time Clock
    char datetime_buf[25];
    char *datetime_str = &datetime_buf[0];
    char events_filter[50];
    datetime_t t;
    datetime_t t_offset;
    rtc_get_datetime(&t);
    memcpy(&t_offset, &t, sizeof(datetime_t));
    datetime_to_today(datetime_str, sizeof(datetime_buf), &t);
    offset_datetime(&t_offset, DAYS_TO_OFFSET);
    snprintf(events_filter, sizeof(events_filter), 
        "start=%04d-%02d-%02dT%02d:%02d:%02d&end=%04d-%02d-%02dT%02d:%02d:%02d",
        t.year, t.month, t.day, t.hour, t.min, t.sec,
        t_offset.year, t_offset.month, t_offset.day, t_offset.hour, t_offset.min, t_offset.sec);

    // setup HTTP settings
    httpc_connection_t settings;
    settings.result_fn = result;
    settings.headers_done_fn = headers;
    char local_calendar_url[] = "/api/calendars";
    err_t err;
    Calendar calendars[MAX_CALENDAR_AMOUNT];

    // Get the different calendars available on HA
    err = httpc_get_file_HA(
            "192.168.1.118",
            8123,
            local_calendar_url,
            ACCESS_TOKEN,
            &settings,
            calendars_received,
            calendars,
            NULL
        ); 
    printf("status %d \n", err);
    sleep_ms(1000);

    // Fetch each calendars events and store them in a Calendar struct
    for (int i = 0; i < calendars[0].calendar_count; i++) {

        char package[100];
        strcpy(package, local_calendar_url);
        strcat(package, "/");
        strcat(package, calendars[i].entity_id);
        strcat(package, "?");
        strcat(package, events_filter);
        err = httpc_get_file_HA(
                "192.168.1.118",
                8123,
                package,
                ACCESS_TOKEN,
                &settings,
                calendar_events_received,
                &calendars[i],
                NULL
            ); 

        printf("status %d \n", err);

    }

    sleep_ms(1000);


    // Print sorted JSON data
    for (int i = 0; i < calendars[0].calendar_count; i++) {
        printf("name: %s\n", calendars[i].name);
        printf("entity_id: %s\n", calendars[i].entity_id);
        printf("empty: %d\n", (int)calendars[i].empty);
        if (calendars[i].empty) 
            continue;
        printf("events: \n");
        for (int j = 0; j <= calendars[i].event_count; j++) {
            printf("    - %s\n", (int)calendars[i].events[j].summary);
            printf("      %s - %s\n", (int)calendars[i].events[j].start_date, (int)calendars[i].events[j].end_date);
            printf("      %s - %s\n", (int)calendars[i].events[j].start, (int)calendars[i].events[j].end);
            printf("      %s\n", (int)calendars[i].events[j].description);
            printf("      %s\n", (int)calendars[i].events[j].location);
        }
    }

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

    // print calendar agenda
    print_agenda(&t, calendars, &calendars[0].calendar_count);

    printf("EPD_Display\r\n");
    EPD_7IN5_V2_Display(BlackImage);
    DEV_Delay_ms(2000);

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