#include "datetimes.h"

#include "hardware/clocks.h"
#include "hardware/rosc.h"
#include "hardware/structs/scb.h"

#define RST_PIN 22

uint scb_orig;
uint clock0_orig;
uint clock1_orig;

void recover_from_sleep(uint scb_orig, uint clock0_orig, uint clock1_orig){

    //Re-enable ring Oscillator control
    rosc_write(&rosc_hw->ctrl, ROSC_CTRL_ENABLE_BITS);

    //reset procs back to default
    scb_hw->scr = scb_orig;
    clocks_hw->sleep_en0 = clock0_orig;
    clocks_hw->sleep_en1 = clock1_orig;

    //reset clocks
    clocks_init();
    stdio_init_all();

    return;
}

void sleep_callback() {
    recover_from_sleep(scb_orig, clock0_orig, clock1_orig);
    printf("RTC woke us up\n");
    uart_default_tx_wait_blocking();
    watchdog_reboot(0,0,1);
    while (true);
}


void rtc_sleep(int minutes) {

    datetime_t t_alarm;
    rtc_get_datetime(&t_alarm);

    offset_datetime_minutes(&t_alarm, minutes);

    if (t_alarm.hour < 6) {
        t_alarm.hour = 6;
        t_alarm.min = 0;
        t_alarm.sec = 0;
    }

    printf("Sleeping until %02d:%02d:%02d\n", t_alarm.hour, t_alarm.min, t_alarm.sec);
    uart_default_tx_wait_blocking();

    //save values for later
    scb_orig = scb_hw->scr;
    clock0_orig = clocks_hw->sleep_en0;
    clock1_orig = clocks_hw->sleep_en1;

    stop_custom_wdt();
    
    // Seems like watchdog only works with ROSC
    sleep_run_from_xosc();
    // sleep_run_from_rosc();

    sleep_goto_sleep_until(&t_alarm, &sleep_callback);

}

void datetime_to_today(char *buf, uint buf_size, const datetime_t *t) {
    snprintf(buf,
             buf_size,
             "%s %d %s",
             DATETIME_DOWS[t->dotw],
             t->day,
             DATETIME_MONTHS[t->month - 1]);
             
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

void offset_datetime_minutes(datetime_t *t_ptr, int minutes) {

    int total_minutes = t_ptr->min + minutes;
    t_ptr->min = total_minutes % 60;

    int hours_to_add = total_minutes / 60;
    int total_hours = t_ptr->hour + hours_to_add;
    t_ptr->hour = total_hours % 24;

    int days_to_add = total_hours / 24;
    offset_datetime(t_ptr, days_to_add);

}