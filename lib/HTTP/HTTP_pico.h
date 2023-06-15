#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/http_client.h"

extern char myBuff[];
extern int8_t local_httpc_result;

void result(void *arg, httpc_result_t httpc_result,
        u32_t rx_content_len, u32_t srv_res, err_t err);
        
err_t headers(httpc_state_t *connection, void *arg, 
        struct pbuf *hdr, u16_t hdr_len, u32_t content_len);

err_t body(void *arg, struct altcp_pcb *conn, 
        struct pbuf *p, err_t err);

int setup(uint32_t country, const char *ssid, const char *pass, uint32_t auth, 
        const char *hostname, ip_addr_t *ip, ip_addr_t *mask, ip_addr_t *gw);