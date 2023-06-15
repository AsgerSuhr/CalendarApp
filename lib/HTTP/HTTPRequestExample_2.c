#include "HTTP_pico.h"
#include "secrets.h"

uint32_t country = CYW43_COUNTRY_DENMARK;
uint32_t auth = CYW43_AUTH_WPA2_MIXED_PSK;

int main()
{
    stdio_init_all();
    setup(country, SSID, PASSWORD, auth, "MyPicoW", NULL, NULL, NULL);
    uint16_t port = 80;
    httpc_connection_t settings;
    settings.result_fn = result;
    settings.headers_done_fn = headers;

    err_t err = httpc_get_file_dns(
            "example.com",
            80,
            "/index.html",
            &settings,
            body,
            NULL,
            NULL
        ); 

    printf("status %d \n", err);
    while (true){
        sleep_ms(500);
    }
}