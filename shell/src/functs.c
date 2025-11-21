#include "functs.h"

static char home_directory[MAX_PATH_LENGTH];
static char previous_directory[MAX_PATH_LENGTH];
static int has_previous_dir = 0;

// LLM CODE BEGINS
void init_shell_directories() {
     // Get the current working directory as the "home" directory for the shell
    if (getcwd(home_directory, MAX_PATH_LENGTH) == NULL) {
        // Fallback to actual home directory if getcwd fails
        struct passwd *pw = getpwuid(getuid());
        if (pw != NULL) {
            strcpy(home_directory, pw->pw_dir);
        } else {
            strcpy(home_directory, getenv("HOME") ? getenv("HOME") : "/");
        }
    }
    
    
    // Initialize previous directory as empty
    previous_directory[0] = '\0';
    has_previous_dir = 0;
}

// Helper function to check if directory exists
int directory_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

// Helper function to resolve path (handle ~, ., .., -, and relative/absolute paths)
int resolve_path(const char *arg, char *resolved_path) {
    if (strcmp(arg, "~") == 0) {
        strcpy(resolved_path, home_directory);
        return 1;
    } else if (strcmp(arg, ".") == 0) {
        if (getcwd(resolved_path, MAX_PATH_LENGTH) == NULL) {
            return 0;
        }
        return 1;
    } else if (strcmp(arg, "..") == 0) {
        if (getcwd(resolved_path, MAX_PATH_LENGTH) == NULL) {
            return 0;
        }
        char *last_slash = strrchr(resolved_path, '/');
        if (last_slash != NULL && last_slash != resolved_path) {
            *last_slash = '\0';
        } else if (last_slash == resolved_path) {
            strcpy(resolved_path, "/");
        }
        return 1;
    } else if (strcmp(arg, "-") == 0) {
        if (!has_previous_dir) {
            return 0; // No previous directory
        }
        strcpy(resolved_path, previous_directory);
        return 1;
    } else {
        // Regular path (relative or absolute)
        if (arg[0] == '/') {
            // Absolute path
            strcpy(resolved_path, arg);
        } else {
            // Relative path
            if (getcwd(resolved_path, MAX_PATH_LENGTH) == NULL) {
                return 0;
            }
            strcat(resolved_path, "/");
            strcat(resolved_path, arg);
        }
        return 1;
    }
}

// Implementation of hop command
int hop_command(int argc, char *argv[]) {
    char current_dir[MAX_PATH_LENGTH];
    char new_dir[MAX_PATH_LENGTH];
    
    // Get current directory
    if (getcwd(current_dir, MAX_PATH_LENGTH) == NULL) {
        perror("hop: getcwd failed");
        return 1;
    }
    
    // If no arguments, go to home directory
    if (argc == 1) {
        if (chdir(home_directory) == 0) {
            strcpy(previous_directory, current_dir);
            has_previous_dir = 1;
        } else {
            printf("No such directory!\n");
            return 1;
        }
        return 0;
    }
    
    // Process each argument
    for (int i = 1; i < argc; i++) {
        // Handle special case of "." (do nothing)
        if (strcmp(argv[i], ".") == 0) {
            continue;
        }
        
        // Handle special case of "-" with no previous directory
        if (strcmp(argv[i], "-") == 0 && !has_previous_dir) {
            continue; // Do nothing if no previous directory
        }
        
        // Resolve the path
        if (!resolve_path(argv[i], new_dir)) {
            continue; // Skip if path resolution failed
        }
        
        // Check if directory exists
        if (!directory_exists(new_dir)) {
            printf("No such directory!\n");
            return 1;
        }
        
        // Get current directory before changing
        if (getcwd(current_dir, MAX_PATH_LENGTH) == NULL) {
            perror("hop: getcwd failed");
            return 1;
        }
        
        // Change directory
        if (chdir(new_dir) == 0) {
            // Update previous directory only if we actually changed
            if (strcmp(current_dir, new_dir) != 0) {
                strcpy(previous_directory, current_dir);
                has_previous_dir = 1;
            }
        } else {
            printf("No such directory!\n");
            return 1;
        }
    }
    
    return 0;
}

// Structure for directory entries
typedef struct {
    char name[256];
    int is_directory;
} dir_entry_t;

// Comparison function for qsort (lexicographic order using ASCII values)
int compare_entries(const void *a, const void *b) {
    const dir_entry_t *entry_a = (const dir_entry_t *)a;
    const dir_entry_t *entry_b = (const dir_entry_t *)b;
    return strcmp(entry_a->name, entry_b->name);
}

// Implementation of reveal command
int reveal_command(int argc, char *argv[]) {
    int show_hidden = 0;
    int line_format = 0;
    char target_dir[MAX_PATH_LENGTH];
    int arg_index = 1;
    
    // Parse flags
    while (arg_index < argc && argv[arg_index][0] == '-' && strlen(argv[arg_index]) > 1) {
        char *flag_ptr = argv[arg_index] + 1; // Skip the '-'
        
        while (*flag_ptr) {
            switch (*flag_ptr) {
                case 'a':
                    show_hidden = 1;
                    break;
                case 'l':
                    line_format = 1;
                    break;
                default:
                    printf("reveal: Invalid flag -%c\n", *flag_ptr);
                    return 1;
            }
            flag_ptr++;
        }
        arg_index++;
    }
    
    // Check for too many arguments (more than one directory argument)
    if (argc - arg_index > 1) {
        printf("reveal: Invalid Syntax!\n");
        return 1;
    }
    
    // Determine target directory
    if (arg_index < argc) {
        // Handle special case of "reveal -" with no previous directory
        if (strcmp(argv[arg_index], "-") == 0 && !has_previous_dir) {
            printf("No such directory!\n");
            return 1;
        }
        
        if (!resolve_path(argv[arg_index], target_dir)) {
            printf("No such directory!\n");
            return 1;
        }
    } else {
        // No directory argument, use current directory
        if (getcwd(target_dir, MAX_PATH_LENGTH) == NULL) {
            perror("reveal: getcwd failed");
            return 1;
        }
    }
    
    // Check if target directory exists
    if (!directory_exists(target_dir)) {
        printf("No such directory!\n");
        return 1;
    }
    
    // Open directory
    DIR *dir = opendir(target_dir);
    if (dir == NULL) {
        printf("No such directory!\n");
        return 1;
    }
    
    // Read directory entries
    dir_entry_t entries[MAX_ENTRIES];
    int entry_count = 0;
    struct dirent *entry;
    
    while ((entry = readdir(dir)) != NULL && entry_count < MAX_ENTRIES) {
        // Skip hidden files if not requested
        if (!show_hidden && entry->d_name[0] == '.') {
            continue;
        }
        
        strcpy(entries[entry_count].name, entry->d_name);
        
        // Check if it's a directory
        char full_path[MAX_PATH_LENGTH];
        size_t path_len = snprintf(full_path, sizeof(full_path), "%s/%s", target_dir, entry->d_name);
        
        // Check if the path was truncated
        if (path_len >= sizeof(full_path)) {
            // Path too long, skip this entry or handle error
            continue;
        }
        
        struct stat st;
        entries[entry_count].is_directory = (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode));
        
        entry_count++;
    }
    
    closedir(dir);
    
    // Sort entries lexicographically
    qsort(entries, entry_count, sizeof(dir_entry_t), compare_entries);
    
    // Display entries
    if (line_format) {
        // Line by line format
        for (int i = 0; i < entry_count; i++) {
            printf("%s\n", entries[i].name);
        }
    } else {
        // Default format (like ls)
        for (int i = 0; i < entry_count; i++) {
            printf("%s", entries[i].name);
            if (i < entry_count - 1) {
                printf("  ");
            }
        }
        if (entry_count > 0) {
            printf("\n");
        }
    }
    
    return 0;
}


//  ping 
int ping_command(int argc, char *argv[]) {
    // Check argument count
    if (argc != 3) {
        printf("Usage: ping <pid> <signal_number>\n");
        return 1;
    }
    
    // Parse PID
    char *endptr;
    pid_t pid = (pid_t)strtol(argv[1], &endptr, 10);
    if (*endptr != '\0' || pid <= 0) {
        printf("Invalid PID: %s\n", argv[1]);
        return 1;
    }
    
    // Parse signal number
    int signal_number = (int)strtol(argv[2], &endptr, 10);
    if (*endptr != '\0') {
        printf("Invalid signal number: %s\n", argv[2]);
        return 1;
    }
    
    // Take signal number modulo 32
    int actual_signal = signal_number % 32;
    
    // Send the signal
    if (kill(pid, actual_signal) == 0) {
        // Success
        printf("Sent signal %d to process with pid %d\n", signal_number, pid);
        return 0;
    } else {
        // Failed to send signal
        if (errno == ESRCH) {
            printf("No such process found\n");
        } else {
            perror("Failed to send signal");
        }
        return 1;
    }
}

int execute_builtin_command(int argc, char *argv[]) {
    if (argc == 0) return 0;
    
   if (strcmp(argv[0], "hop") == 0) {
        return hop_command(argc, argv);
    } else if (strcmp(argv[0], "reveal") == 0) {
        return reveal_command(argc, argv);
    } else if (strcmp(argv[0], "activities") == 0) {
        return activities_command();
    } else if (strcmp(argv[0], "ping") == 0) {
        return ping_command(argc, argv);
    } else if (strcmp(argv[0], "fg") == 0) {
        return fg_command(argc, argv);
    } else if (strcmp(argv[0], "bg") == 0) {
        return bg_command(argc, argv);
    }

    return -1; // Not a builtin command
}
// LLM CODE ENDS
