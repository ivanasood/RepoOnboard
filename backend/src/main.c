#include <curl/curl.h>
#include <stdio.h>

#include "config.h"
#include "http_server.h"

int main(void) {
    app_config_t config;
    config_load(&config);
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0) {
        fprintf(stderr, "Could not initialize libcurl\n");
        return 1;
    }
    {
        int result = http_server_run(&config);
        curl_global_cleanup();
        return result;
    }
}
