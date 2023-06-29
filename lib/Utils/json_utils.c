#include "json_utils.h"

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
char **split(char *string, char *separators, int *count, int buffer_size) 
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
    char buffer[buffer_size];
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



/**
 * @brief unfinished function, dont use!
 * 
 * @param str 
 * @param delimiter 
 * @param max_items 
 * @param max_values 
 * @param items 
 * @return int 0 if succesfull
 */
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

/**
 * @brief Parses a string representing a JSON list containging dictionaries.
 * it's very specific so can't be used to parse generic JSON data.
 * 
 * @param list 
 * @param max_dicts 
 * @param max_dict_len 
 * @param dictionaries 
 * @return int 
 */
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

/**
 * @brief checks if a string is empty. If it is it will return true, otherwise false
 * 
 * @param str 
 * @return true 
 * @return false 
 */
bool is_empty(const char *str) {
    return str == NULL || str[0] == '\0';
}


void remove_chars(char* str, int start_index, int num_chars_to_remove) {
    int i;
    int len = strlen(str);
    for(i = start_index; i <= len - num_chars_to_remove; i++) {
        str[i] = str[i + num_chars_to_remove];
    }
}