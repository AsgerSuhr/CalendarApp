
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

char **split(char *string, char *separators, int *count, int buffer_size);

int split_string(char* str, const char delimiter, int8_t* max_items, int8_t* max_values, 
                char items[*max_items][*max_values]);

int parse_json_list(char* list, int8_t* max_dicts, uint8_t *max_dict_len, 
                    char dictionaries[*max_dicts][*max_dict_len]);

bool is_empty(const char *str);

/**
 * @brief remove characters from a character array
 * 
 * @param str string you want to remove characters from
 * @param start_index the index from where you want to start removing characters
 * @param num_chars_to_remove how many characters you want to remove
 */
void remove_chars(char* str, int start_index, int num_chars_to_remove);