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

#define DEVICE_NAME "PiCalendar"
#define DAYS_TO_OFFSET 5
#define MAX_CALENDAR_AMOUNT 10

uint32_t country = CYW43_COUNTRY_DENMARK;
uint32_t auth = CYW43_AUTH_WPA2_MIXED_PSK;

static const char *DATETIME_MONTHS[12] = {
        "January",
        "February",
        "March",
        "April",
        "May",
        "June",
        "July",
        "August",
        "September",
        "October",
        "November",
        "December"
};

static const char *DATETIME_DOWS[7] = {
        "Sunday",
        "Monday",
        "Tuesday",
        "Wednesday",
        "Thursday",
        "Friday",
        "Saturday",
};


void datetime_to_today(char *buf, uint buf_size, const datetime_t *t) {
    snprintf(buf,
             buf_size,
             "%s %d %s %d",
             DATETIME_DOWS[t->dotw],
             t->day,
             DATETIME_MONTHS[t->month - 1],
             t->year);
};

bool isLeapYear(int year) {
    if (year % 4 == 0) {
        if (year % 100 == 0) {
            if (year % 400 == 0) {
                return true; // Divisible by 400: leap year
            } else {
                return false; // Divisible by 100 but not 400: not a leap year
            }
        } else {
            return true; // Divisible by 4 but not 100: leap year
        }
    } else {
        return false; // Not divisible by 4: not a leap year
    }
}

void offset_datetime(datetime_t *t_ptr, uint8_t offset_days) {

    for (int i=0; i < offset_days; i++) {
        // offsetting the date
        t_ptr->day += 1;
        t_ptr->dotw += 1;

        // making sure we know whether we are in a leap year or not
        uint8_t february_days = 28;
        if (isLeapYear(t_ptr->year)) february_days = 29;

        // making sure we are in the correct month
        if (t_ptr->month == 2 && t_ptr->day > february_days) {
            t_ptr->month += 1;
            t_ptr->day = 1;
        } else if (t_ptr->day > 30 && (t_ptr->month == 4 || t_ptr->month == 6 || t_ptr->month == 9 || t_ptr->month == 11)) {
            t_ptr->month += 1;
            t_ptr->day = 1;
        } else if (t_ptr->day > 31) {
            t_ptr->month += 1;
            t_ptr->day = 1;
        }

        // making sure we are in the correct year
        if (t_ptr->month > 12) {
            t_ptr->year += 1;
            t_ptr->month = 1;
            t_ptr->day = 1;
        }

        // get the right day of the week
        t_ptr->dotw = t_ptr->dotw % 7;
    }

}

// Function to check if a date falls within the event's start and end dates
bool event_on_date(Event *event, char *date) {
    // if start isn't a timestamp (00:00:00) it must be a date 2023-07-01
    // which means its a whole day event
    char *several_day_event = strchr(event->start, ':'); 

    if (several_day_event != NULL) {
        return (strcmp(event->start_date, date) <= 0 && strcmp(event->end_date, date) >= 0);
    } else {
        return (strcmp(event->start, date) <= 0 && strcmp(event->end, date) > 0);
    }
}

void print_agenda(datetime_t *date, Calendar *calendars, int *num_calendars) {
    datetime_t current_date = *date;
    char datetime_buf[25];
    char *datetime_str = &datetime_buf[0];
    int Y_location = 0;
    for (int day=0; day < DAYS_TO_OFFSET; day++) {
        if (Y_location >= 450)
            break;
        datetime_to_today(datetime_str, sizeof(datetime_buf), &current_date);
        
        char date_string[11];
        sniprintf(date_string, sizeof(date_string), "%04d-%02d-%02d", 
                    current_date.year, current_date.month, current_date.day);
        
        bool date_has_events_today = false;
        for (int i=0; i < *num_calendars; i++) {

            Calendar current_calendar = calendars[i];

            if (!current_calendar.empty) {

                bool calendar_has_event_today = false;
                for (int j=0; j <= current_calendar.event_count; j++){
                    if (event_on_date(&calendars[i].events[j], date_string)) {
                        if (!date_has_events_today) {
                            date_has_events_today = true;
                            printf("Events on %s\n", datetime_str);
                            Paint_DrawString_EN(10, Y_location, datetime_str, &Font24, WHITE, BLACK);
                            Paint_DrawLine(5, Y_location+20, 400, Y_location+20, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
                            Paint_DrawLine(10, Y_location+15, 10, Y_location+50, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
                            Y_location += 30;
                        }
                        if (!calendar_has_event_today) {
                            printf("\t%s\n", current_calendar.name);
                            calendar_has_event_today = true;
                            Paint_DrawString_EN(20, Y_location, current_calendar.name, &Font16, WHITE, BLACK);
                            Y_location += 20;
                        }
                        printf("\t\tSummary: %s\n", current_calendar.events[j].summary);
                        printf("\t\tStart: %s\n", current_calendar.events[j].start);
                        printf("\t\tEnd: %s\n", current_calendar.events[j].end);
                        printf("\t\tDescription: %s\n", current_calendar.events[j].description);
                        printf("\t\tLocation: %s\n", current_calendar.events[j].location);
                        char *several_day_event = strchr(current_calendar.events[j].start, ':');
                        char to_write[100];
                        char start[11];
                        char end[11];
                        if (several_day_event != NULL) {
                            strcpy(end, current_calendar.events[j].end);
                            end[5] = '\0';
                            strcpy(start, current_calendar.events[j].start);
                            start[5] = '\0';
                            snprintf(to_write, sizeof(to_write), "%s-%s %s",
                                start, end, current_calendar.events[j].summary);

                        } else {
                            strcpy(end, current_calendar.events[j].end);
                            end[11] = '\0';
                            strcpy(start, current_calendar.events[j].start);
                            start[11] = '\0';
                            snprintf(to_write, sizeof(to_write), "All Day - %s",
                                current_calendar.events[j].summary);
                        }
                        Paint_DrawPoint(30, Y_location + 5, BLACK, DOT_PIXEL_2X2, DOT_STYLE_DFT);
                        Paint_DrawString_EN(35, Y_location, to_write, &Font12, WHITE, BLACK);
                        Y_location += 20;

                        if (Y_location >= 450)
                            break;
                    }
                }
            }
        }
        
        offset_datetime(&current_date, 1);
    }
}


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