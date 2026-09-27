#ifndef REPOONBOARD_ANALYSIS_SERVICE_H
#define REPOONBOARD_ANALYSIS_SERVICE_H

#include <cjson/cJSON.h>
#include <stddef.h>

#include "config.h"

typedef enum {
    ANALYSIS_OK = 0,
    ANALYSIS_INVALID_URL,
    ANALYSIS_REPOSITORY_NOT_FOUND,
    ANALYSIS_GITHUB_ERROR,
    ANALYSIS_INTERNAL_ERROR
} analysis_result_t;

typedef struct {
    const char *code;
    char message[512];
    int http_status;
} analysis_error_t;

analysis_result_t analyze_repository(const app_config_t *config,
                                     const char *github_url,
                                     cJSON **result_out,
                                     analysis_error_t *error);

#endif