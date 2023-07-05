#include "fonts.h"
#include "CalendarApp.h"
#include "pico/stdlib.h"
#include <string.h>
#include "datetimes.h"
#include "DEV_Config.h"
#include "GUI_Paint.h"

bool event_on_date(event_t *event, char *date);
void print_agenda(datetime_t *date, calendar_t *calendars, int *num_calendars);
void print_todo_list(list_item_t* list_items);