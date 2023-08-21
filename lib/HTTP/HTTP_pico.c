#include "HTTP_pico.h"

char myBuff[3000];
char packet_buf[MAX_PACKET_SIZE];
int packet_pos = 0;

void init_calendar(calendar_t *calendar) {
    calendar->empty = true;
}

void print_wrapped(const char *text, int wrapLength) {
    int count = 0;
    while (*text) {
        if (count && !(count % wrapLength))
            printf("\n");
        putchar(*text++);
        count++;
    }
}

void clearBuffer()
{
    memset(myBuff, 0, sizeof(myBuff));
}

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
    clearBuffer();
    return ERR_OK;
}

err_t body(void *arg, struct altcp_pcb *conn,
           struct pbuf *p, err_t err)
{
    printf("body\n");
    pbuf_copy_partial(p, myBuff, p->tot_len, 0);
    // printf("%s", myBuff);
    print_wrapped(myBuff, 150);
    printf("\n");
    return ERR_OK;
}

int process_calendars(char *buff, calendar_t* calendars) {

    int parse_result;
    jsmntok_t tokens[128];
    jsmn_parser jsmn;
    jsmn_init(&jsmn);
    parse_result = jsmn_parse(&jsmn, buff, strlen(buff), tokens, 128);
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

    if (calendar_count > MAX_CALENDAR_AMOUNT)
        calendar_count = MAX_CALENDAR_AMOUNT;

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

    for (int i = 0; i < calendar_count; i++) {
        jsmntok_t current_token = tokens[ calendar_token_pos[i] ];
        int current_string_length = current_token.end - current_token.start;
        printf("\n%.*s\n", current_string_length, buff + current_token.start);

        init_calendar(&calendars[i]);
        calendars[i].calendar_count = calendar_count;

        for (int pairs = 0; pairs < current_token.size; pairs++) {
            jsmntok_t key = tokens[ calendar_token_pos[i] + 1 + (pairs*2) ];
            jsmntok_t value = tokens[ calendar_token_pos[i] + 2 + (pairs*2)];
            
            char key_string[20];
            char value_string[50];
            strncpy(key_string, buff + key.start, key.end - key.start);
            strncpy(value_string, buff + value.start, value.end - value.start);
            key_string[key.end - key.start] = '\0';
            value_string[value.end - value.start] = '\0';

            // printf("\nkey: %s ", key_string);
            // printf("value: %s", value_string);

            if (strcmp(key_string, "name") == 0) {
                strcpy(calendars[i].name, value_string);
                calendars[i].name[value.end - value.start] = '\0';
                printf("calendar_t %d name = %s\n", i, calendars[i].name);
            }
            else if (strcmp(key_string, "entity_id") == 0) {
                strcpy(calendars[i].entity_id, value_string);
                calendars[i].entity_id[value.end - value.start] = '\0';
                printf("calendar_t %d entity_id = %s\n", i, calendars[i].entity_id);
            }
        }

    }
    
    free(calendar_token_pos);

    return 0;
}

err_t calendars_received(void *arg, struct altcp_pcb *conn,
           struct pbuf *p, err_t err)
{

    calendar_t *calendars = (calendar_t *)arg;

    if (p != NULL && err == ERR_OK) {
        if (packet_pos + p->tot_len < MAX_PACKET_SIZE) {
            // Copy data to our buffer
            pbuf_copy_partial(p, packet_buf + packet_pos, p->tot_len, 0);
            packet_pos += p->tot_len;

            // If JSON document is complete (for example, it ends with "}" - 
            // you might need a more sophisticated check for your use case)
            if (packet_buf[packet_pos - 1] == ']') {
                // Process JSON
                process_calendars(packet_buf, calendars);

                // Reset buffer
                packet_pos = 0;
                memset(packet_buf, 0, sizeof(packet_buf));
            }
        } else {
            // Buffer overflow, handle error
            printf("buffer overflow!");
            return ERR_BUF;
        }
        pbuf_free(p);
        
    } else if (p != NULL) {
        // If an error occurred but we received a packet, free the packet.
        pbuf_free(p);
    }


    return ERR_OK;
}

int process_calendar_events(char* buff, calendar_t* calendar) {

    int parse_result;
    jsmntok_t tokens[256];
    jsmn_parser jsmn;
    jsmn_init(&jsmn);
    size_t len = strlen(buff);
    parse_result = jsmn_parse(&jsmn, buff, len, tokens, 256);
    if (parse_result < 0) {
        printf("Failed to parse JSON: %d\n", parse_result);
        return 1;
    }

    int event_count = -1;
    for (int j = 0; j < parse_result; j++) {
        
        if ( j < 0 ) 
            break;

        if (tokens[j].type == JSMN_OBJECT && tokens[j].size > 0) {
            
            for (int x = 0; x < tokens[j].size; x++) {
                jsmntok_t key_token = tokens[j + 1 + (x*2)];
                jsmntok_t value_token = tokens[j + 2 + (x*2)];
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
                strncpy(key, buff + key_token.start, key_length);
                strncpy(value, buff + value_token.start, value_length);
                key[key_length] = '\0';
                value[value_length] = '\0';
                
                // printf("Key: %s, Value: %s\n", key, value);

                if ( strcmp(key, "start") == 0 || strcmp(key, "end") == 0 ) {
                    char time_string[100];
                    char date_string[20];
                    jsmntok_t datetime_token = tokens[j + 4 + (x*2)];
                    strncpy(time_string, buff + datetime_token.start, datetime_token.end - datetime_token.start);
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
                    } else {
                        strcpy(date_string, time_string);
                    }

                    if ( strcmp(key, "start") == 0) {
                        event_count++;
                        calendar->empty = false;
                        calendar->event_count = event_count;
                        strcpy(calendar->events[event_count].start, time_string);
                        strcpy(calendar->events[event_count].start_date, date_string);
                        printf("Calendars %s start: %s\n", calendar->name, calendar->events[event_count].start);
                    } else {
                        strcpy(calendar->events[event_count].end, time_string);
                        strcpy(calendar->events[event_count].end_date, date_string);
                        printf("Calendars %s end: %s\n", calendar->name, calendar->events[event_count].end);
                    }
                    
                
                } else if ( strcmp(key, "summary") == 0 ) {
                    strcpy(calendar->events[event_count].summary, value);
                    printf("Calendars %s summary: %s\n", calendar->name, calendar->events[event_count].summary);
                } else if ( strcmp(key, "description") == 0 ) {
                    strcpy(calendar->events[event_count].description, value);
                    printf("Calendars %s description: %s\n", calendar->name, calendar->events[event_count].description);
                } else if ( strcmp(key, "location") == 0 ) {
                    strcpy(calendar->events[event_count].location, value);
                    printf("Calendars %s location: %s\n", calendar->name, calendar->events[event_count].location);
                }
            }
            printf("\n");

            j += tokens[j].size * 2;

        }
    }    
    
    
    clearBuffer();
}

err_t calendar_events_received(void *arg, struct altcp_pcb *conn,
           struct pbuf *p, err_t err)
{
    calendar_t *calendar = (calendar_t *)arg;

    if (p != NULL && err == ERR_OK) {
        if (packet_pos + p->tot_len < MAX_PACKET_SIZE) {
            // Copy data to our buffer
            pbuf_copy_partial(p, packet_buf + packet_pos, p->tot_len, 0);
            packet_pos += p->tot_len;

            // If JSON document is complete (for example, it ends with "}" - 
            // you might need a more sophisticated check for your use case)
            if (packet_buf[packet_pos - 1] == ']') {
                // Process JSON
                print_wrapped(packet_buf, 150);
                process_calendar_events(packet_buf, calendar);

                // Reset buffer
                packet_pos = 0;
                memset(packet_buf, 0, sizeof(packet_buf));
            }
        } else {
            // Buffer overflow, handle error
            printf("buffer overflow!");
            return ERR_BUF;
        }
        pbuf_free(p);
        
    } else if (p != NULL) {
        // If an error occurred but we received a packet, free the packet.
        pbuf_free(p);
    }


    return ERR_OK;
}


int process_todo_list(char* buff, list_item_t* list_items) {

    int parse_result;
    jsmntok_t tokens[256];
    jsmn_parser jsmn;
    jsmn_init(&jsmn);
    parse_result = jsmn_parse(&jsmn, buff, strlen(buff), tokens, 256);
    if (parse_result < 0) {
        printf("Failed to parse JSON: %d\n", parse_result);
        return 1;
    }

    int list_item_count = 0;
    int *list_item_token_pos = NULL;
    for (int i = 0; i < parse_result; i++) {
        if (tokens[i].type == JSMN_OBJECT) {
            list_item_count++;
        }
    }

    if (list_item_count > MAX_LIST_ITEM_AMOUNT)
        list_item_count = MAX_LIST_ITEM_AMOUNT;

    list_item_token_pos = (int*)malloc(list_item_count * sizeof(int));

    if (list_item_token_pos == NULL) {
        printf("Memory allocation failed. Exiting...\n");
        return 1;
    }

    int index = 0;

    for (int i = 0; i < parse_result; i++) {
        if (tokens[i].type == JSMN_OBJECT) {
            list_item_token_pos[index] = i;
            index++;
        }
    }

    for (int i = 0; i < list_item_count; i++) {
        jsmntok_t current_token = tokens[ list_item_token_pos[i] ];
        int current_string_length = current_token.end - current_token.start;
        printf("\n%.*s\n", current_string_length, buff + current_token.start);

        list_items[i].total_items = list_item_count;

        for (int pairs = 0; pairs < current_token.size; pairs++) {
            jsmntok_t key = tokens[ list_item_token_pos[i] + 1 + (pairs*2) ];
            jsmntok_t value = tokens[ list_item_token_pos[i] + 2 + (pairs*2)];
            
            char key_string[20];
            char value_string[50];
            strncpy(key_string, buff + key.start, key.end - key.start);
            strncpy(value_string, buff + value.start, value.end - value.start);
            key_string[key.end - key.start] = '\0';
            value_string[value.end - value.start] = '\0';

            // printf("\nkey: %s ", key_string);
            // printf("value: %s", value_string);

            if (strcmp(key_string, "name") == 0) {
                strcpy(list_items[i].name, value_string);
                list_items[i].name[value.end - value.start] = '\0';
                printf("list item %d name = %s\n", i, list_items[i].name);
            }
            else if (strcmp(key_string, "complete") == 0) {
                if (strcmp(value_string, "true") == 0) {
                    list_items[i].completed = true;
                } else {
                    list_items[i].completed = false;
                }
                printf("list item %d entity_id = %s\n", i, value_string);
            }
        }

    }
    
    free(list_item_token_pos);

    return 0;
}

err_t todo_list_received(void *arg, struct altcp_pcb *conn,
           struct pbuf *p, err_t err)
{
    list_item_t *list_items = (list_item_t *)arg;

    if (p != NULL && err == ERR_OK) {
        if (packet_pos + p->tot_len < MAX_PACKET_SIZE) {
            // Copy data to our buffer
            pbuf_copy_partial(p, packet_buf + packet_pos, p->tot_len, 0);
            packet_pos += p->tot_len;

            // If JSON document is complete (for example, it ends with "}" - 
            // you might need a more sophisticated check for your use case)
            if (packet_buf[packet_pos - 1] == ']') {
                // Process JSON
                print_wrapped(packet_buf, 150);
                process_todo_list(packet_buf, list_items);

                // Reset buffer
                packet_pos = 0;
                memset(packet_buf, 0, sizeof(packet_buf));
            }
        } else {
            // Buffer overflow, handle error
            printf("buffer overflow!");
            return ERR_BUF;
        }
        pbuf_free(p);
        
    } else if (p != NULL) {
        // If an error occurred but we received a packet, free the packet.
        pbuf_free(p);
    }


    return ERR_OK;
}


err_t calendar_body(void *arg, struct altcp_pcb *conn,
           struct pbuf *p, err_t err)
{
    clearBuffer();
    printf("body\n");
    pbuf_copy_partial(p, myBuff, p->tot_len, 0);
    // printf("%s", myBuff);
    print_wrapped(myBuff, 175);
    printf("\n");
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
            &cyw43_state, CYW43_ITF_STA);
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
