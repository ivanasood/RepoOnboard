#include "response_builder.h"

#include <cjson/cJSON.h>

static cJSON *string_array(const char values[][64], size_t count) {
    cJSON *array = cJSON_CreateArray();
    size_t i;
    for (i = 0; array != NULL && i < count; i++) {
        cJSON_AddItemToArray(array, cJSON_CreateString(values[i]));
    }
    return array;
}

static cJSON *path_array(const repo_snapshot_t *snapshot) {
    cJSON *array = cJSON_CreateArray();
    size_t i;
    for (i = 0; array != NULL && i < snapshot->important_file_count; i++) {
        cJSON_AddItemToArray(array,
                             cJSON_CreateString(snapshot->important_files[i]));
    }
    return array;
}

/*
 * Build a JSON array from a MAX_PATH_LENGTH-wide string matrix.
 * Used for all five structure directory lists.
 */
static cJSON *dir_array(const char dirs[][MAX_PATH_LENGTH], size_t count) {
    cJSON *array = cJSON_CreateArray();
    size_t i;
    for (i = 0; array != NULL && i < count; i++) {
        cJSON_AddItemToArray(array, cJSON_CreateString(dirs[i]));
    }
    return array;
}

cJSON *response_build(const repo_snapshot_t *snapshot) {
    cJSON *root = cJSON_CreateObject();
    cJSON *repository = cJSON_CreateObject();
    cJSON *c_analysis = cJSON_CreateObject();
    cJSON *statistics = cJSON_CreateObject();
    cJSON *structure = cJSON_CreateObject();
    cJSON *snapshot_json = cJSON_CreateObject();
    cJSON *selected_files = cJSON_CreateArray();
    size_t i;

    if (root == NULL || repository == NULL || c_analysis == NULL ||
        statistics == NULL || structure == NULL ||
        snapshot_json == NULL || selected_files == NULL) {
        cJSON_Delete(root);
        cJSON_Delete(repository);
        cJSON_Delete(c_analysis);
        cJSON_Delete(statistics);
        cJSON_Delete(structure);
        cJSON_Delete(snapshot_json);
        cJSON_Delete(selected_files);
        return NULL;
    }
    cJSON_AddStringToObject(repository, "owner", snapshot->owner);
    cJSON_AddStringToObject(repository, "name", snapshot->name);
    cJSON_AddStringToObject(repository, "url", snapshot->url);
    cJSON_AddStringToObject(repository, "default_branch",
                            snapshot->default_branch);
    cJSON_AddStringToObject(repository, "description", snapshot->description);
    cJSON_AddNumberToObject(repository, "stars", snapshot->stars);
    cJSON_AddItemToObject(root, "repository", repository);

    cJSON_AddItemToObject(c_analysis, "technologies",
                          string_array(snapshot->technologies,
                                       snapshot->technology_count));
    cJSON_AddItemToObject(c_analysis, "important_files",
                          path_array(snapshot));
    cJSON_AddNumberToObject(statistics, "files",
                            (double)snapshot->tree_count);
    cJSON_AddNumberToObject(statistics, "directories",
                            (double)snapshot->directory_count);
    cJSON_AddNumberToObject(statistics, "source_files",
                            (double)snapshot->source_file_count);
    cJSON_AddNumberToObject(statistics, "test_files",
                            (double)snapshot->test_file_count);
    cJSON_AddNumberToObject(statistics, "documentation_files",
                            (double)snapshot->documentation_file_count);
    cJSON_AddNumberToObject(statistics, "configuration_files",
                            (double)snapshot->configuration_file_count);
    cJSON_AddItemToObject(c_analysis, "file_statistics", statistics);
    cJSON_AddBoolToObject(c_analysis, "readme_available",
                          snapshot->readme_length > 0);

    /* ── structure: deterministic repository organisation summary ── */
    cJSON_AddItemToObject(structure, "top_level_directories",
                          dir_array(snapshot->structure.top_level_dirs,
                                    snapshot->structure.top_level_dir_count));
    cJSON_AddItemToObject(structure, "source_directories",
                          dir_array(snapshot->structure.source_dirs,
                                    snapshot->structure.source_dir_count));
    cJSON_AddItemToObject(structure, "test_directories",
                          dir_array(snapshot->structure.test_dirs,
                                    snapshot->structure.test_dir_count));
    cJSON_AddItemToObject(structure, "documentation_directories",
                          dir_array(snapshot->structure.doc_dirs,
                                    snapshot->structure.doc_dir_count));
    cJSON_AddItemToObject(structure, "configuration_directories",
                          dir_array(snapshot->structure.config_dirs,
                                    snapshot->structure.config_dir_count));
    cJSON_AddItemToObject(c_analysis, "structure", structure);

    cJSON_AddItemToObject(root, "c_analysis", c_analysis);

    cJSON_AddStringToObject(snapshot_json, "readme",
                            snapshot->readme_length > 0
                                ? snapshot->readme
                                : "(README unavailable)");
    for (i = 0; i < snapshot->selected_file_count; i++) {
        cJSON *file = cJSON_CreateObject();
        if (file == NULL) {
            continue;
        }
        cJSON_AddStringToObject(file, "path",
                                snapshot->selected_files[i].path);
        cJSON_AddStringToObject(file, "content",
                                snapshot->selected_files[i].content);
        cJSON_AddItemToArray(selected_files, file);
    }
    cJSON_AddItemToObject(snapshot_json, "selected_files", selected_files);
    cJSON_AddNumberToObject(snapshot_json, "max_readme_characters",
                            MAX_README_LENGTH);
    cJSON_AddNumberToObject(snapshot_json, "max_selected_files",
                            MAX_SELECTED_FILES);
    cJSON_AddItemToObject(root, "bounded_snapshot", snapshot_json);
    return root;
}
