#ifndef REPOONBOARD_GITHUB_CLIENT_H
#define REPOONBOARD_GITHUB_CLIENT_H

#include <stddef.h>

#define MAX_OWNER_LENGTH 128
#define MAX_REPOSITORY_LENGTH 128
#define MAX_BRANCH_LENGTH 256
#define MAX_DESCRIPTION_LENGTH 512
#define MAX_URL_LENGTH 512
#define MAX_PATH_LENGTH 512
#define MAX_README_LENGTH 6000
#define MAX_FILE_CONTENT_LENGTH 2200
#define MAX_SELECTED_FILES 12
#define MAX_TECHNOLOGIES 32

/* Maximum unique directory entries stored per structure category. */
#define MAX_STRUCTURE_DIRS 20

typedef struct {
    char path[MAX_PATH_LENGTH];
    char content[MAX_FILE_CONTENT_LENGTH + 1];
    size_t content_length;
} snapshot_file_t;

/*
 * Concise, deterministic summary of how a repository is organised.
 * Derived solely from the file paths returned by the GitHub tree API;
 * no architectural inference is performed.
 */
typedef struct {
    /* Unique directories immediately under the repository root. */
    char top_level_dirs[MAX_STRUCTURE_DIRS][MAX_PATH_LENGTH];
    size_t top_level_dir_count;

    /* Directories containing at least one source file. */
    char source_dirs[MAX_STRUCTURE_DIRS][MAX_PATH_LENGTH];
    size_t source_dir_count;

    /* Directories containing at least one test file. */
    char test_dirs[MAX_STRUCTURE_DIRS][MAX_PATH_LENGTH];
    size_t test_dir_count;

    /* Directories containing at least one documentation file. */
    char doc_dirs[MAX_STRUCTURE_DIRS][MAX_PATH_LENGTH];
    size_t doc_dir_count;

    /* Directories containing at least one configuration file. */
    char config_dirs[MAX_STRUCTURE_DIRS][MAX_PATH_LENGTH];
    size_t config_dir_count;
} repo_structure_t;

typedef struct {
    char owner[MAX_OWNER_LENGTH];
    char name[MAX_REPOSITORY_LENGTH];
    char url[MAX_URL_LENGTH];
    char default_branch[MAX_BRANCH_LENGTH];
    char description[MAX_DESCRIPTION_LENGTH];
    int stars;
    size_t tree_count;
    size_t source_file_count;
    size_t test_file_count;
    size_t documentation_file_count;
    size_t configuration_file_count;
    size_t directory_count;
    char readme[MAX_README_LENGTH + 1];
    size_t readme_length;
    char technologies[MAX_TECHNOLOGIES][64];
    size_t technology_count;
    char important_files[MAX_SELECTED_FILES][MAX_PATH_LENGTH];
    size_t important_file_count;
    snapshot_file_t selected_files[MAX_SELECTED_FILES];
    size_t selected_file_count;
    /* Repository structure derived from the tree walk. */
    repo_structure_t structure;
} repo_snapshot_t;

typedef enum {
    GITHUB_OK = 0,
    GITHUB_INVALID_URL,
    GITHUB_NOT_FOUND,
    GITHUB_REQUEST_FAILED,
    GITHUB_MALFORMED_RESPONSE,
    GITHUB_MISSING_README
} github_result_t;

github_result_t github_parse_url(const char *input,
                                 char *owner,
                                 size_t owner_size,
                                 char *name,
                                 size_t name_size,
                                 char *canonical_url,
                                 size_t canonical_size);

github_result_t github_fetch_snapshot(const char *api_base_url,
                                      const char *owner,
                                      const char *name,
                                      const char *canonical_url,
                                      repo_snapshot_t *snapshot,
                                      char *error_message,
                                      size_t error_size);

#endif
