#ifndef REPOONBOARD_REPO_ANALYZER_H
#define REPOONBOARD_REPO_ANALYZER_H

#include "github_client.h"
#include <stddef.h>

void repo_analyze_tree(repo_snapshot_t *snapshot,
                       const char *tree_json,
                       size_t tree_json_length);

void repo_add_file_content(repo_snapshot_t *snapshot,
                           const char *path,
                           const char *content,
                           size_t content_length);

#endif
