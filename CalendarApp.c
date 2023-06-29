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
#include "jsmn.h"

#define DEVICE_NAME "PiCalendar"
#define DAYS_TO_OFFSET 5

uint32_t country = CYW43_COUNTRY_DENMARK;
uint32_t auth = CYW43_AUTH_WPA2_MIXED_PSK;

typedef struct {
    char summary[50];
    char description[100];
    char start[75];
    char end[75];
} Event;

typedef struct {
    char name[50];
    char entity_id[100];
    bool empty;
    Event events[10];
} Calendar;


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
    // offsetting the date
    t_ptr->day += offset_days;

    // making sure we know whether we are in a leap year or not
    uint8_t february_days = 28;
    if (isLeapYear(t_ptr->year)) february_days = 29;

    // making sure we are in the correct month
    if (t_ptr->month == 2 && t_ptr->day > february_days) {
        t_ptr->month += 1;
        t_ptr->day -= february_days;
    } else if (t_ptr->day > 30 && (t_ptr->month == 4 || t_ptr->month == 6 || t_ptr->month == 9 || t_ptr->month == 11)) {
        t_ptr->month += 1;
        t_ptr->day -= 30;
    } else if (t_ptr->day > 31) {
        t_ptr->month += 1;
        t_ptr->day -= 31;
    }

    // making sure we are in the correct year
    if (t_ptr->month > 12) {
        t_ptr->year += 1;
        t_ptr->month = 1;
    }
}

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
  if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
      strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
    return 0;
  }
  return -1;
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
    sleep_ms(2000);

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

    // Get the different calendars available on HA
    err = httpc_get_file_HA(
            "192.168.1.118",
            8123,
            local_calendar_url,
            ACCESS_TOKEN,
            &settings,
            body,
            NULL,
            NULL
        ); 
    printf("status %d \n", err);

    // Wait until data is recieved from HA
    while (is_empty(myBuff)) sleep_ms(500);
    sleep_ms(1000);


    int parse_result;
    jsmn_parser jsmn;
    jsmntok_t tokens[128];
    jsmn_init(&jsmn);
    parse_result = jsmn_parse(&jsmn, myBuff, strlen(myBuff), tokens,
                    sizeof(tokens) / sizeof(tokens[0]));
    if (parse_result < 0) {
        printf("Failed to parse JSON: %d\n", parse_result);
        return 1;
    }

    int calendar_count = 0;
    for (int i = 0; i < parse_result; i++) {
        if (tokens[i].type == JSMN_OBJECT) {
            calendar_count++;
        }
    }    

    Calendar calendars[calendar_count];
    calendar_count = -1;
    for (int i = 0; i < parse_result; i++) {
        if (tokens[i].type == JSMN_STRING) {
            int current_string_length = tokens[i].end - tokens[i].start;
            char current_string[current_string_length];
            strncpy(current_string, myBuff + tokens[i].start, current_string_length);
            current_string[current_string_length] = '\0';
            printf("\n%s", current_string);

            if (strcmp(current_string, "name") == 0) {
                int next_string_length = tokens[i+1].end - tokens[i+1].start;
                strncpy(calendars[calendar_count].name, myBuff + tokens[i+1].start, next_string_length);
                calendars[calendar_count].name[next_string_length] = '\0';

                printf(":%s", calendars[calendar_count].name);
            } else if (strcmp(current_string, "entity_id") == 0) {
                int next_string_length = tokens[i+1].end - tokens[i+1].start;
                strncpy(calendars[calendar_count].entity_id, myBuff + tokens[i+1].start, next_string_length);
                calendars[calendar_count].entity_id[next_string_length] = '\0';

                printf(":%s", calendars[calendar_count].entity_id);
            }

        } else if (tokens[i].type == JSMN_OBJECT) {
            calendar_count++;
        }
    }
    for (int i = 0; i < calendar_count; i++) {
        printf("\n%s", calendars[i].entity_id);
    }
    clearBuffer();

    // Fetch each calendars events and store them in a Calendar struct
    int calendar_parse_result;
    jsmntok_t calendar_tokens[128];
    for (int i = 0; i < calendar_count; i++) 
    {

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
                body,
                NULL,
                NULL
            ); 

        printf("status %d \n", err);

        while (is_empty(myBuff)) sleep_ms(500);

        jsmn_init(&jsmn);
        calendar_parse_result = jsmn_parse(&jsmn, myBuff, strlen(myBuff), calendar_tokens,
                        sizeof(calendar_tokens) / sizeof(calendar_tokens[0]));

        int event_count = 0;
        int events_token[10];
        memset(events_token, -1, sizeof(events_token));
        for (int j = 0; j < calendar_parse_result; j++) {
            if (calendar_tokens[j].type == JSMN_OBJECT) {
                events_token[event_count] = j;
                event_count++;
                j+=10;
            }
        }    
        
        for (int j = 0; j < 10; j++) {
            if (events_token[j] >= 0) {
                jsmntok_t current_token = calendar_tokens[events_token[j]];
                int current_dictionary_length = current_token.end - current_token.start;
                char current_dictionary[current_dictionary_length];

                strncpy(current_dictionary, myBuff + current_token.start, current_dictionary_length);
                current_dictionary[current_dictionary_length] = '\0';

                printf("\n%s", current_dictionary);
                for (int x = 0; x < current_token.size; x++) {
                    int index = x + current_token.start;
                    if (calendar_tokens[index].type == JSMN_STRING) {
                        printf("\n%.*s", calendar_tokens[index].end - calendar_tokens[index].start, myBuff + calendar_tokens[index].start);
                    }
                }
            }
        }
        
        clearBuffer();
    }

    for (int i = 0; i < parse_result; i++)
        printf("%s\n", calendars[i].name);
        sleep_ms(100);

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
    Paint_DrawString_EN(10, 0, datetime_str, &Font24, WHITE, BLACK);
    Paint_DrawLine(5, 20, 400, 20, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(10, 15, 10, 50, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
/* 
    for (int i = 0, j = 30; i < parse_result; i++) {
        if (!calendars[i].empty) {
            // calendar title
            Paint_DrawString_EN(20, j, calendars[i].name, &Font16, WHITE, BLACK);
            j += 20;

            // Events
            int strings_count = 0;
            char event[250];
            char **split_strings = split(calendars[i].events.start, "T+", &strings_count, 150);
            remove_chars(split_strings[strings_count-2], 4, 3);
            strcpy(event, split_strings[strings_count-2]);
            for (int x = 0; x < strings_count; x++)
                free(split_strings[x]);
            free(split_strings);
            
            split_strings = split(calendars[i].end, "T+", &strings_count, 150);
            remove_chars(split_strings[strings_count-2], 4, 3);
            strcat(event, "-");
            strcat(event, split_strings[strings_count-2]);
            for (int x = 0; x < strings_count; x++)
                free(split_strings[x]);
            free(split_strings);

            strings_count = 0;
            split_strings = split(calendars[i].summary, ":\"", &strings_count, 150);
            strcat(event, " ");
            strcat(event, split_strings[strings_count-1]);
            for (int x = 0; x < strings_count; x++)
                free(split_strings[x]);
            free(split_strings);

            Paint_DrawPoint(30, j+5, BLACK, DOT_PIXEL_2X2, DOT_STYLE_DFT);
            Paint_DrawString_EN(35, j, event, &Font12, WHITE, BLACK);
            j += 20;

        }
    } */

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