#include "events.h"


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
