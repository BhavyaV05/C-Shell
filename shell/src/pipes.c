#include "pipes.h"
#include "jobs.h"

extern char current_foreground_command[MAX_COMMAND_NAME];
// Forward declarations for builtin functions (from previous implementation)
// int execute_builtin_command(int argc, char *argv[]);
// void init_shell_directories();
const char* get_command_name(char *args[]) {
    if (args[0] == NULL) return "unknown";
    return args[0];
}
// LLM CODE BEGINS
// Execute a single external command
int execute_external_command(char *args[], char *input_file, char *output_file, 
                            int append_output, int background) {
    pid_t pid;
    int status;
    
    if (args[0] == NULL) {
        return 0; // Empty command
    }
    
    // Fork a child process
    pid = fork();
    
    if (pid == 0) {
        // Child process
        // Create a new process group for this process
        if (setpgid(0, 0) == -1) {
            perror("setpgid failed");
        }
        // Background processes should not have access to terminal input
        if (background) {
            // Redirect stdin to /dev/null for background processes
            freopen("/dev/null", "r", stdin);
        }
        
        // Handle input redirection
        if (input_file != NULL) {
            int input_fd = open(input_file, O_RDONLY);
            if (input_fd == -1) {
                perror("Input redirection failed");
                exit(EXIT_FAILURE);
            }
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }
        
        // Handle output redirection
        if (output_file != NULL) {
            int output_fd;
            if (append_output) {
                output_fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
            } else {
                output_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            }
            if (output_fd == -1) {
                perror("Output redirection failed");
                exit(EXIT_FAILURE);
            }
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }
        
        // Execute the command
        if (execvp(args[0], args) == -1) {
            fprintf(stderr, "%s: command not found\n", args[0]);
            exit(EXIT_FAILURE);
        }
        
    } else if (pid < 0) {
        // Fork failed
        perror("fork failed");
        return 1;
        
    } else {
        // Parent process

         // Set process group for the child
        setpgid(pid, pid);
        if (background) {
            // Add to background job list and don't wait
            add_background_job(pid, get_command_name(args));
            return 0;
        } else {

            // Set as foreground process
            current_foreground_pid = pid;
            current_foreground_pgid = pid;
            snprintf(current_foreground_command, MAX_COMMAND_NAME, "%s", get_command_name(args));
            // Wait for foreground process to complete
            do {
                waitpid(pid, &status, WUNTRACED);
                 // Check if process was stopped
                if (WIFSTOPPED(status)) {
                    // Process was stopped, add to job list
                    add_stopped_job(pid, get_command_name(args));
                    printf("[%d] Stopped %s\n", next_job_id - 1, get_command_name(args));
                    current_foreground_pid = 0;
                    current_foreground_pgid = 0;
                    return 0;
                }
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
             // Clear foreground process
            current_foreground_pid = 0;
            current_foreground_pgid = 0;
            
            return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
        }
    }
    
    return 0;
}
// Execute a single command (checks for builtins first, then external)
int execute_single_command(command_t *cmd)
{
    if (cmd == NULL || cmd->argc == 0)
    {
        return 0;
    }

    // Check if it's a builtin command first
    if (strcmp(cmd->args[0], "hop") == 0 || strcmp(cmd->args[0], "reveal") == 0 || strcmp(cmd->args[0], "activities") == 0|| strcmp(cmd->args[0], "ping") == 0|| strcmp(cmd->args[0], "bg") == 0|| strcmp(cmd->args[0], "fg") == 0)
    {
        // It's a builtin command - handle redirections
        if (cmd->input_file || cmd->output_file)
        {
            // Fork a process to handle redirections for built-in commands
            pid_t pid = fork();
            
            if (pid == 0)
            {
                // Child process - set up redirections
                
                // Handle input redirection
                if (cmd->input_file)
                {
                    int input_fd = open(cmd->input_file, O_RDONLY);
                    if (input_fd == -1)
                    {
                        perror("Input redirection failed");
                        exit(EXIT_FAILURE);
                    }
                    dup2(input_fd, STDIN_FILENO);
                    close(input_fd);
                }
                
                // Handle output redirection
                if (cmd->output_file)
                {
                    int output_fd;
                    if (cmd->append_output)
                    {
                        output_fd = open(cmd->output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
                    }
                    else
                    {
                        output_fd = open(cmd->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    }
                    if (output_fd == -1)
                    {
                        perror("Output redirection failed");
                        exit(EXIT_FAILURE);
                    }
                    dup2(output_fd, STDOUT_FILENO);
                    close(output_fd);
                }
                
                // Execute the builtin command
                int result = execute_builtin_command(cmd->argc, cmd->args);
                exit(result);
            }
            else if (pid < 0)
            {
                perror("fork failed");
                return 1;
            }
            else
            {
                // Parent process - wait for child
                int status;
                if (!cmd->background)
                {
                    waitpid(pid, &status, 0);
                    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
                }
                else
                {
                    printf("[Background process started with PID: %d]\n", pid);
                    return 0;
                }
            }
        }
        else
        {
            // No redirections, execute directly
            return execute_builtin_command(cmd->argc, cmd->args);
        }
    }

    // Not a builtin, execute as external command
    return execute_external_command(cmd->args, cmd->input_file, cmd->output_file,
                                    cmd->append_output, cmd->background);
}

int execute_pipeline(pipeline_t *pipeline) {
    if (pipeline == NULL || pipeline->num_commands == 0) {
        return 0;
    }

    // If only one command, execute it directly
    if (pipeline->num_commands == 1) {
        command_t *cmd = &pipeline->commands[0];
        
        // Check if it's a builtin command
        if ((strcmp(cmd->args[0], "hop") == 0) || (strcmp(cmd->args[0], "reveal") == 0) || (strcmp(cmd->args[0], "activities") == 0)|| (strcmp(cmd->args[0], "ping") == 0) || (strcmp(cmd->args[0], "bg") == 0) || (strcmp(cmd->args[0], "fg") == 0)) {
            // Built-in commands cannot run in background meaningfully
            if (pipeline->background) {
                printf("Warning: Built-in command '%s' cannot run in background\n", cmd->args[0]);
            }
            return execute_single_command(cmd);
        }
        
        // External command - pass background flag
        return execute_external_command(cmd->args, cmd->input_file, cmd->output_file,
                                       cmd->append_output, pipeline->background);
    }

    // Multiple commands - create pipes (existing pipeline logic)
    // For background pipelines, the entire pipeline runs in background
    int num_pipes = pipeline->num_commands - 1;
    int pipes[num_pipes][2];
    pid_t pids[pipeline->num_commands];
    
    // Create all pipes
    for (int i = 0; i < num_pipes; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe failed");
            return 1;
        }
    }
    
    // Execute each command in the pipeline
    for (int i = 0; i < pipeline->num_commands; i++) {
        command_t *cmd = &pipeline->commands[i];
        
        pids[i] = fork();
        
        if (pids[i] == 0) {
            // Child process
            
            // Background processes should not have access to terminal input
            if (pipeline->background && i == 0) {
                freopen("/dev/null", "r", stdin);
            }
            
            // Set up input redirection
            if (i == 0) {
                if (cmd->input_file) {
                    int input_fd = open(cmd->input_file, O_RDONLY);
                    if (input_fd == -1) {
                        perror("Input redirection failed");
                        exit(EXIT_FAILURE);
                    }
                    dup2(input_fd, STDIN_FILENO);
                    close(input_fd);
                }
            } else {
                dup2(pipes[i-1][0], STDIN_FILENO);
            }
            
            // Set up output redirection
            if (i == pipeline->num_commands - 1) {
                if (cmd->output_file) {
                    int output_fd;
                    if (cmd->append_output) {
                        output_fd = open(cmd->output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
                    } else {
                        output_fd = open(cmd->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    }
                    if (output_fd == -1) {
                        perror("Output redirection failed");
                        exit(EXIT_FAILURE);
                    }
                    dup2(output_fd, STDOUT_FILENO);
                    close(output_fd);
                }
            } else {
                dup2(pipes[i][1], STDOUT_FILENO);
            }
            
            // Close all pipe file descriptors in child
            for (int j = 0; j < num_pipes; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            // Execute the command
            if (strcmp(cmd->args[0], "hop") == 0 || strcmp(cmd->args[0], "reveal") == 0) {
                int result = execute_builtin_command(cmd->argc, cmd->args);
                exit(result);
            } else {
                if (execvp(cmd->args[0], cmd->args) == -1) {
                    fprintf(stderr, "%s: command not found\n", cmd->args[0]);
                    exit(EXIT_FAILURE);
                }
            }
        } else if (pids[i] < 0) {
            perror("fork failed");
            return 1;
        }
    }
    
    // Parent process - close all pipe file descriptors
    for (int i = 0; i < num_pipes; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    
    if (pipeline->background) {
        // For background pipeline, add the first process to job list
        // (or you could track the entire pipeline)
        add_background_job(pids[0], get_command_name(pipeline->commands[0].args));
        return 0;
    } else {
        // Wait for all child processes
        int status, last_status = 0;
        for (int i = 0; i < pipeline->num_commands; i++) {
            waitpid(pids[i], &status, 0);
            if (i == pipeline->num_commands - 1) {
                last_status = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
            }
        }
        return last_status;
    }
}
// Update the execute_simple_pipeline function to use the new pipeline function
// int execute_simple_pipeline(pipeline_t *pipeline) {
//     return execute_pipeline_with_pipes(pipeline);
// }

// Main shell execution function
// Updated main shell execution function
int execute_command_line(char *input_line) {
    token_t tokens[MAX_TOKENS];
    
    // Tokenize the input
    int token_count = tokenize(input_line, tokens);
    if (token_count == 0) {
        return 0;
    }
    
    // Check if input contains semicolons (command sequence)
    int has_semicolon = 0;
    for (int i = 0; i < token_count; i++) {
        if (tokens[i].type == TOKEN_SEMICOLON) {
            has_semicolon = 1;
            break;
        }
    }
    
    if (has_semicolon) {
        // Parse and execute as command sequence
        command_sequence_t sequence;
        if (parse_command_sequence(tokens, &sequence)) {
            int result = execute_command_sequence(&sequence);
            free_command_sequence(&sequence);
            return result;
        } else {
            printf("Parse error in command sequence\n");
            return 1;
        }
    } else {
        // Single pipeline - use existing logic
        pipeline_t pipeline;
        if (parse_pipeline(tokens, &pipeline)) {
            int result = execute_pipeline(&pipeline);
            free_pipeline(&pipeline);
            return result;
        } else {
            printf("Parse error\n");
            return 1;
        }
    }
}

// Execute a sequence of commands separated by semicolons
int execute_command_sequence(command_sequence_t *sequence) {
    if (sequence == NULL || sequence->num_pipelines == 0) {
        return 0;
    }
    
    int last_exit_status = 0;
    
    // Execute each pipeline in sequence
    for (int i = 0; i < sequence->num_pipelines; i++) {
        pipeline_t *pipeline = &sequence->pipelines[i];
        
        // Execute the pipeline
        int exit_status = execute_pipeline(pipeline);
        
        // Update last exit status
        last_exit_status = exit_status;
        
        // Note: We continue execution even if a command fails
        // This is the required behavior for semicolon operator
    }
    
    return last_exit_status;
}

// Enhanced main shell loop

// Enhanced command parsing to handle multiple input redirections
// (Only the last input redirection takes effect)
int parse_command_with_multiple_redirections(token_t tokens[], int *token_index, command_t *cmd)
{
    init_command(cmd);

    while (tokens[*token_index].type != TOKEN_EOF &&
           tokens[*token_index].type != TOKEN_PIPE &&
           tokens[*token_index].type != TOKEN_SEMICOLON &&
           tokens[*token_index].type != TOKEN_AND &&
           tokens[*token_index].type != TOKEN_OR)
    {

        token_t *current = &tokens[*token_index];

        switch (current->type)
        {
        case TOKEN_WORD:
            if (cmd->argc < MAX_TOKENS - 1)
            {
                cmd->args[cmd->argc] = malloc(strlen(current->value) + 1);
                strcpy(cmd->args[cmd->argc], current->value);
                cmd->argc++;
            }
            break;

        case TOKEN_REDIRECT_IN:
            (*token_index)++;
            if (tokens[*token_index].type == TOKEN_WORD)
            {
                // Free previous input file if exists (multiple redirections)
                if (cmd->input_file)
                {
                    free(cmd->input_file);
                }
                // Set the new input file (last one takes effect)
                cmd->input_file = malloc(strlen(tokens[*token_index].value) + 1);
                strcpy(cmd->input_file, tokens[*token_index].value);
            }
            break;

        case TOKEN_REDIRECT_OUT:
            (*token_index)++;
            if (tokens[*token_index].type == TOKEN_WORD)
            {
                // Create the intermediate file (even if it won't receive output)
                if (cmd->output_file) {
                    // Create/truncate the previous output file
                    int temp_fd = open(cmd->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (temp_fd != -1) {
                        close(temp_fd);
                    }
                    free(cmd->output_file);
                }
                
                // Set the new output file (this will be the final destination)
                cmd->output_file = malloc(strlen(tokens[*token_index].value) + 1);
                strcpy(cmd->output_file, tokens[*token_index].value);
                cmd->append_output = 0;
            }
            break;

        case TOKEN_REDIRECT_APPEND:
            (*token_index)++;
            if (tokens[*token_index].type == TOKEN_WORD)
            {
                // Create the intermediate file (even if it won't receive output)
                if (cmd->output_file) {
                    // For previous file, create it based on the previous append_output flag
                    int temp_fd;
                    if (cmd->append_output) {
                        temp_fd = open(cmd->output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
                    } else {
                        temp_fd = open(cmd->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    }
                    if (temp_fd != -1) {
                        close(temp_fd);
                    }
                    free(cmd->output_file);
                }
                
                // Set the new output file (this will be the final destination)
                cmd->output_file = malloc(strlen(tokens[*token_index].value) + 1);
                strcpy(cmd->output_file, tokens[*token_index].value);
                cmd->append_output = 1;
            }
            break;

        case TOKEN_BACKGROUND:
            cmd->background = 1;
            break;

        default:
            break;
        }

        (*token_index)++;
    }

    // Null-terminate the args array
    cmd->args[cmd->argc] = NULL;

    return (cmd->argc > 0) ? 1 : 0;
}

// LLM CODE ENDS
