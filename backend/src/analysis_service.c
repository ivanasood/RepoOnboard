#include "analysis_service.h"

#include <string.h>

#include "github_client.h"
#include "json_utils.h"
#include "response_builder.h"

static void set_analysis_error(analysis_error_t *error,
                               const char *code,
                               int http_status,
                               const char *message) {
    error->code = code;
    error->http_status = http_status;
    copy_string(error->message, sizeof(error->message), message);
}

analysis_result_t analyze_repository(const app_config_t *config,
                                     const char *github_url,
                                     cJSON **result_out,
                                     analysis_error_t *error) {
    char owner[MAX_OWNER_LENGTH];
    char name[MAX_REPOSITORY_LENGTH];
    char canonical_url[MAX_URL_LENGTH];
    char github_error[512];
    repo_snapshot_t snapshot;
    github_result_t github_result;

    *result_out = NULL;
    memset(error, 0, sizeof(*error));
    if (github_parse_url(github_url, owner, sizeof(owner), name, sizeof(name),
                         canonical_url, sizeof(canonical_url)) != GITHUB_OK) {
        set_analysis_error(
            error, "INVALID_GITHUB_URL", 400,
            "Use a public URL such as https://github.com/owner/repository");
        return ANALYSIS_INVALID_URL;
    }

    github_result = github_fetch_snapshot(
        config->github_api_url, owner, name, canonical_url, &snapshot,
        github_error, sizeof(github_error));
    if (github_result == GITHUB_NOT_FOUND) {
        set_analysis_error(error, "REPOSITORY_NOT_FOUND", 404, github_error);
        return ANALYSIS_REPOSITORY_NOT_FOUND;
    }
    if (github_result == GITHUB_REQUEST_FAILED ||
        github_result == GITHUB_MALFORMED_RESPONSE) {
        set_analysis_error(error, "GITHUB_REQUEST_FAILED", 502, github_error);
        return ANALYSIS_GITHUB_ERROR;
    }

    *result_out = response_build(&snapshot);
    if (*result_out == NULL) {
        set_analysis_error(error, "RESPONSE_BUILD_FAILED", 500,
                           "Could not build repository analysis response");
        return ANALYSIS_INTERNAL_ERROR;
    }
    return ANALYSIS_OK;
}