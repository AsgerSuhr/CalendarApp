#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/http_client.h"
#include "hardware/rtc.h"
#include <lwip/apps/sntp.h>
#include "pico/util/datetime.h"
#include "secrets.h"

char myBuff[2500];
uint32_t country = CYW43_COUNTRY_DENMARK;
uint32_t auth = CYW43_AUTH_WPA2_MIXED_PSK;

void set_system_time(u32_t secs){
    // minus difference between 01-01-1970 and 01-01-1900
    // in seconds (70 years = 2208988800 seconds)
    secs -= 2208988800;
    // adding two hours for timezone difference
    secs += 7200;
    time_t epoch = secs;
    struct tm *time = gmtime(&epoch);

    datetime_t datetime = {
    .year = (int16_t) (time->tm_year + 1900),
    .month = (int8_t) (time->tm_mon + 1),
    .day = (int8_t) time->tm_mday,
    .hour = (int8_t) time->tm_hour,
    .min = (int8_t) time->tm_min,
    .sec = (int8_t) time->tm_sec,
    .dotw = (int8_t) time->tm_wday,
    };

    rtc_set_datetime(&datetime);
};


void result(void *arg, httpc_result_t httpc_result,
        u32_t rx_content_len, u32_t srv_res, err_t err)

{
    printf("transfer complete\n");
    printf("local result=%d\n", httpc_result);
    printf("http result=%d\n", srv_res);
}

err_t headers(httpc_state_t *connection, void *arg, 
    struct pbuf *hdr, u16_t hdr_len, u32_t content_len)
{
    printf("headers recieved\n");
    printf("content length=%d\n", content_len);
    printf("header length %d\n", hdr_len);
    pbuf_copy_partial(hdr, myBuff, hdr->tot_len, 0);
    printf("headers \n");
    printf("%s", myBuff);
    return ERR_OK;
}

err_t body(void *arg, struct altcp_pcb *conn, 
                            struct pbuf *p, err_t err)
{
    printf("body\n");
    pbuf_copy_partial(p, myBuff, p->tot_len, 0);
    printf("%s", myBuff);
    return ERR_OK;
}

int setup(uint32_t country, const char *ssid, const char *pass, uint32_t auth, 
    const char *hostname, ip_addr_t *ip, ip_addr_t *mask, ip_addr_t *gw)
{
    if (cyw43_arch_init_with_country(country))
    {
        return 1;
    }

    cyw43_arch_enable_sta_mode();
    if (hostname != NULL)
    {
        netif_set_hostname(netif_default, hostname);
    }

    if (cyw43_arch_wifi_connect_async(ssid, pass, auth))
    {
        return 2;
    }

    int flashrate = 1000;
    int status = CYW43_LINK_UP + 1;
    while (status >= 0 && status != CYW43_LINK_UP)
    {
        int new_status = cyw43_tcpip_link_status(
             &cyw43_state,CYW43_ITF_STA);
        if (new_status != status)
        {
            status = new_status;
            flashrate = flashrate / (status + 1);
            printf("connect status: %d %d\n", 
                                    status, flashrate);
        }
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(flashrate);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(flashrate);
    }
    if (status < 0)
    {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    }
    else
    {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        if (ip != NULL)
        {
            netif_set_ipaddr(netif_default, ip);
        }
        if (mask != NULL)
        {
            netif_set_netmask(netif_default, mask);
        }
        if (gw != NULL)
        {
            netif_set_gw(netif_default, gw);
        }
        printf("IP: %s\n",
        ip4addr_ntoa(netif_ip_addr4(netif_default)));
        printf("Mask: %s\n", 
        ip4addr_ntoa(netif_ip_netmask4(netif_default)));
        printf("Gateway: %s\n",
        ip4addr_ntoa(netif_ip_gw4(netif_default)));
        printf("Host Name: %s\n",
        netif_get_hostname(netif_default));
    }
    return status;
}

int main()
{
    stdio_init_all();
    rtc_init();

    char calendar_id[] = "calendar.calendar";
    char local_calendar_url[] = "http://homeassistant.local:8123/api/calendars";
    
    char datetime_buf[256];
    char *datetime_str = &datetime_buf[0];
    datetime_t t = {
            .year  = 1970,
            .month = 1,
            .day   = 1,
            .dotw  = 0, // 0 is Sunday, so 5 is Friday
            .hour  = 0,
            .min   = 0,
            .sec   = 4
    };

    setup(country, SSID, PASSWORD, auth, "MyPicoW", NULL, NULL, NULL);
    uint16_t port = 80;
    httpc_connection_t settings;
    settings.result_fn = result;
    settings.headers_done_fn = headers;

    //start getting the time
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_init();

    sleep_ms(2000);
    
    rtc_get_datetime(&t);
    datetime_to_str(datetime_str, sizeof(datetime_buf), &t);
    

    // err_t err = httpc_get_file_dns(
    //         "example.com",
    //         80,
    //         "/index.html",
    //         &settings,
    //         body,
    //         NULL,
    //         NULL
    //     ); 

    // Print the time
    while (true) {
        printf("\r%s      ", datetime_str);
        sleep_ms(100);
    }

    // printf("status %d \n", err);
    // while (true){
    //     sleep_ms(500);
    // }
}