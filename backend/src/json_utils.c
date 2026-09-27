#include "json_utils.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

void copy_string(char *destination, size_t destination_size, const char *source) {
    if (destination == NULL || destination_size == 0) {
        return;
    }
    if (source == NULL) {
        destination[0] = '\0';
        return;
    }
    snprintf(destination, destination_size, "%s", source);
}

void set_error(char *destination, size_t destination_size, const char *message) {
    copy_string(destination, destination_size, message);
}

int string_contains_case_insensitive(const char *haystack, const char *needle) {
    size_t needle_length;
    size_t i;

    if (haystack == NULL || needle == NULL) {
        return 0;
    }
    needle_length = strlen(needle);
    if (needle_length == 0) {
        return 1;
    }

    for (; *haystack != '\0'; haystack++) {
        for (i = 0; i < needle_length; i++) {
            if (haystack[i] == '\0' ||
                tolower((unsigned char)haystack[i]) !=
                    tolower((unsigned char)needle[i])) {
                break;
            }
        }
        if (i == needle_length) {
            return 1;
        }
    }
    return 0;
}

int has_suffix_case_insensitive(const char *value, const char *suffix) {
    size_t value_length;
    size_t suffix_length;
    size_t i;

    if (value == NULL || suffix == NULL) {
        return 0;
    }
    value_length = strlen(value);
    suffix_length = strlen(suffix);
    if (suffix_length > value_length) {
        return 0;
    }

    for (i = 0; i < suffix_length; i++) {
        if (tolower((unsigned char)value[value_length - suffix_length + i]) !=
            tolower((unsigned char)suffix[i])) {
            return 0;
        }
    }
    return 1;
}
