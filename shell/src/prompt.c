#include "prompt.h"

static char home_dir[PATH_MAX];
// LLM CODE BEGINS
void init_home() {
    // store initial working directory as home
    if (getcwd(home_dir, sizeof(home_dir)) == NULL) {
        perror("getcwd");
        exit(1);
    }
}

void print_prompt() {
    // Username
    char *username = getlogin();
    if (!username) {
        struct passwd *pw = getpwuid(getuid());
        username = pw ? pw->pw_name : "unknown";
    }

    // Hostname
    char hostname[HOST_NAME_MAX + 1];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        perror("gethostname");
        strcpy(hostname, "unknown");
    }

    // Current working dir
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd");
        strcpy(cwd, "?");
    }

    // Replace home with ~ if applicable
    char display_path[PATH_MAX];
    if (strncmp(cwd, home_dir, strlen(home_dir)) == 0) {
        snprintf(display_path, sizeof(display_path), "~%s", cwd + strlen(home_dir));
    } else {
        snprintf(display_path, sizeof(display_path), "%s", cwd);
    }

    // Print prompt
    printf("<%s@%s:%s> ", username, hostname, display_path);
    fflush(stdout);
}
// LLM CODE ENDS
