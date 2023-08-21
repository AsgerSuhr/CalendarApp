#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "ImageData.h"
#include "Debug.h"
#include <stdlib.h>
#include "EPD_7in5_V2.h"
#include "hardware/watchdog.h"
#include "pico/multicore.h"

#include "HTTP_pico.h"
#include "HTTP_client.h"
#include "NTP_pico.h"
#include "secrets.h"
#include "json_utils.h"
#define JSMN_HEADER
#include "jsmn.h"
#include "CalendarApp.h"
#include "datetimes.h"
#include "events.h"

#define REFRESH_INTERVAL_MINUTES 60
uint32_t country = CYW43_COUNTRY_DENMARK;
uint32_t auth = CYW43_AUTH_WPA2_MIXED_PSK;

#define TIMEOUT_MS 10000

volatile bool ping_received = false;

// This runs on Core 1 and acts as our watchdog
void core1_entry() {
    uint32_t last_ping_time = to_ms_since_boot(get_absolute_time());

    while (1) {

        if (ping_received) {
            ping_received = false; // Reset the flag
            last_ping_time = to_ms_since_boot(get_absolute_time());
        } else if (to_ms_since_boot(get_absolute_time()) - last_ping_time > TIMEOUT_MS) {
            // Timeout has occurred, reset the system
            watchdog_reboot(0, 0, 1);
        }

        sleep_ms(100); // Sleep for a bit before checking again, to save power
    }
}

void feed_wdt() {
    ping_received = true;
}

void stop_custom_wdt() {
    multicore_reset_core1();
}

void start_custom_wdt() {
    multicore_launch_core1(core1_entry);
}

int main()
{
    stdio_init_all();
    rtc_init();
    // watchdog_enable(6000, 1);

    // Launch the "watchdog" task on Core 1
    start_custom_wdt();

    // first connect to the internet
    setup(country, SSID, PASSWORD, auth, DEVICE_NAME, NULL, NULL, NULL);
    feed_wdt();
    
    // fetch date and time from NTP server, it will also set the RTC on the Pico 
    sleep_ms(500);
    feed_wdt();
    fetch_ntp_time();
    sleep_ms(500);

    clearBuffer();
    feed_wdt();

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
    char local_shopping_list_url[] = "/api/shopping_list";
    err_t err;
    calendar_t calendars[MAX_CALENDAR_AMOUNT];

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
    feed_wdt();
    sleep_ms(1000);

    // Fetch each calendars events and store them in a calendar_t struct
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
        feed_wdt();
        sleep_ms(500);

    }

    feed_wdt();
    sleep_ms(1000);
    feed_wdt();


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

    feed_wdt();

    list_item_t todo_list[MAX_LIST_ITEM_AMOUNT];
    // Get the different calendars available on HA
    err = httpc_get_file_HA(
            "192.168.1.118",
            8123,
            local_shopping_list_url,
            ACCESS_TOKEN,
            &settings,
            todo_list_received,
            todo_list,
            NULL
        ); 
    printf("status %d \n", err);
    sleep_ms(1000);
    feed_wdt();

    if(DEV_Module_Init()!=0){
        return -1;
    }

    // print to E-Paper screen
    feed_wdt();
    printf("e-Paper Init and Clear...\r\n");
    EPD_7IN5_V2_Init();
    feed_wdt();

    EPD_7IN5_V2_Clear();
    DEV_Delay_ms(500);
    feed_wdt();

    //Create a new image cache
    UBYTE *BlackImage;
    /* you have to edit the startup_stm32fxxx.s file and set a big enough heap size */
    UWORD Imagesize = ((EPD_7IN5_V2_WIDTH % 8 == 0)? (EPD_7IN5_V2_WIDTH / 8 ): (EPD_7IN5_V2_WIDTH / 8 + 1)) * EPD_7IN5_V2_HEIGHT;
    if((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) {
        printf("Failed to apply for black memory...\r\n");
        watchdog_reboot(0, 0, 1);
        return -1;
    }
    printf("Paint_NewImage\r\n");
    Paint_NewImage(BlackImage, EPD_7IN5_V2_WIDTH, EPD_7IN5_V2_HEIGHT, 0, WHITE);

    //1.Select Image
    printf("SelectImage:BlackImage\r\n");
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);

    // Paint_DrawBitMap(cat);

    // paint calendar agenda
    print_agenda(&t, calendars, &calendars[0].calendar_count);
    Paint_DrawLine(390, 0, 390, MAX_SCREEN_HEIGHT, BLACK, DOT_PIXEL_2X2, LINE_STYLE_DOTTED);
    print_todo_list(todo_list);

    printf("EPD_Display\r\n");
    EPD_7IN5_V2_Display(BlackImage);
    feed_wdt();
    DEV_Delay_ms(2000);
    feed_wdt();

    printf("Goto Sleep...\r\n");
    EPD_7IN5_V2_Sleep();
    free(BlackImage);
    BlackImage = NULL;
    DEV_Delay_ms(2000);//important, at least 2s
    feed_wdt();

    // close 5V
    printf("close 5V, Module enters 0 power consumption ...\r\n");
    DEV_Module_Exit();

    cyw43_arch_deinit();

    feed_wdt();
    rtc_sleep(REFRESH_INTERVAL_MINUTES);

    while (true){
        sleep_ms(500);
    }
}