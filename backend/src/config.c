#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "json_utils.h"

void config_load(app_config_t *config) {
    memset(config, 0, sizeof(*config));
    config->port = 8080;
    if (getenv("PORT") != NULL && getenv("PORT")[0] != '\0') {
        int parsed_port = atoi(getenv("PORT"));
        if (parsed_port > 0 && parsed_port < 65536) {
            config->port = parsed_port;
        }
    }
    copy_string(config->github_api_url, sizeof(config->github_api_url),
                getenv("GITHUB_API_URL"));
    if (config->github_api_url[0] == '\0') {
        copy_string(config->github_api_url, sizeof(config->github_api_url),
                    "https://api.github.com");
    }
}
