#include "prompt.h"
#include "parser.h"
#include "functs.h"
#include "pipes.h"
#include "jobs.h"

int main()
{
    init_home();
    init_shell_directories(); // Add this - it's required for hop and reveal commands
    init_job_system(); // Initialize job management system

    while(1)
    {
        check_background_jobs(); // Check and update background jobs
        print_prompt();
        // printf("hello, welcome\n");
        char input[1024];
        if(!fgets(input, sizeof(input), stdin)) {
            // EOF detected (Ctrl-D)
            printf("logout\n");
            kill_all_children();
            exit(0);
        }
        input[strcspn(input, "\n")] = '\0'; 
        // printf("hifirst");
        // Skip empty input
        if (strlen(input) == 0) {
            continue;
        }
        
        // checking for valid input
        if (!parse_input(input)) {
            printf("INVALID SYNTAX\n");
            continue;
        }
        
    //     // if valid, we are proceeding with rest of the code 
    //     token_t tokens[MAX_TOKENS];       
    //     pipeline_t pipeline;
    //     int token_count = tokenize(input, tokens);
    //     printf("Token count: %d\n", token_count); // Debug line
    //     if (parse_pipeline(tokens, &pipeline)) { // Remove the duplicate call
    //         // Check for built-in commands after parsing
    //         printf("hi");
    //         if (pipeline.num_commands > 0 && pipeline.commands[0].argc > 0) {
    //             char *command = pipeline.commands[0].args[0];
    //             printf("Command: %s, argc: %d\n", command, pipeline.commands[0].argc);
    //             if (strcmp(command, "hop") == 0) {
    //                 hop_command(pipeline.commands[0].argc, pipeline.commands[0].args);
    //             } else if (strcmp(command, "reveal") == 0) {
    //                 printf("Calling reveal_command...\n"); // Debug line
    //                 reveal_command(pipeline.commands[0].argc, pipeline.commands[0].args);
    //                 printf("reveal_command finished\n");
    //             } else if (strcmp(command, "exit") == 0) {
    //                 free_pipeline(&pipeline);
    //                 break;
    //             } else {
    //                 // This is where you would handle external commands
    //                 printf("Unknown command: %s\n", command);
    //             }
    //         }
            
    //         // Free allocated memory
    //         free_pipeline(&pipeline);
    //     } else {
    //         printf("Failed to parse command\n");
    //     }
    // }
        // Execute the command line
        if (strcmp(input, "exit") == 0) {
             kill_all_children();
            break; // Exit the shell
        }
        execute_command_line(input);
    }

    return 0;
}