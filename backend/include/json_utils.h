#ifndef REPOONBOARD_JSON_UTILS_H
#define REPOONBOARD_JSON_UTILS_H

#include <stddef.h>

void copy_string(char *destination, size_t destination_size, const char *source);
void set_error(char *destination, size_t destination_size, const char *message);
int string_contains_case_insensitive(const char *haystack, const char *needle);
int has_suffix_case_insensitive(const char *value, const char *suffix);

#endif
