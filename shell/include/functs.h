#include "prompt.h"
#include "jobs.h"

#ifndef FUNCTS_H
#define FUNCTS_H
#define MAX_PATH_LENGTH 4096
#define MAX_ENTRIES 1024



// void my_function();
void init_shell_directories();
int directory_exists(const char *path);
int resolve_path(const char *arg, char *resolved_path);
int hop_command(int argc, char *argv[]);
int reveal_command(int argc, char *argv[]); 
int compare_entries(const void *a, const void *b);
int execute_builtin_command(int argc, char *argv[]);
int ping_command(int argc, char *argv[]);



#endif
