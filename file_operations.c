#include <stdio.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

int copy_file(const char *src, const char *dst) {
    FILE *in = fopen(src, "rb");
    if (!in)
        return -1;

    FILE *out = fopen(dst, "wb");
    if (!out) {
        fclose(in);
        return -1;
    }

    char buffer[8192];
    size_t n;

    while ((n = fread(buffer, 1, sizeof(buffer), in)) > 0) {
        if (fwrite(buffer, 1, n, out) != n) {
            fclose(in);
            fclose(out);
            return -1;
        }
    }

    fclose(in);
    fclose(out);

    return ferror(in) ? -1 : 0;
}

int file_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}


int remove_recursive(const char *path) {
    struct stat st;

    // Use lstat so symlinks are not followed.
    if (lstat(path, &st) != 0) {
        return -1;
    }

    // If it's not a directory, simply unlink it.
    if (!S_ISDIR(st.st_mode)) {
        return unlink(path);
    }

    DIR *dir = opendir(path);
    if (!dir) {
        return -1;
    }

    struct dirent *entry;
    int result = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        size_t len = strlen(path) + strlen(entry->d_name) + 2;
        char *child = malloc(len);

        if (!child) {
            result = -1;
            break;
        }

        snprintf(child, len, "%s/%s", path, entry->d_name);

        if (remove_recursive(child) != 0) {
            result = -1;
        }

        free(child);

        if (result != 0) {
            break;
        }
    }

    closedir(dir);

    if (result != 0) {
        return -1;
    }

    // Directory should now be empty.
    return rmdir(path);
}


int copy_recursive(const char *src, const char *dst) {
    struct stat st;

    if (stat(src, &st) != 0) return 1;

    if (S_ISREG(st.st_mode)) {
        return copy_file(src, dst);
    }

    if (!S_ISDIR(st.st_mode)) return 1;

    if (mkdir(dst, 0755) != 0) return 1;

    DIR *dir = opendir(src);
    if (!dir) return 1;

    struct dirent *entry;
    int error = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char src_path[PATH_MAX];
        char dst_path[PATH_MAX];

        snprintf(src_path, sizeof(src_path), "%s/%s", src, entry->d_name);
        snprintf(dst_path, sizeof(dst_path), "%s/%s", dst, entry->d_name);

        if (copy_recursive(src_path, dst_path) != 0) {
            error = 1;
            break;
        }
    }

    closedir(dir);
    return error;
}
