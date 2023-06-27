#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "ImageData.h"
#include "Debug.h"
#include <stdlib.h>
#include "EPD_7in5_V2.h"

#include "HTTP_pico.h"
#include "NTP_pico.h"
#include "secrets.h"

uint32_t country = CYW43_COUNTRY_DENMARK;
uint32_t auth = CYW43_AUTH_WPA2_MIXED_PSK;

typedef struct {
    char name[50];
    char entity_id[100];
    char start[75];
    char end[75];
    char summary[50];
    char description[100];
} Calendar;


/**
 * @brief Split a string into substrings based on a characters in the separator argument.
 * 
 * This function splits the input string into an array of strings,
 * splitting the input string wherever the any separator character occurs.
 * The separator string itself is not included in any of the output
 * strings. Memory for the array and each individual string is 
 * dynamically allocated.
 * remember to free all memory after using the returned values.
 * 
 * for (int i = 0; i < count_strings; i++) 
 *      free(split_strings[i]);
 *  
 * free(split_strings);
 *
 * @param string The input string to be split.
 * @param separators The characters to split the string on.
 * @param count Pointer to an int where the function will store the number of substrings.
 * @return A dynamically-allocated array of dynamically-allocated strings.
 */
char **split(char *string, char *separators, int *count) 
{
    int len = strlen(string);

    // making an ascii table where everything is zero, 
    // except for the separator characters
    int ascii_table[256] = {0};
    for (int i = 0; separators[i]; i++) {
        ascii_table[(unsigned char)separators[i]] = 1;
    }

    *count = 0;
    
    int i = 0;
    while (i < len) 
    {
        // first we are running past any potential leading
        // separator characters. So when we hit the first character
        // that isn't a separator character we break this loop,
        // because we found the start of the string we want to store
        while (i < len)
        {
            // If the current character is not a separator, break this inner loop.
            // because then we have effectively skipped any leading separators
            if ( !ascii_table[(unsigned char)string[i]] )
                break;
            i++; // otherwise the current character must be a separator and we increment i
        }
        
        /* unlike the previous nested loop, we break here when we hit a character
        that is in the separators array. The old_i variable is just a guard, in case
        we don't encounter any more separator characters. We might just be at the end of 
        the string we are splitting */
        int old_i = i; 
        while (i < len) 
        {
            // now if we hit a new separator character, we now have the length of the substring.
            if ( ascii_table[(unsigned char)string[i]] )
                break;
            i++;
        }

        /* when "i" is bigger than "old_i", it means that there's a substring, and we increment our count.
        If not we probably reached the end */
        if (i > old_i) *count = *count + 1;
    }

    /* now that we know how many strings there is we can allocate the space for it
    on the heap */
    char **strings = malloc(sizeof(char *) * *count);
    if (strings == NULL) {
        fprintf(stderr, "Error! Failed to allocate memory for split function");
        exit(1);
    }

    /* then we basically do the same thing again but this time we store the string */
    i = 0;
    char buffer[1000];
    int string_index = 0;
    while (i < len) 
    {
        while (i < len)
        {
            if ( !ascii_table[(unsigned char)string[i]] )
                break;
            i++;
        }
        
        int j = 0; 
        while (i < len) 
        {
            if ( ascii_table[(unsigned char)string[i]] )
                break;
            buffer[j] = string[i]; // storing the characters, one at a time in the buffer
            i++;
            j++;
        }

        // If there's something to store, we will
        if (j > 0) 
        {
            // adding NULL terminator
            buffer[j] = '\0';
            
            // preparing to store the string/array
            int to_allocate = sizeof(char) * (strlen(buffer) + 1); 
            
            // allocating the memory
            strings[string_index] = malloc(to_allocate);
            
            // copying the content of the buffer to the list we are making
            strcpy(strings[string_index], buffer);

            // incrementing the index, so that the next buffer 
            // will be stored in the nex string item of the list
            string_index++;
        }
    }

    return strings;

}

int split_string(char* str, const char delimiter, int8_t* max_items, int8_t* max_values, 
                char items[*max_items][*max_values]) {
    // removing the first character, in this case => {
    str = str+1;

    // and removing the last character, in this case => }
    size_t length = strlen(str);
    if (length > 0) {
        str[length-1] = '\0';
    }

    char* token = strchr(str, delimiter);
    int8_t count = 0; 
    while (token != NULL) {
        int len = token - str;
        if (len < *max_values && count < *max_items) {
            strncpy(items[0], str, len);
            items[0][len] = '\0';
            count++;
        }
        str = token + 1;
        token = strchr(str, delimiter);
    }

    strncpy(items[1], str, strlen(str));
    items[1][strlen(str)] = '\0';
    // printf("\n%s \n%s", items[0], items[1]);
    *max_items = count;

    return 0;
}

int parse_json_list(char* list, int8_t* max_dicts, uint8_t *max_dict_len, 
                    char dictionaries[*max_dicts][*max_dict_len]) {
    int dictCount = 0;

    char *start = strchr(list, '{');
    char *end;

    while (start) {
        end = strchr(start, '}');

        if (end) {
            int len = end - start + 1;

            if (len < *max_dict_len) {
                strncpy(dictionaries[dictCount], start, len);
                dictionaries[dictCount][len] = '\0';  // add null terminator

                dictCount++;
            }

            start = strchr(end + 1, '{');
        }
        else {
            // mismatched braces, handle error...
            break;
        }
    }

    *max_dicts = dictCount;

    return 0;
}


bool is_empty(const char *str) {
    return str == NULL || str[0] == '\0';
}

int main()
{
    stdio_init_all();
    rtc_init();

    setup(country, SSID, PASSWORD, auth, "MyPicoW", NULL, NULL, NULL);
    
    sleep_ms(500);
    fetch_ntp_time();
    sleep_ms(2000);

    ip_addr_t ip;
    IP4_ADDR(&ip, 192, 168, 1, 118);
    httpc_connection_t settings;
    settings.result_fn = result;
    settings.headers_done_fn = headers;
    char calendar_id[] = "calendar.calendar";
    char local_calendar_url[] = "/api/calendars";
    char test_url[] = "/api/calendars/calendar.asger31_gmail_com?start=2023-06-26T16:56:06&end=2023-06-27T16:56:06";
    err_t err;

    sleep_ms(2000);
    clearBuffer();

    // Fetch the time from the Real Time Clock
    char datetime_buf[256];
    char *datetime_str = &datetime_buf[0];
    char events_filter[128];
    datetime_t t;
    rtc_get_datetime(&t);
    datetime_to_str(datetime_str, sizeof(datetime_buf), &t);
    snprintf(events_filter, sizeof(events_filter), 
        "start=%04d-%02d-%02dT%02d:%02d:%02d&end=%04d-%02d-%02dT%02d:%02d:%02d", 
        t.year, t.month, t.day, t.hour, t.min, t.sec,
        t.year, t.month, t.day+1, t.hour, t.min, t.sec);


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

    while (is_empty(myBuff)) sleep_ms(500);
    sleep_ms(1000);
    uint8_t max_dicts = 10;
    uint8_t max_dict_len = 100;
    char dictionaries[max_dicts][max_dict_len];
    parse_json_list(myBuff, &max_dicts, &max_dict_len, dictionaries);

    clearBuffer();

    // print the dictionaries to verify
    for (int i = 0; i < max_dicts; i++) {
        printf("%s  %d\n", dictionaries[i], i);
    }

    Calendar calendars[max_dicts];
    for (int i = 0; i < max_dicts; i++) 
    {

        int count_strings = 0;
        char **split_strings = split(dictionaries[i], "{},\":", &count_strings);
        for (int j = 0; j < count_strings; j++) 
            printf("%s\n", split_strings[j]);

        strcpy(calendars[i].name, split_strings[1]);
        strcpy(calendars[i].entity_id, split_strings[count_strings-1]);
        for (int i = 0; i < count_strings; i++) 
            free(split_strings[i]);
        free(split_strings);

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




        count_strings = 0;
        split_strings = split(myBuff, "{}[],", &count_strings);
        
        if (count_strings > 0){
            for (int j = 0; j < count_strings; j++) 
                printf("%s\n", split_strings[j]);
            
            strcpy(calendars[i].start, split_strings[1]);
            strcpy(calendars[i].end, split_strings[3]);
            strcpy(calendars[i].summary, split_strings[4]);
            strcpy(calendars[i].summary, split_strings[5]);
        }
        
        for (int i = 0; i < count_strings; i++) 
            free(split_strings[i]);
        free(split_strings);
        
        clearBuffer();
    }

    for (int i = 0; i < max_dicts; i++)
        printf("%s\n", calendars[i].name);
        sleep_ms(100);

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
    Paint_DrawString_EN(20, 0, myBuff, &Font16, BLACK, WHITE);

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