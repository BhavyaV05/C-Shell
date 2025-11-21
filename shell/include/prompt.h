#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>


#define _GNU_SOURCE
// #define _POSIX_C_SOURCE 200112L
#ifndef PROMPT_H
#define PROMPT_H

void init_home();
void print_prompt();

#endif
