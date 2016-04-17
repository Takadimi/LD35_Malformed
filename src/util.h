#ifndef UTIL_H
#define UTIL_H

char* read_entire_file(const char* file_path, size_t *file_size);
void set_resource_path(const char* file_path);
char* realize_path(const char* file_path);

#endif