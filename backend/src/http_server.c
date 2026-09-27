#include "http_server.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cjson/cJSON.h>

#include "analysis_service.h"
#include "github_client.h"
#include "json_utils.h"
#include "response_builder.h"

#define REQUEST_BUFFER_SIZE (1024 * 1024)
#define MAX_RESPONSE_SIZE (2 * 1024 * 1024)

typedef struct {
    char data[REQUEST_BUFFER_SIZE];
    size_t size;
    size_t body_offset;
    size_t body_length;
    char method[16];
    char path[128];
} http_request_t;

static ssize_t find_header_end(const char *data, size_t length) {
    size_t i;
    if (length < 4) {
        return -1;
    }
    for (i = 0; i + 3 < length; i++) {
        if (data[i] == '\r' && data[i + 1] == '\n' &&
            data[i + 2] == '\r' && data[i + 3] == '\n') {
            return (ssize_t)i;
        }
    }
    return -1;
}

static int parse_content_length(const char *headers, size_t length,
                                size_t *content_length) {
    const char *cursor = headers;
    const char *end = headers + length;
    const char *marker;
    char number[32];
    char *number_end;
    unsigned long parsed;

    *content_length = 0;
    while (cursor < end) {
        marker = strstr(cursor, "\r\n");
        if (marker == NULL || marker > end) {
            marker = end;
        }
        if ((size_t)(marker - cursor) >= strlen("Content-Length:") &&
            strncasecmp(cursor, "Content-Length:", strlen("Content-Length:"))
                == 0) {
            size_t number_length = (size_t)(marker - cursor) -
                                   strlen("Content-Length:");
            if (number_length >= sizeof(number)) {
                return 0;
            }
            memcpy(number, cursor + strlen("Content-Length:"), number_length);
            number[number_length] = '\0';
            parsed = strtoul(number, &number_end, 10);
            while (*number_end == ' ' || *number_end == '\t') {
                number_end++;
            }
            if (*number_end != '\0' || parsed >= REQUEST_BUFFER_SIZE) {
                return 0;
            }
            *content_length = (size_t)parsed;
            return 1;
        }
        if (marker == end) {
            break;
        }
        cursor = marker + 2;
    }
    return 1;
}

static int read_request(int client, http_request_t *request) {
    ssize_t header_end;
    size_t content_length;
    ssize_t received;
    size_t target_size;

    memset(request, 0, sizeof(*request));
    while ((header_end = find_header_end(request->data, request->size)) < 0) {
        if (request->size == sizeof(request->data)) {
            return 0;
        }
        received = recv(client, request->data + request->size,
                        sizeof(request->data) - request->size, 0);
        if (received <= 0) {
            return 0;
        }
        request->size += (size_t)received;
    }
    request->body_offset = (size_t)header_end + 4;
    if (!parse_content_length(request->data, request->body_offset,
                              &content_length)) {
        return 0;
    }
    if (content_length >= REQUEST_BUFFER_SIZE - request->body_offset) {
        return 0;
    }
    target_size = request->body_offset + content_length;
    while (request->size < target_size) {
        received = recv(client, request->data + request->size,
                        sizeof(request->data) - request->size, 0);
        if (received <= 0) {
            return 0;
        }
        request->size += (size_t)received;
    }
    request->body_length = content_length;
    if (sscanf(request->data, "%15s %127s", request->method,
               request->path) != 2) {
        return 0;
    }
    return 1;
}

static void send_json(int client, int status, const char *body) {
    const char *status_text = status == 200 ? "OK" :
                              status == 400 ? "Bad Request" :
                              status == 404 ? "Not Found" :
                              status == 405 ? "Method Not Allowed" :
                              status == 502 ? "Bad Gateway" :
                              status == 503 ? "Service Unavailable" :
                              "Internal Server Error";
    char header[1024];
    size_t body_length = strlen(body);
    int header_length = snprintf(
        header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: application/json; charset=utf-8\r\n"
        "Content-Length: %zu\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Connection: close\r\n\r\n",
        status, status_text, body_length);
    if (header_length > 0) {
        send(client, header, (size_t)header_length, 0);
        send(client, body, body_length, 0);
    }
}

static void send_error(int client, int status, const char *code,
                       const char *message) {
    cJSON *root = cJSON_CreateObject();
    cJSON *error = cJSON_CreateObject();
    char *body;
    if (root == NULL || error == NULL) {
        cJSON_Delete(root);
        cJSON_Delete(error);
        send_json(client, 500, "{\"success\":false,\"error\":{\"code\":\"INTERNAL_ERROR\",\"message\":\"response allocation failed\"}}");
        return;
    }
    cJSON_AddBoolToObject(root, "success", 0);
    cJSON_AddStringToObject(error, "code", code);
    cJSON_AddStringToObject(error, "message", message);
    cJSON_AddItemToObject(root, "error", error);
    body = cJSON_PrintUnformatted(root);
    if (body != NULL) {
        send_json(client, status, body);
    }
    free(body);
    cJSON_Delete(root);
}

static void send_success(int client, cJSON *result) {
    char *body = cJSON_PrintUnformatted(result);
    if (body == NULL || strlen(body) > MAX_RESPONSE_SIZE) {
        free(body);
        send_error(client, 500, "RESPONSE_TOO_LARGE",
                   "Analysis response exceeded the response limit");
        return;
    }
    send_json(client, 200, body);
    free(body);
}

static void handle_health(int client, const app_config_t *config) {
    cJSON *root = cJSON_CreateObject();
    char *body;
    cJSON_AddStringToObject(root, "status", "ok");
    cJSON_AddStringToObject(root, "service", "repoonboard-c-backend");
    cJSON_AddStringToObject(root, "mcp_server",
                            "backend/bin/repoonboard-mcp");
    body = cJSON_PrintUnformatted(root);
    send_json(client, 200, body == NULL ? "{\"status\":\"ok\"}" : body);
    free(body);
    cJSON_Delete(root);
}

static void handle_analyze(int client, const app_config_t *config,
                           const http_request_t *request) {
    cJSON *input;
    cJSON *url_value;
    cJSON *result = NULL;
    analysis_error_t error;

    input = cJSON_ParseWithLength(request->data + request->body_offset,
                                  request->body_length);
    if (input == NULL) {
        send_error(client, 400, "INVALID_JSON",
                   "Request body must be valid JSON");
        return;
    }
    url_value = cJSON_GetObjectItemCaseSensitive(input, "github_url");
    if (!cJSON_IsString(url_value) || url_value->valuestring[0] == '\0') {
        cJSON_Delete(input);
        send_error(client, 400, "INVALID_INPUT",
                   "github_url must be a non-empty string");
        return;
    }
    {
        analysis_result_t analysis_result =
            analyze_repository(config, url_value->valuestring, &result, &error);
        cJSON_Delete(input);
        if (analysis_result != ANALYSIS_OK) {
            send_error(client, error.http_status, error.code, error.message);
            return;
        }
    }
    send_success(client, result);
    cJSON_Delete(result);
}

static void handle_client(int client, const app_config_t *config) {
    http_request_t request;
    if (!read_request(client, &request)) {
        send_error(client, 400, "INVALID_HTTP_REQUEST",
                   "Could not parse the HTTP request");
        return;
    }
    if (strcmp(request.method, "OPTIONS") == 0) {
        send_json(client, 200, "{\"status\":\"ok\"}");
    } else if (strcmp(request.method, "GET") == 0 &&
               strcmp(request.path, "/api/health") == 0) {
        handle_health(client, config);
    } else if (strcmp(request.method, "POST") == 0 &&
               strcmp(request.path, "/api/analyze") == 0) {
        handle_analyze(client, config, &request);
    } else if (strcmp(request.path, "/api/health") == 0 ||
               strcmp(request.path, "/api/analyze") == 0) {
        send_error(client, 405, "METHOD_NOT_ALLOWED",
                   "Use the method defined for this endpoint");
    } else {
        send_error(client, 404, "NOT_FOUND", "Endpoint not found");
    }
}

int http_server_run(const app_config_t *config) {
    int server;
    int option = 1;
    struct sockaddr_in address;
    signal(SIGPIPE, SIG_IGN);
    server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0) {
        perror("socket");
        return 1;
    }
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons((uint16_t)config->port);
    if (bind(server, (struct sockaddr *)&address, sizeof(address)) < 0 ||
        listen(server, 16) < 0) {
        perror("bind/listen");
        close(server);
        return 1;
    }
    printf("RepoOnboard C backend listening on http://localhost:%d\n",
           config->port);
    fflush(stdout);
    for (;;) {
        int client = accept(server, NULL, NULL);
        if (client < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("accept");
            close(server);
            return 1;
        }
        handle_client(client, config);
        close(client);
    }
}
