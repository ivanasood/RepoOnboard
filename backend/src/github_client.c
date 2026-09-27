#include "github_client.h"

#include <ctype.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cjson/cJSON.h>

#include "json_utils.h"
#include "repo_analyzer.h"

#define MAX_HTTP_RESPONSE (1024 * 1024)

typedef struct {
    char *data;
    size_t size;
    size_t capacity;
    int overflow;
} http_buffer_t;

typedef struct {
    char *data;
    size_t size;
    long status;
    CURLcode curl_code;
} http_response_t;

static size_t write_response(void *contents, size_t size, size_t count,
                             void *user_data) {
    http_buffer_t *buffer = user_data;
    size_t incoming = size * count;

    if (incoming > buffer->capacity - buffer->size - 1) {
        buffer->overflow = 1;
        return 0;
    }
    memcpy(buffer->data + buffer->size, contents, incoming);
    buffer->size += incoming;
    buffer->data[buffer->size] = '\0';
    return incoming;
}

static void free_http_response(http_response_t *response) {
    if (response != NULL) {
        free(response->data);
        response->data = NULL;
    }
}

static int http_get(const char *url, const char *accept,
                    http_response_t *response) {
    CURL *curl;
    CURLcode result;
    http_buffer_t buffer;
    struct curl_slist *headers = NULL;

    memset(response, 0, sizeof(*response));
    response->status = 0;
    response->curl_code = CURLE_FAILED_INIT;
    buffer.capacity = MAX_HTTP_RESPONSE;
    buffer.size = 0;
    buffer.overflow = 0;
    buffer.data = calloc(buffer.capacity, 1);
    if (buffer.data == NULL) {
        return 0;
    }

    curl = curl_easy_init();
    if (curl == NULL) {
        free(buffer.data);
        return 0;
    }
    headers = curl_slist_append(headers, "User-Agent: RepoOnboard-MVP/1.0");
    headers = curl_slist_append(headers, "Accept: application/vnd.github+json");
    if (accept != NULL) {
        headers = curl_slist_append(headers, accept);
    }
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_response);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
    result = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->status);
    response->curl_code = result;
    response->data = buffer.data;
    response->size = buffer.size;
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return result == CURLE_OK && !buffer.overflow;
}

static int valid_segment(const char *value) {
    size_t i;
    if (value == NULL || value[0] == '\0') {
        return 0;
    }
    for (i = 0; value[i] != '\0'; i++) {
        unsigned char c = (unsigned char)value[i];
        if (isspace(c) || c == '?' || c == '#' || c == '\\' ||
            c == ':' || c == '%') {
            return 0;
        }
    }
    return 1;
}

github_result_t github_parse_url(const char *input,
                                 char *owner,
                                 size_t owner_size,
                                 char *name,
                                 size_t name_size,
                                 char *canonical_url,
                                 size_t canonical_size) {
    const char *prefix_https = "https://github.com/";
    const char *prefix_http = "http://github.com/";
    const char *path;
    char copy[MAX_URL_LENGTH];
    char *slash;
    char *extra;

    if (input == NULL || owner == NULL || name == NULL ||
        canonical_url == NULL) {
        return GITHUB_INVALID_URL;
    }
    if (strncmp(input, prefix_https, strlen(prefix_https)) == 0) {
        path = input + strlen(prefix_https);
    } else if (strncmp(input, prefix_http, strlen(prefix_http)) == 0) {
        path = input + strlen(prefix_http);
    } else {
        return GITHUB_INVALID_URL;
    }
    if (strlen(path) >= sizeof(copy)) {
        return GITHUB_INVALID_URL;
    }
    copy_string(copy, sizeof(copy), path);
    while (strlen(copy) > 0 && copy[strlen(copy) - 1] == '/') {
        copy[strlen(copy) - 1] = '\0';
    }
    slash = strchr(copy, '/');
    if (slash == NULL) {
        return GITHUB_INVALID_URL;
    }
    *slash = '\0';
    extra = strchr(slash + 1, '/');
    if (extra != NULL) {
        if (strcmp(extra, "/") != 0 &&
            !(strlen(extra) == 4 && strcmp(extra, ".git") == 0)) {
            return GITHUB_INVALID_URL;
        }
        *extra = '\0';
    }
    if (!valid_segment(copy) || !valid_segment(slash + 1) ||
        strlen(copy) >= owner_size || strlen(slash + 1) >= name_size) {
        return GITHUB_INVALID_URL;
    }
    copy_string(owner, owner_size, copy);
    copy_string(name, name_size, slash + 1);
    if (has_suffix_case_insensitive(name, ".git")) {
        name[strlen(name) - 4] = '\0';
    }
    if (name[0] == '\0') {
        return GITHUB_INVALID_URL;
    }
    snprintf(canonical_url, canonical_size, "https://github.com/%s/%s",
             owner, name);
    return GITHUB_OK;
}

static void error_from_github_status(long status,
                                     char *error_message,
                                     size_t error_size) {
    if (status == 404) {
        set_error(error_message, error_size, "GitHub repository not found");
    } else if (status == 403) {
        set_error(error_message, error_size,
                  "GitHub request was rate-limited or forbidden");
    } else {
        snprintf(error_message, error_size,
                 "GitHub request failed with HTTP status %ld", status);
    }
}

static int join_url(char *destination, size_t destination_size,
                    const char *base, const char *path) {
    size_t base_length;
    if (base == NULL || path == NULL) {
        return 0;
    }
    base_length = strlen(base);
    while (base_length > 0 && base[base_length - 1] == '/') {
        base_length--;
    }
    return snprintf(destination, destination_size, "%.*s/%s",
                    (int)base_length, base, path) < (int)destination_size;
}

static int parse_metadata(repo_snapshot_t *snapshot, const char *json,
                          size_t json_length) {
    cJSON *root;
    cJSON *value;

    root = cJSON_ParseWithLength(json, json_length);
    if (root == NULL) {
        return 0;
    }
    value = cJSON_GetObjectItemCaseSensitive(root, "default_branch");
    if (!cJSON_IsString(value)) {
        cJSON_Delete(root);
        return 0;
    }
    copy_string(snapshot->default_branch, sizeof(snapshot->default_branch),
                value->valuestring);
    value = cJSON_GetObjectItemCaseSensitive(root, "description");
    if (cJSON_IsString(value)) {
        copy_string(snapshot->description, sizeof(snapshot->description),
                    value->valuestring);
    }
    value = cJSON_GetObjectItemCaseSensitive(root, "stargazers_count");
    if (cJSON_IsNumber(value)) {
        snapshot->stars = value->valueint;
    }
    cJSON_Delete(root);
    return 1;
}

static int is_readme_path(const char *path) {
    return string_contains_case_insensitive(path, "readme") &&
           (has_suffix_case_insensitive(path, ".md") ||
            has_suffix_case_insensitive(path, ".mdx") ||
            has_suffix_case_insensitive(path, ".rst") ||
            strchr(path, '.') == NULL);
}

static void select_paths_from_tree(repo_snapshot_t *snapshot,
                                   const char *tree_json,
                                   size_t tree_json_length,
                                   char *readme_path,
                                   size_t readme_path_size) {
    cJSON *root;
    cJSON *tree;
    cJSON *item;
    cJSON *path;
    cJSON *type;
    size_t i;

    root = cJSON_ParseWithLength(tree_json, tree_json_length);
    if (root == NULL) {
        return;
    }
    tree = cJSON_GetObjectItemCaseSensitive(root, "tree");
    if (!cJSON_IsArray(tree)) {
        cJSON_Delete(root);
        return;
    }
    cJSON_ArrayForEach(item, tree) {
        const char *path_value;
        type = cJSON_GetObjectItemCaseSensitive(item, "type");
        path = cJSON_GetObjectItemCaseSensitive(item, "path");
        if (!cJSON_IsString(type) || !cJSON_IsString(path) ||
            strcmp(type->valuestring, "blob") != 0) {
            continue;
        }
        path_value = path->valuestring;
        if (is_readme_path(path_value) && readme_path[0] == '\0') {
            copy_string(readme_path, readme_path_size, path_value);
        }
    }
    for (i = 0; i < snapshot->important_file_count; i++) {
        if (is_readme_path(snapshot->important_files[i])) {
            continue;
        }
        if (snapshot->selected_file_count >= MAX_SELECTED_FILES) {
            break;
        }
        copy_string(snapshot->selected_files[snapshot->selected_file_count].path,
                    sizeof(snapshot->selected_files[0].path),
                    snapshot->important_files[i]);
        snapshot->selected_files[snapshot->selected_file_count].content[0] =
            '\0';
        snapshot->selected_file_count++;
    }
    cJSON_Delete(root);
}

static void encode_url_part(CURL *curl, const char *value,
                            char *encoded, size_t encoded_size) {
    char *escaped = curl_easy_escape(curl, value, 0);
    if (escaped != NULL) {
        copy_string(encoded, encoded_size, escaped);
        curl_free(escaped);
    } else {
        encoded[0] = '\0';
    }
}

static int fetch_raw_file(const char *owner, const char *name,
                          const char *branch, const char *path,
                          char *content, size_t content_size,
                          size_t *content_length) {
    CURL *curl = curl_easy_init();
    char *owner_encoded;
    char *name_encoded;
    char *branch_encoded;
    char *path_encoded;
    char url[MAX_URL_LENGTH * 2];
    http_response_t response;
    int ok;

    if (curl == NULL) {
        return 0;
    }
    owner_encoded = curl_easy_escape(curl, owner, 0);
    name_encoded = curl_easy_escape(curl, name, 0);
    branch_encoded = curl_easy_escape(curl, branch, 0);
    path_encoded = curl_easy_escape(curl, path, 0);
    if (owner_encoded == NULL || name_encoded == NULL ||
        branch_encoded == NULL || path_encoded == NULL) {
        curl_free(owner_encoded);
        curl_free(name_encoded);
        curl_free(branch_encoded);
        curl_free(path_encoded);
        curl_easy_cleanup(curl);
        return 0;
    }
    snprintf(url, sizeof(url),
             "https://raw.githubusercontent.com/%s/%s/%s/%s",
             owner_encoded, name_encoded, branch_encoded, path_encoded);
    curl_free(owner_encoded);
    curl_free(name_encoded);
    curl_free(branch_encoded);
    curl_free(path_encoded);
    curl_easy_cleanup(curl);

    if (!http_get(url, "Accept: text/plain", &response)) {
        free_http_response(&response);
        return 0;
    }
    if (response.status < 200 || response.status >= 300) {
        free_http_response(&response);
        return 0;
    }
    *content_length = response.size;
    if (*content_length >= content_size) {
        *content_length = content_size - 1;
    }
    memcpy(content, response.data, *content_length);
    content[*content_length] = '\0';
    ok = 1;
    free_http_response(&response);
    return ok;
}

github_result_t github_fetch_snapshot(const char *api_base_url,
                                      const char *owner,
                                      const char *name,
                                      const char *canonical_url,
                                      repo_snapshot_t *snapshot,
                                      char *error_message,
                                      size_t error_size) {
    char endpoint[MAX_URL_LENGTH * 2];
    char tree_endpoint[MAX_URL_LENGTH * 3];
    char readme_path[MAX_PATH_LENGTH] = {0};
    char content[MAX_README_LENGTH + 1];
    size_t content_length;
    http_response_t response;
    CURL *url_curl;
    char *owner_encoded;
    char *name_encoded;
    char *branch_encoded;
    github_result_t result = GITHUB_OK;

    memset(snapshot, 0, sizeof(*snapshot));
    copy_string(snapshot->owner, sizeof(snapshot->owner), owner);
    copy_string(snapshot->name, sizeof(snapshot->name), name);
    copy_string(snapshot->url, sizeof(snapshot->url), canonical_url);

    if (!join_url(endpoint, sizeof(endpoint), api_base_url,
                  "repos") ||
        snprintf(endpoint + strlen(endpoint),
                 sizeof(endpoint) - strlen(endpoint), "/%s/%s",
                 owner, name) >= (int)(sizeof(endpoint) - strlen(endpoint))) {
        set_error(error_message, error_size, "GitHub endpoint is too long");
        return GITHUB_REQUEST_FAILED;
    }
    if (!http_get(endpoint, NULL, &response)) {
        set_error(error_message, error_size, "Could not reach GitHub");
        free_http_response(&response);
        return GITHUB_REQUEST_FAILED;
    }
    if (response.status == 404) {
        error_from_github_status(response.status, error_message, error_size);
        free_http_response(&response);
        return GITHUB_NOT_FOUND;
    }
    if (response.status < 200 || response.status >= 300 ||
        !parse_metadata(snapshot, response.data, response.size)) {
        error_from_github_status(response.status, error_message, error_size);
        free_http_response(&response);
        return response.status == 404 ? GITHUB_NOT_FOUND
                                      : GITHUB_MALFORMED_RESPONSE;
    }
    free_http_response(&response);

    url_curl = curl_easy_init();
    if (url_curl == NULL) {
        set_error(error_message, error_size, "Could not initialize URL encoder");
        return GITHUB_REQUEST_FAILED;
    }
    owner_encoded = curl_easy_escape(url_curl, owner, 0);
    name_encoded = curl_easy_escape(url_curl, name, 0);
    branch_encoded = curl_easy_escape(url_curl, snapshot->default_branch, 0);
    if (owner_encoded == NULL || name_encoded == NULL ||
        branch_encoded == NULL) {
        curl_free(owner_encoded);
        curl_free(name_encoded);
        curl_free(branch_encoded);
        curl_easy_cleanup(url_curl);
        set_error(error_message, error_size, "Could not encode GitHub URL");
        return GITHUB_REQUEST_FAILED;
    }
    snprintf(tree_endpoint, sizeof(tree_endpoint),
             "%s/repos/%s/%s/git/trees/%s?recursive=1",
             api_base_url, owner_encoded, name_encoded, branch_encoded);
    curl_free(owner_encoded);
    curl_free(name_encoded);
    curl_free(branch_encoded);
    curl_easy_cleanup(url_curl);

    if (!http_get(tree_endpoint, NULL, &response)) {
        set_error(error_message, error_size, "Could not fetch GitHub file tree");
        free_http_response(&response);
        return GITHUB_REQUEST_FAILED;
    }
    if (response.status < 200 || response.status >= 300) {
        error_from_github_status(response.status, error_message, error_size);
        free_http_response(&response);
        return GITHUB_REQUEST_FAILED;
    }
    repo_analyze_tree(snapshot, response.data, response.size);
    select_paths_from_tree(snapshot, response.data, response.size,
                           readme_path, sizeof(readme_path));
    free_http_response(&response);

    if (readme_path[0] != '\0' &&
        fetch_raw_file(owner, name, snapshot->default_branch, readme_path,
                       content, sizeof(content), &content_length)) {
        repo_add_file_content(snapshot, readme_path, content, content_length);
    } else {
        set_error(error_message, error_size,
                  "Repository has no readable README");
        result = GITHUB_MISSING_README;
    }

    /* Fetch only the selected non-README files and cap each response. */
    for (size_t i = 0; i < snapshot->selected_file_count; i++) {
        if (snapshot->selected_files[i].path[0] == '\0') {
            continue;
        }
        if (fetch_raw_file(owner, name, snapshot->default_branch,
                           snapshot->selected_files[i].path,
                           content, sizeof(content), &content_length)) {
            repo_add_file_content(snapshot, snapshot->selected_files[i].path,
                                  content, content_length);
        }
    }
    return result;
}
