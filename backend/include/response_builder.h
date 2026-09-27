#ifndef REPOONBOARD_RESPONSE_BUILDER_H
#define REPOONBOARD_RESPONSE_BUILDER_H

#include <cjson/cJSON.h>
#include "github_client.h"

cJSON *response_build(const repo_snapshot_t *snapshot);

#endif
