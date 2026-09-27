#ifndef REPOONBOARD_CONFIG_H
#define REPOONBOARD_CONFIG_H

typedef struct {
    int port;
    char github_api_url[512];
} app_config_t;

void config_load(app_config_t *config);

#endif
