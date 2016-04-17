#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "util.h"

static char resource_path[512];

char* read_entire_file(const char* file_path, size_t *file_size) {
    char terminated_file_path[512];
    strcpy(terminated_file_path, file_path);
    terminated_file_path[strlen(file_path)] = 0;

    FILE* fp = fopen(terminated_file_path, "rb");

    if (fp) {
        fseek(fp, 0, SEEK_END);
        *file_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        char* file_contents = (char*) malloc(*file_size+1);

        size_t bytes_read = fread(file_contents, 1, *file_size, fp);
        assert(*file_size == bytes_read);

        file_contents[*file_size] = 0;
        fclose(fp);

        return file_contents;
    } else {
        perror("Error");
        return NULL;
    }
}

void set_resource_path(const char* file_path) {
    strcpy(resource_path, file_path);
}

char* realize_path(const char* file_path) {
    char temp_path[512];
    strcpy(temp_path, resource_path);

    return strcat(temp_path, file_path);
}
