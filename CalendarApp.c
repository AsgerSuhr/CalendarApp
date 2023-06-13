/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include <time.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/util/datetime.h"
#include "hardware/rtc.h"

#include "lwip/dns.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"

#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "ImageData.h"
#include "Debug.h"
#include <stdlib.h>
#include "EPD_7in5_V2.h"
#include "HTTP_pico.h"

#include "secrets.h"

typedef struct NTP_T_ {
    ip_addr_t ntp_server_address;
    bool dns_request_sent;
    struct udp_pcb *ntp_pcb;
    absolute_time_t ntp_test_time;
    alarm_id_t ntp_resend_alarm;
} NTP_T;

#define NTP_SERVER "pool.ntp.org"
#define NTP_MSG_LEN 48
#define NTP_PORT 123
#define NTP_DELTA 2208988800 // seconds between 1 Jan 1900 and 1 Jan 1970
#define NTP_TEST_TIME (30 * 1000)
#define NTP_RESEND_TIME (10 * 1000)
#define COUNTRY CYW43_COUNTRY_DENMARK
#define AUTH CYW43_AUTH_WPA2_MIXED_PSK
#define TIME_DIFF 2

// Called with results of operation
static void ntp_result(NTP_T* state, int status, time_t *result) {
    if (status == 0 && result) {
        struct tm *utc = gmtime(result);
        char buffer[42];
        snprintf(buffer, sizeof(buffer), 
            "got ntp response: %02d/%02d/%04d %02d:%02d:%02d\n", 
            utc->tm_mday, utc->tm_mon + 1, utc->tm_year + 1900,
            utc->tm_hour + TIME_DIFF, utc->tm_min, utc->tm_sec);
        printf("%s", buffer);

        datetime_t datetime = {
            .year = (int16_t) (utc->tm_year + 1900),
            .month = (int8_t) (utc->tm_mon + 1),
            .day = (int8_t) utc->tm_mday,
            .hour = (int8_t) utc->tm_hour + TIME_DIFF,
            .min = (int8_t) utc->tm_min,
            .sec = (int8_t) utc->tm_sec,
            .dotw = (int8_t) utc->tm_wday,
        };

        rtc_set_datetime(&datetime);
    }

    if (state->ntp_resend_alarm > 0) {
        cancel_alarm(state->ntp_resend_alarm);
        state->ntp_resend_alarm = 0;
    }
    state->ntp_test_time = make_timeout_time_ms(NTP_TEST_TIME);
    state->dns_request_sent = false;
}

static int64_t ntp_failed_handler(alarm_id_t id, void *user_data);

// Make an NTP request
static void ntp_request(NTP_T *state) {
    // cyw43_arch_lwip_begin/end should be used around calls into lwIP to ensure correct locking.
    // You can omit them if you are in a callback from lwIP. Note that when using pico_cyw_arch_poll
    // these calls are a no-op and can be omitted, but it is a good practice to use them in
    // case you switch the cyw43_arch type later.
    cyw43_arch_lwip_begin();
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, NTP_MSG_LEN, PBUF_RAM);
    uint8_t *req = (uint8_t *) p->payload;
    memset(req, 0, NTP_MSG_LEN);
    req[0] = 0x1b;
    udp_sendto(state->ntp_pcb, p, &state->ntp_server_address, NTP_PORT);
    pbuf_free(p);
    cyw43_arch_lwip_end();
}

static int64_t ntp_failed_handler(alarm_id_t id, void *user_data)
{
    NTP_T* state = (NTP_T*)user_data;
    printf("ntp request failed\n");
    ntp_result(state, -1, NULL);
    return 0;
}

// Call back with a DNS result
static void ntp_dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg) {
    NTP_T *state = (NTP_T*)arg;
    if (ipaddr) {
        state->ntp_server_address = *ipaddr;
        printf("ntp address %s\n", ipaddr_ntoa(ipaddr));
        ntp_request(state);
    } else {
        printf("ntp dns request failed\n");
        ntp_result(state, -1, NULL);
    }
}

// NTP data received
static void ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port) {
    NTP_T *state = (NTP_T*)arg;
    uint8_t mode = pbuf_get_at(p, 0) & 0x7;
    uint8_t stratum = pbuf_get_at(p, 1);

    // Check the result
    if (ip_addr_cmp(addr, &state->ntp_server_address) && port == NTP_PORT && p->tot_len == NTP_MSG_LEN &&
        mode == 0x4 && stratum != 0) {
        uint8_t seconds_buf[4] = {0};
        pbuf_copy_partial(p, seconds_buf, sizeof(seconds_buf), 40);
        uint32_t seconds_since_1900 = seconds_buf[0] << 24 | seconds_buf[1] << 16 | seconds_buf[2] << 8 | seconds_buf[3];
        uint32_t seconds_since_1970 = seconds_since_1900 - NTP_DELTA;
        time_t epoch = seconds_since_1970;
        ntp_result(state, 0, &epoch);
    } else {
        printf("invalid ntp response\n");
        ntp_result(state, -1, NULL);
    }
    pbuf_free(p);
}

// Perform initialisation
static NTP_T* ntp_init(void) {
    NTP_T *state = calloc(1, sizeof(NTP_T));
    if (!state) {
        printf("failed to allocate state\n");
        return NULL;
    }
    state->ntp_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
    if (!state->ntp_pcb) {
        printf("failed to create pcb\n");
        free(state);
        return NULL;
    }
    udp_recv(state->ntp_pcb, ntp_recv, state);
    return state;
}

// Runs ntp test forever
void run_ntp_test(void) {
    NTP_T *state = ntp_init();
    if (!state)
        return;
    while(true) {
        if (absolute_time_diff_us(get_absolute_time(), state->ntp_test_time) < 0 && !state->dns_request_sent) {
            // Set alarm in case udp requests are lost
            state->ntp_resend_alarm = add_alarm_in_ms(NTP_RESEND_TIME, ntp_failed_handler, state, true);

            // cyw43_arch_lwip_begin/end should be used around calls into lwIP to ensure correct locking.
            // You can omit them if you are in a callback from lwIP. Note that when using pico_cyw_arch_poll
            // these calls are a no-op and can be omitted, but it is a good practice to use them in
            // case you switch the cyw43_arch type later.
            cyw43_arch_lwip_begin();
            int err = dns_gethostbyname(NTP_SERVER, &state->ntp_server_address, ntp_dns_found, state);
            cyw43_arch_lwip_end();

            state->dns_request_sent = true;
            if (err == ERR_OK) {
                ntp_request(state); // Cached result
            } else if (err != ERR_INPROGRESS) { // ERR_INPROGRESS means expect a callback
                printf("dns request failed\n");
                ntp_result(state, -1, NULL);
            }
        }
        sleep_ms(1000);
    }
    free(state);
}

void fetch_ntp_time() 
{
    NTP_T *state = ntp_init();
    if (!state)
        return;

    // Set alarm in case udp requests are lost
    state->ntp_resend_alarm = add_alarm_in_ms(NTP_RESEND_TIME, ntp_failed_handler, state, true);

    // cyw43_arch_lwip_begin/end should be used around calls into lwIP to ensure correct locking.
    // You can omit them if you are in a callback from lwIP. Note that when using pico_cyw_arch_poll
    // these calls are a no-op and can be omitted, but it is a good practice to use them in
    // case you switch the cyw43_arch type later.
    cyw43_arch_lwip_begin();
    int err = dns_gethostbyname(NTP_SERVER, &state->ntp_server_address, ntp_dns_found, state);
    cyw43_arch_lwip_end();

    state->dns_request_sent = true;
    if (err == ERR_OK) {
        ntp_request(state); // Cached result
    } else if (err != ERR_INPROGRESS) { // ERR_INPROGRESS means expect a callback
        printf("dns request failed\n");
        ntp_result(state, -1, NULL);
    }
    
}

int main() {

    // Initialixing modules
    stdio_init_all();
    rtc_init();

    // preparing data
    char datetime_buf[256];
    char *datetime_str = &datetime_buf[0];
    char events_filter[128];
    datetime_t t;
    char calendar_id[] = "calendar.calendar";
    char local_HA_url[] = "http://homeassistant.local:8123";
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
        t.year, t.month, t.day, t.hour + TIME_DIFF, t.min, t.sec,
        t.year, t.month, t.day+1, t.hour + TIME_DIFF, t.min, t.sec);
    printf("\r%s      %s      ", datetime_str, events_filter);

    ip_addr_t ip;
    IP4_ADDR(&ip, 192, 168, 1, 118);   
    httpc_connection_t settings;
    settings.result_fn = result;
    settings.headers_done_fn = headers;
    err_t err = httpc_get_file_HA_IP(
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

    // err_t err = httpc_get_file_dns(
    //         "example.com",
    //         80,
    //         "/index.html",
    //         &settings,
    //         body,
    //         NULL,
    //         NULL
    //     ); 

    sleep_ms(2000);
    while (true) sleep_ms(500);

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

    while (true) tight_loop_contents();
    cyw43_arch_deinit();
   return 0;
}