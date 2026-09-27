#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cjson/cJSON.h>
#include <curl/curl.h>

#include "analysis_service.h"
#include "config.h"

static void write_json_line(cJSON *value) {
    char *text = cJSON_PrintUnformatted(value);
    if (text != NULL) {
        fputs(text, stdout);
        fputc('\n', stdout);
        fflush(stdout);
    }
    free(text);
}

static void send_jsonrpc_error(cJSON *id, int code, const char *message) {
    cJSON *response = cJSON_CreateObject();
    cJSON *error = cJSON_CreateObject();
    if (response == NULL || error == NULL) {
        cJSON_Delete(response);
        cJSON_Delete(error);
        return;
    }
    cJSON_AddStringToObject(response, "jsonrpc", "2.0");
    cJSON_AddItemToObject(response, "id",
                          id == NULL ? cJSON_CreateNull() : cJSON_Duplicate(id, 1));
    cJSON_AddNumberToObject(error, "code", code);
    cJSON_AddStringToObject(error, "message", message);
    cJSON_AddItemToObject(response, "error", error);
    write_json_line(response);
    cJSON_Delete(response);
}

static void send_jsonrpc_result(cJSON *id, cJSON *result) {
    cJSON *response = cJSON_CreateObject();
    if (response == NULL) {
        cJSON_Delete(result);
        return;
    }
    cJSON_AddStringToObject(response, "jsonrpc", "2.0");
    cJSON_AddItemToObject(response, "id",
                          id == NULL ? cJSON_CreateNull() : cJSON_Duplicate(id, 1));
    cJSON_AddItemToObject(response, "result", result);
    write_json_line(response);
    cJSON_Delete(response);
}

static cJSON *initialize_result(void) {
    cJSON *result = cJSON_CreateObject();
    cJSON *capabilities = cJSON_CreateObject();
    cJSON *tools = cJSON_CreateObject();
    cJSON *server_info = cJSON_CreateObject();
    if (result == NULL || capabilities == NULL || tools == NULL ||
        server_info == NULL) {
        cJSON_Delete(result);
        cJSON_Delete(capabilities);
        cJSON_Delete(tools);
        cJSON_Delete(server_info);
        return NULL;
    }
    cJSON_AddBoolToObject(tools, "listChanged", 0);
    cJSON_AddItemToObject(capabilities, "tools", tools);
    cJSON_AddStringToObject(server_info, "name", "repoonboard-mcp");
    cJSON_AddStringToObject(server_info, "version", "0.1.0");
    cJSON_AddStringToObject(result, "protocolVersion", "2024-11-05");
    cJSON_AddItemToObject(result, "capabilities", capabilities);
    cJSON_AddItemToObject(result, "serverInfo", server_info);
    return result;
}

static cJSON *tools_list_result(void) {
    cJSON *result = cJSON_CreateObject();
    cJSON *tools = cJSON_CreateArray();
    cJSON *tool = cJSON_CreateObject();
    cJSON *schema = cJSON_CreateObject();
    cJSON *properties = cJSON_CreateObject();
    cJSON *github_url = cJSON_CreateObject();
    cJSON *required = cJSON_CreateArray();

    if (result == NULL || tools == NULL || tool == NULL || schema == NULL ||
        properties == NULL || github_url == NULL || required == NULL) {
        cJSON_Delete(result);
        cJSON_Delete(tools);
        cJSON_Delete(tool);
        cJSON_Delete(schema);
        cJSON_Delete(properties);
        cJSON_Delete(github_url);
        cJSON_Delete(required);
        return NULL;
    }
    cJSON_AddStringToObject(tool, "name", "analyze_github_repository");
    cJSON_AddStringToObject(
        tool, "description",
        "Fetch and analyze a public GitHub repository using RepoOnboard's "
        "bounded C analysis. Returns repository metadata, deterministic "
        "technology/file statistics, README context, and selected file "
        "contents. It never executes repository code.");
    cJSON_AddStringToObject(schema, "type", "object");
    cJSON_AddStringToObject(github_url, "type", "string");
    cJSON_AddStringToObject(
        github_url, "description",
        "Public GitHub URL, for example https://github.com/owner/repository");
    cJSON_AddItemToObject(properties, "github_url", github_url);
    cJSON_AddItemToArray(required, cJSON_CreateString("github_url"));
    cJSON_AddItemToObject(schema, "properties", properties);
    cJSON_AddItemToObject(schema, "required", required);
    cJSON_AddBoolToObject(schema, "additionalProperties", 0);
    cJSON_AddItemToObject(tool, "inputSchema", schema);
    cJSON_AddItemToArray(tools, tool);
    cJSON_AddItemToObject(result, "tools", tools);
    return result;
}

static cJSON *tool_error_result(const char *code, const char *message) {
    cJSON *result = cJSON_CreateObject();
    cJSON *content = cJSON_CreateArray();
    cJSON *item = cJSON_CreateObject();
    cJSON *error = cJSON_CreateObject();

    if (result == NULL || content == NULL || item == NULL || error == NULL) {
        cJSON_Delete(result);
        cJSON_Delete(content);
        cJSON_Delete(item);
        cJSON_Delete(error);
        return NULL;
    }
    cJSON_AddStringToObject(item, "type", "text");
    cJSON_AddStringToObject(item, "text", message);
    cJSON_AddItemToArray(content, item);
    cJSON_AddItemToObject(result, "content", content);
    cJSON_AddBoolToObject(result, "isError", 1);
    cJSON_AddStringToObject(error, "code", code);
    cJSON_AddStringToObject(error, "message", message);
    cJSON_AddItemToObject(result, "structuredContent", error);
    return result;
}

static cJSON *tool_success_result(cJSON *analysis) {
    cJSON *result = cJSON_CreateObject();
    cJSON *content = cJSON_CreateArray();
    cJSON *item = cJSON_CreateObject();
    char *text;

    if (result == NULL || content == NULL || item == NULL) {
        cJSON_Delete(result);
        cJSON_Delete(content);
        cJSON_Delete(item);
        cJSON_Delete(analysis);
        return NULL;
    }
    text = cJSON_PrintUnformatted(analysis);
    if (text == NULL) {
        cJSON_Delete(result);
        cJSON_Delete(content);
        cJSON_Delete(item);
        cJSON_Delete(analysis);
        return NULL;
    }
    cJSON_AddStringToObject(item, "type", "text");
    cJSON_AddStringToObject(item, "text", text);
    free(text);
    cJSON_AddItemToArray(content, item);
    cJSON_AddItemToObject(result, "content", content);
    cJSON_AddBoolToObject(result, "isError", 0);
    cJSON_AddItemToObject(result, "structuredContent", analysis);
    return result;
}

static cJSON *handle_tool_call(const app_config_t *config, cJSON *params) {
    cJSON *name = cJSON_GetObjectItemCaseSensitive(params, "name");
    cJSON *arguments = cJSON_GetObjectItemCaseSensitive(params, "arguments");
    cJSON *github_url;
    cJSON *analysis = NULL;
    analysis_error_t error;
    analysis_result_t result;

    if (!cJSON_IsString(name) ||
        strcmp(name->valuestring, "analyze_github_repository") != 0) {
        return tool_error_result("UNKNOWN_TOOL", "Unknown RepoOnboard MCP tool");
    }
    if (!cJSON_IsObject(arguments)) {
        return tool_error_result("INVALID_ARGUMENTS",
                                 "Tool arguments must be a JSON object");
    }
    github_url = cJSON_GetObjectItemCaseSensitive(arguments, "github_url");
    if (!cJSON_IsString(github_url) || github_url->valuestring[0] == '\0') {
        return tool_error_result("INVALID_ARGUMENTS",
                                 "github_url must be a non-empty string");
    }
    result = analyze_repository(config, github_url->valuestring, &analysis,
                                &error);
    if (result != ANALYSIS_OK) {
        return tool_error_result(error.code, error.message);
    }
    return tool_success_result(analysis);
}

static void handle_message(const app_config_t *config, cJSON *request) {
    cJSON *id = cJSON_GetObjectItemCaseSensitive(request, "id");
    cJSON *method = cJSON_GetObjectItemCaseSensitive(request, "method");
    cJSON *params = cJSON_GetObjectItemCaseSensitive(request, "params");
    cJSON *result;

    if (!cJSON_IsString(method)) {
        if (id != NULL) {
            send_jsonrpc_error(id, -32600, "Invalid JSON-RPC request");
        }
        return;
    }
    if (strcmp(method->valuestring, "notifications/initialized") == 0 ||
        strcmp(method->valuestring, "notifications/cancelled") == 0) {
        return;
    }
    if (strcmp(method->valuestring, "initialize") == 0) {
        result = initialize_result();
    } else if (strcmp(method->valuestring, "tools/list") == 0) {
        result = tools_list_result();
    } else if (strcmp(method->valuestring, "tools/call") == 0) {
        if (!cJSON_IsObject(params)) {
            send_jsonrpc_error(id, -32602,
                               "tools/call params must be an object");
            return;
        }
        result = handle_tool_call(config, params);
    } else if (strcmp(method->valuestring, "ping") == 0) {
        result = cJSON_CreateObject();
    } else {
        send_jsonrpc_error(id, -32601, "Method not found");
        return;
    }
    if (result == NULL) {
        send_jsonrpc_error(id, -32603, "Could not allocate MCP response");
        return;
    }
    send_jsonrpc_result(id, result);
}

int main(void) {
    app_config_t config;
    char *line = NULL;
    size_t capacity = 0;
    ssize_t length;

    config_load(&config);
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0) {
        fprintf(stderr, "RepoOnboard MCP could not initialize libcurl\n");
        return 1;
    }
    setvbuf(stdout, NULL, _IOLBF, 0);
    fprintf(stderr, "RepoOnboard MCP server ready on stdio\n");
    while ((length = getline(&line, &capacity, stdin)) >= 0) {
        cJSON *request;
        if (length == 0) {
            continue;
        }
        request = cJSON_ParseWithLength(line, (size_t)length);
        if (request == NULL) {
            send_jsonrpc_error(NULL, -32700, "Parse error");
            continue;
        }
        handle_message(&config, request);
        cJSON_Delete(request);
    }
    free(line);
    curl_global_cleanup();
    return 0;
}
