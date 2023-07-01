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
#define MAX_EVENTS 10

uint32_t country = CYW43_COUNTRY_DENMARK;
uint32_t auth = CYW43_AUTH_WPA2_MIXED_PSK;

typedef struct {
    bool empty;
    char summary[50];
    char description[100];
    char location[100];
    char start_date[20];
    char end_date[20];
    char start[100];
    char end[100];
} Event;

typedef struct {
    char name[50];
    char entity_id[100];
    bool empty;
    int event_count;
    Event events[MAX_EVENTS];
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
    t_ptr->dotw += offset_days;

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

    // get the right day of the week
    t_ptr->dotw = t_ptr->dotw % 7;
    // if (t_ptr->dotw == 0)
    //     t_ptr->dotw = 7;

}

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
  if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
      strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
    return 0;
  }
  return -1;
}

void init_calendar(Calendar *calendar) {
    calendar->empty = true;
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
    int *calendar_token_pos = NULL;
    for (int i = 0; i < parse_result; i++) {
        if (tokens[i].type == JSMN_OBJECT) {
            calendar_count++;
        }
    }    

    calendar_token_pos = (int*)malloc(calendar_count * sizeof(int));

    if (calendar_token_pos == NULL) {
        printf("Memory allocation failed. Exiting...\n");
        return 1;
    }

    int index = 0;

    for (int i = 0; i < parse_result; i++) {
        if (tokens[i].type == JSMN_OBJECT) {
            calendar_token_pos[index] = i;
            index++;
        }
    }
    
    Calendar calendars[calendar_count];
    for (int i = 0; i < calendar_count; i++) {
        jsmntok_t current_token = tokens[ calendar_token_pos[i] ];
        int current_string_length = current_token.end - current_token.start;
        printf("\n%.*s", current_string_length, myBuff + current_token.start);

        init_calendar(&calendars[i]);

        for (int pairs = 0; pairs < current_token.size; pairs++) {
            jsmntok_t key = tokens[ calendar_token_pos[i] + 1 + (pairs*2) ];
            jsmntok_t value = tokens[ calendar_token_pos[i] + 2 + (pairs*2)];
            
            char key_string[20];
            char value_string[50];
            strncpy(key_string, myBuff + key.start, key.end - key.start);
            strncpy(value_string, myBuff + value.start, value.end - value.start);
            key_string[key.end - key.start] = '\0';
            value_string[value.end - value.start] = '\0';

            // printf("\nkey: %s ", key_string);
            // printf("value: %s", value_string);

            if (strcmp(key_string, "name") == 0) {
                strcpy(calendars[i].name, value_string);
                calendars[i].name[value.end - value.start] = '\0';
                printf("\nCalendar %d name = %s", i, calendars[i].name);
            }
            else if (strcmp(key_string, "entity_id") == 0) {
                strcpy(calendars[i].entity_id, value_string);
                calendars[i].entity_id[value.end - value.start] = '\0';
                printf("\nCalendar %d entity_id = %s", i, calendars[i].entity_id);
            }
        }

    }
    
    free(calendar_token_pos);

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

        // for (int j = 0; j < calendar_parse_result; j++) {
        //     printf("%.*s", myBuff + calendar_tokens[j].start, calendar_tokens[j].end - calendar_tokens[j].start);
        // }
        int event_count = -1;
        for (int j = 0; j < calendar_parse_result; j++) {
            
            if ( j < 0 ) 
                break;

            if (calendar_tokens[j].type == JSMN_OBJECT && calendar_tokens[j].size > 0) {
                
                for (int x = 0; x < calendar_tokens[j].size; x++) {
                    jsmntok_t key_token = calendar_tokens[j + 1 + (x*2)];
                    jsmntok_t value_token = calendar_tokens[j + 2 + (x*2)];
                    if ( key_token.type != JSMN_STRING ) {
                        j++;
                        break;
                    }

                    char key[100];
                    char value[100];
                    int key_length = key_token.end - key_token.start;
                    int value_length = value_token.end - value_token.start;
                    if (key_length > 100)
                        key_length = 100;
                    if (value_length > 100)
                        value_length = 100;
                    strncpy(key, myBuff + key_token.start, key_length);
                    strncpy(value, myBuff + value_token.start, value_length);
                    key[key_length] = '\0';
                    value[value_length] = '\0';
                    
                    printf("Key: %s, Value: %s\n", key, value);

                    if ( strcmp(key, "start") == 0 || strcmp(key, "end") == 0 ) {
                        char time_string[100];
                        char date_string[20];
                        jsmntok_t datetime_token = calendar_tokens[j + 4 + (x*2)];
                        strncpy(time_string, myBuff + datetime_token.start, datetime_token.end - datetime_token.start);
                        time_string[datetime_token.end - datetime_token.start] = '\0';
                        
                        char* result = strchr(time_string, 'T');
                        if ( result != NULL ) {
                            int string_count = 0;
                            char **split_string = split(time_string, "T+", &string_count, 256);
                            strcpy(time_string, split_string[string_count - 2]);
                            strcpy(date_string, split_string[0]);

                            for (int split_index = 0; split_index < string_count; split_index++)
                                free(split_string[split_index]);
                            free(split_string);
                        }

                        if ( strcmp(key, "start") == 0) {
                            event_count++;
                            calendars[i].empty = false;
                            calendars[i].event_count = event_count;
                            strcpy(calendars[i].events[event_count].start, time_string);
                            strcpy(calendars[i].events[event_count].start_date, date_string);
                            printf("Calendars %d start: %s\n", i, calendars[i].events[event_count].start);
                        } else {
                            strcpy(calendars[i].events[event_count].end, time_string);
                            strcpy(calendars[i].events[event_count].end_date, date_string);
                            printf("Calendars %d end: %s\n", i, calendars[i].events[event_count].end);
                        }
                        
                    
                    } else if ( strcmp(key, "summary") == 0 ) {
                        strcpy(calendars[i].events[event_count].summary, value);
                        printf("Calendars %d summary: %s\n", i, calendars[i].events[event_count].summary);
                    } else if ( strcmp(key, "description") == 0 ) {
                        strcpy(calendars[i].events[event_count].description, value);
                        printf("Calendars %d description: %s\n", i, calendars[i].events[event_count].description);
                    } else if ( strcmp(key, "location") == 0 ) {
                        strcpy(calendars[i].events[event_count].location, value);
                        printf("Calendars %d location: %s\n", i, calendars[i].events[event_count].location);
                    }
                }

                j += calendar_tokens[j].size * 2;

            }
        }    
        
        
        clearBuffer();
    }

    for (int i = 0; i < calendar_count; i++) {
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

    int Y_location = 0;
    for (int i = 0; i < DAYS_TO_OFFSET; i++) {
        if (i > 0) 
            offset_datetime(&t, 1);

        datetime_to_today(datetime_str, sizeof(datetime_buf), &t);
        printf("\n\n%s", datetime_str);

        char date_string[11];
        sniprintf(date_string, sizeof(date_string), "%04d-%02d-%02d", t.year, t.month, t.day);
        bool skip = true;
        for (int cal = 0; cal < calendar_count; cal++) {
            if (calendars[cal].empty) 
                continue;
            printf("\n%s", calendars[cal].name);
            printf("    %d", calendars[cal].event_count);
            for (int event = 0; event <= calendars[cal].event_count; event++) {
                printf("\n    %s", calendars[cal].events[event].summary);
                printf("\n    %s", calendars[cal].events[event].start_date);
                printf(" - %s", calendars[cal].events[event].end_date);

                char* several_day_event = strchr(calendars[cal].events[event].start, ':');
                char start_event_date[11];
                char end_event_date[11];
                if (several_day_event != NULL) {
                    strcpy(start_event_date, calendars[cal].events[event].start_date);
                    strcpy(end_event_date, calendars[cal].events[event].end_date);
                    start_event_date[11] = '\0';
                    end_event_date[11] = '\0';
                } else {
                    strcpy(start_event_date, calendars[cal].events[event].start);
                    strcpy(end_event_date, calendars[cal].events[event].end);
                    start_event_date[11] = '\0';
                    end_event_date[11] = '\0';
                }
                
                if ( strcmp(date_string, start_event_date) >= 0 && strcmp(date_string, end_event_date) <= 0 ) {
                    skip = false;
                    break;
                }

            }
            
            if (!skip)
                break;
        }

        if (skip)
            continue;
        
        // printf("Drawing:BlackImage\r\n");
        Paint_DrawString_EN(10, Y_location, datetime_str, &Font24, WHITE, BLACK);
        Paint_DrawLine(5, Y_location+20, 400, Y_location+20, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(10, Y_location+15, 10, Y_location+50, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Y_location += 30;

        for (int cal = 0; cal < calendar_count; cal++) {
            if (calendars[cal].empty) 
                continue;

            Paint_DrawString_EN(20, Y_location, calendars[cal].name, &Font16, WHITE, BLACK);
            Y_location += 20;
            for (int event = 0; event <= calendars[cal].event_count; event++) {


                char* several_day_event = strchr(calendars[cal].events[event].start, ':');
                char start_event_date[11];
                char end_event_date[11];
                if (several_day_event != NULL) {
                    strcpy(start_event_date, calendars[cal].events[event].start_date);
                    strcpy(end_event_date, calendars[cal].events[event].end_date);
                    start_event_date[11] = '\0';
                    end_event_date[11] = '\0';
                } else {
                    strcpy(start_event_date, calendars[cal].events[event].start);
                    strcpy(end_event_date, calendars[cal].events[event].end);
                    start_event_date[11] = '\0';
                    end_event_date[11] = '\0';
                }
                
                if ( strcmp(date_string, start_event_date) >= 0 && strcmp(date_string, end_event_date) <= 0 ) {
                    Paint_DrawPoint(30, Y_location + 5, BLACK, DOT_PIXEL_2X2, DOT_STYLE_DFT);
                    Paint_DrawString_EN(35, Y_location, calendars[cal].events[event].summary, &Font12, WHITE, BLACK);
                    Y_location += 20;
                }
            }
        }


    }

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