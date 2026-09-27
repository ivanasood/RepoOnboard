#include "repo_analyzer.h"

#include <ctype.h>
#include <string.h>
#include <strings.h>

#include <cjson/cJSON.h>

#include "json_utils.h"

static int is_directory_path(const char *path) {
    return path != NULL && path[0] != '\0' && path[strlen(path) - 1] == '/';
}

/*
 * Copy the parent directory of `path` into `dir_out` (up to `dir_size` bytes).
 * Returns 1 if a parent directory exists, 0 if the file is at the root.
 * The result never ends with '/' and never equals ".".
 * E.g. "src/routes/users.js" -> "src/routes"
 *      "package.json"        -> (returns 0, no parent dir)
 */
static int dir_of_path(const char *path, char *dir_out, size_t dir_size) {
    const char *last_slash;
    size_t len;

    if (path == NULL || dir_out == NULL || dir_size == 0) {
        return 0;
    }
    last_slash = strrchr(path, '/');
    if (last_slash == NULL || last_slash == path) {
        /* File is at root or starts with '/'; no parent directory. */
        dir_out[0] = '\0';
        return 0;
    }
    len = (size_t)(last_slash - path);
    if (len >= dir_size) {
        len = dir_size - 1;
    }
    memcpy(dir_out, path, len);
    dir_out[len] = '\0';
    return 1;
}

/*
 * Copy the top-level directory component of `path` into `top_out`.
 * E.g. "src/routes/users.js" -> "src"
 *      "src/index.js"        -> "src"
 *      "package.json"        -> (returns 0, file is at root)
 */
static int top_level_dir_of_path(const char *path, char *top_out, size_t top_size) {
    const char *slash;
    size_t len;

    if (path == NULL || top_out == NULL || top_size == 0) {
        return 0;
    }
    slash = strchr(path, '/');
    if (slash == NULL || slash == path) {
        top_out[0] = '\0';
        return 0;
    }
    len = (size_t)(slash - path);
    if (len >= top_size) {
        len = top_size - 1;
    }
    memcpy(top_out, path, len);
    top_out[len] = '\0';
    return 1;
}

/*
 * Add `dir` to the string array `dirs` (which has `*count` entries) if it is
 * not already present and `*count` < `max`.  Does nothing if `dir` is empty.
 */
static void add_dir_unique(char dirs[][MAX_PATH_LENGTH], size_t *count,
                           size_t max, const char *dir) {
    size_t i;

    if (dir == NULL || dir[0] == '\0' || *count >= max) {
        return;
    }
    for (i = 0; i < *count; i++) {
        if (strcmp(dirs[i], dir) == 0) {
            return; /* already present */
        }
    }
    copy_string(dirs[*count], MAX_PATH_LENGTH, dir);
    (*count)++;
}

static int is_test_path(const char *path) {
    return string_contains_case_insensitive(path, "test") ||
           string_contains_case_insensitive(path, "spec");
}

static int is_readme_path(const char *path) {
    const char *basename;
    if (path == NULL) {
        return 0;
    }
    basename = strrchr(path, '/');
    basename = basename == NULL ? path : basename + 1;
    return strcasecmp(basename, "README") == 0 ||
           has_suffix_case_insensitive(path, ".md") &&
               string_contains_case_insensitive(basename, "readme") ||
           has_suffix_case_insensitive(path, ".mdx") &&
               string_contains_case_insensitive(basename, "readme") ||
           has_suffix_case_insensitive(path, ".rst") &&
               string_contains_case_insensitive(basename, "readme");
}

static int is_documentation_path(const char *path) {
    return is_readme_path(path) ||
           has_suffix_case_insensitive(path, ".md") ||
           has_suffix_case_insensitive(path, ".mdx") ||
           string_contains_case_insensitive(path, "docs/");
}

static int is_configuration_path(const char *path) {
    return has_suffix_case_insensitive(path, ".json") ||
           has_suffix_case_insensitive(path, ".yml") ||
           has_suffix_case_insensitive(path, ".yaml") ||
           has_suffix_case_insensitive(path, ".toml") ||
           has_suffix_case_insensitive(path, ".ini") ||
           has_suffix_case_insensitive(path, ".env.example") ||
           string_contains_case_insensitive(path, "dockerfile") ||
           string_contains_case_insensitive(path, "cmakelists.txt");
}

static int is_source_path(const char *path) {
    static const char *extensions[] = {
        ".c", ".h", ".cpp", ".hpp", ".cc", ".java", ".go", ".rs",
        ".py", ".rb", ".php", ".js", ".jsx", ".ts", ".tsx", ".sh",
        ".swift", ".kt", ".css", ".html"
    };
    size_t i;

    for (i = 0; i < sizeof(extensions) / sizeof(extensions[0]); i++) {
        if (has_suffix_case_insensitive(path, extensions[i])) {
            return 1;
        }
    }
    return 0;
}

static void add_technology(repo_snapshot_t *snapshot, const char *technology) {
    size_t i;

    for (i = 0; i < snapshot->technology_count; i++) {
        if (strcmp(snapshot->technologies[i], technology) == 0) {
            return;
        }
    }
    if (snapshot->technology_count < MAX_TECHNOLOGIES) {
        copy_string(snapshot->technologies[snapshot->technology_count],
                    sizeof(snapshot->technologies[0]), technology);
        snapshot->technology_count++;
    }
}

static void infer_technology(repo_snapshot_t *snapshot, const char *path) {
    if (strcmp(path, "package.json") == 0 ||
        has_suffix_case_insensitive(path, "package-lock.json") ||
        has_suffix_case_insensitive(path, "pnpm-lock.yaml")) {
        add_technology(snapshot, "JavaScript/Node.js");
    } else if (strcmp(path, "requirements.txt") == 0 ||
               strcmp(path, "pyproject.toml") == 0 ||
               strcmp(path, "Pipfile") == 0) {
        add_technology(snapshot, "Python");
    } else if (strcmp(path, "Cargo.toml") == 0) {
        add_technology(snapshot, "Rust");
    } else if (strcmp(path, "go.mod") == 0) {
        add_technology(snapshot, "Go");
    } else if (strcmp(path, "pom.xml") == 0 ||
               strcmp(path, "build.gradle") == 0) {
        add_technology(snapshot, "Java");
    } else if (strcmp(path, "CMakeLists.txt") == 0 ||
               has_suffix_case_insensitive(path, ".c") ||
               has_suffix_case_insensitive(path, ".h")) {
        add_technology(snapshot, "C/C++");
    } else if (strcmp(path, "Dockerfile") == 0) {
        add_technology(snapshot, "Docker");
    }

    if (has_suffix_case_insensitive(path, ".tsx") ||
        has_suffix_case_insensitive(path, ".jsx")) {
        add_technology(snapshot, "React");
    }
}

static int important_file_score(const char *path) {
    if (is_readme_path(path)) {
        return 100;
    }
    if (strcmp(path, "package.json") == 0 ||
        strcmp(path, "requirements.txt") == 0 ||
        strcmp(path, "Cargo.toml") == 0 ||
        strcmp(path, "go.mod") == 0 ||
        strcmp(path, "pom.xml") == 0 ||
        strcmp(path, "CMakeLists.txt") == 0) {
        return 95;
    }
    if (strcmp(path, "Dockerfile") == 0 ||
        strcmp(path, ".env.example") == 0) {
        return 90;
    }
    if (string_contains_case_insensitive(path, "src/main") ||
        string_contains_case_insensitive(path, "src/index") ||
        string_contains_case_insensitive(path, "src/app") ||
        string_contains_case_insensitive(path, "app/main")) {
        return 80;
    }
    if (is_documentation_path(path)) {
        return 60;
    }
    if (is_source_path(path) && !is_test_path(path)) {
        return 40;
    }
    return 0;
}

static void add_important_file(repo_snapshot_t *snapshot, const char *path) {
    size_t i;
    int new_score;

    if (snapshot->important_file_count < MAX_SELECTED_FILES) {
        copy_string(snapshot->important_files[snapshot->important_file_count],
                    sizeof(snapshot->important_files[0]), path);
        snapshot->important_file_count++;
        return;
    }

    new_score = important_file_score(path);
    for (i = 0; i < snapshot->important_file_count; i++) {
        if (new_score > important_file_score(snapshot->important_files[i])) {
            copy_string(snapshot->important_files[i],
                        sizeof(snapshot->important_files[i]), path);
            return;
        }
    }
}

void repo_analyze_tree(repo_snapshot_t *snapshot,
                       const char *tree_json,
                       size_t tree_json_length) {
    cJSON *root;
    cJSON *tree;
    cJSON *item;
    cJSON *type;
    cJSON *path;
    char dir[MAX_PATH_LENGTH];
    char top_dir[MAX_PATH_LENGTH];
    size_t i;

    if (snapshot == NULL || tree_json == NULL || tree_json_length == 0) {
        return;
    }

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
        if (!cJSON_IsString(path) || !cJSON_IsString(type)) {
            continue;
        }
        path_value = path->valuestring;
        if (strcmp(type->valuestring, "tree") == 0 ||
            is_directory_path(path_value)) {
            snapshot->directory_count++;
            continue;
        }

        snapshot->tree_count++;

        /* ── Per-file classification (existing) ── */
        if (is_source_path(path_value)) {
            snapshot->source_file_count++;
        }
        if (is_test_path(path_value)) {
            snapshot->test_file_count++;
        }
        if (is_documentation_path(path_value)) {
            snapshot->documentation_file_count++;
        }
        if (is_configuration_path(path_value)) {
            snapshot->configuration_file_count++;
        }
        infer_technology(snapshot, path_value);
        if (important_file_score(path_value) > 0) {
            add_important_file(snapshot, path_value);
        }

        /* ── Structure: top-level directory ── */
        if (top_level_dir_of_path(path_value, top_dir, sizeof(top_dir))) {
            add_dir_unique(snapshot->structure.top_level_dirs,
                           &snapshot->structure.top_level_dir_count,
                           MAX_STRUCTURE_DIRS, top_dir);
        }

        /* ── Structure: category directories ──
         * Use the immediate parent directory of the file for category
         * directories.  This correctly handles both flat and nested layouts
         * (e.g. "test/users.test.js" -> "test";
         *       "src/routes/users.js" -> "src/routes").
         */
        if (dir_of_path(path_value, dir, sizeof(dir))) {
            if (is_source_path(path_value)) {
                add_dir_unique(snapshot->structure.source_dirs,
                               &snapshot->structure.source_dir_count,
                               MAX_STRUCTURE_DIRS, dir);
            }
            if (is_test_path(path_value)) {
                add_dir_unique(snapshot->structure.test_dirs,
                               &snapshot->structure.test_dir_count,
                               MAX_STRUCTURE_DIRS, dir);
            }
            if (is_documentation_path(path_value)) {
                add_dir_unique(snapshot->structure.doc_dirs,
                               &snapshot->structure.doc_dir_count,
                               MAX_STRUCTURE_DIRS, dir);
            }
            if (is_configuration_path(path_value)) {
                add_dir_unique(snapshot->structure.config_dirs,
                               &snapshot->structure.config_dir_count,
                               MAX_STRUCTURE_DIRS, dir);
            }
        }
    }

    /* A README is valuable even when its casing/path was not in the top list. */
    for (i = 0; i < snapshot->important_file_count; i++) {
        if (string_contains_case_insensitive(snapshot->important_files[i],
                                             "readme")) {
            break;
        }
    }
    cJSON_Delete(root);
}

void repo_add_file_content(repo_snapshot_t *snapshot,
                           const char *path,
                           const char *content,
                           size_t content_length) {
    size_t bounded_length;
    size_t i;

    if (snapshot == NULL || path == NULL || content == NULL) {
        return;
    }
    bounded_length = content_length;

    if (string_contains_case_insensitive(path, "readme")) {
        if (bounded_length > MAX_README_LENGTH) {
            bounded_length = MAX_README_LENGTH;
        }
        memcpy(snapshot->readme, content, bounded_length);
        snapshot->readme[bounded_length] = '\0';
        snapshot->readme_length = bounded_length;
        return;
    }

    for (i = 0; i < snapshot->selected_file_count; i++) {
        if (strcmp(snapshot->selected_files[i].path, path) == 0) {
            if (bounded_length > MAX_FILE_CONTENT_LENGTH) {
                bounded_length = MAX_FILE_CONTENT_LENGTH;
            }
            memcpy(snapshot->selected_files[i].content, content,
                   bounded_length);
            snapshot->selected_files[i].content[bounded_length] = '\0';
            snapshot->selected_files[i].content_length = bounded_length;
            return;
        }
    }
    if (snapshot->selected_file_count >= MAX_SELECTED_FILES) {
        return;
    }
    if (bounded_length > MAX_FILE_CONTENT_LENGTH) {
        bounded_length = MAX_FILE_CONTENT_LENGTH;
    }
    copy_string(snapshot->selected_files[snapshot->selected_file_count].path,
                sizeof(snapshot->selected_files[0].path), path);
    memcpy(snapshot->selected_files[snapshot->selected_file_count].content,
           content, bounded_length);
    snapshot->selected_files[snapshot->selected_file_count]
        .content[bounded_length] = '\0';
    snapshot->selected_files[snapshot->selected_file_count].content_length =
        bounded_length;
    snapshot->selected_file_count++;
}
