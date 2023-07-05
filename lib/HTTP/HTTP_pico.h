#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/http_client.h"
#include "CalendarApp.h"
#include "json_utils.h"
#define JSMN_HEADER
#include "jsmn.h"

#define MAX_PACKET_SIZE 4096
#ifndef MAX_CALENDAR_AMOUNT
        #define MAX_CALENDAR_AMOUNT 10
#endif // MAX_CALENDAR_AMOUNT
#ifndef MAX_LIST_ITEM_AMOUNT
        #define MAX_LIST_ITEM_AMOUNT 20
#endif // MAX_LIST_ITEM_AMOUNT

extern char myBuff[];
extern char packet_buf[];
extern int packet_pos;

void init_calendar(calendar_t *calendar);

void clearBuffer();

void result(void *arg, httpc_result_t httpc_result,
        u32_t rx_content_len, u32_t srv_res, err_t err);
        
err_t headers(httpc_state_t *connection, void *arg, 
        struct pbuf *hdr, u16_t hdr_len, u32_t content_len);

err_t body(void *arg, struct altcp_pcb *conn, 
        struct pbuf *p, err_t err);

int process_calendars(char *buff, calendar_t* calendars);

err_t calendars_received(void *arg, struct altcp_pcb *conn,
           struct pbuf *p, err_t err);

int process_calendar_events(char* buff, calendar_t* calendar);

err_t calendar_events_received(void *arg, struct altcp_pcb *conn,
           struct pbuf *p, err_t err);

int process_todo_list(char* buff, list_item_t* list_items);

err_t todo_list_received(void *arg, struct altcp_pcb *conn,
           struct pbuf *p, err_t err);

int setup(uint32_t country, const char *ssid, const char *pass, uint32_t auth, 
        const char *hostname, ip_addr_t *ip, ip_addr_t *mask, ip_addr_t *gw);