#include "prompt.h"
#ifndef PARSER_H
#define PARSER_H

#define MAX_TOKENS 64
#define MAX_TOKEN_LENGTH 256
#define MAX_INPUT_LENGTH 1024
#define MAX_PIPELINE_COMMANDS 32  
#define MAX_SEQUENCE_PIPELINES 16
// Token types
typedef enum {
    TOKEN_WORD,
    TOKEN_PIPE,
    TOKEN_REDIRECT_IN,
    TOKEN_REDIRECT_OUT,
    TOKEN_REDIRECT_APPEND,
    TOKEN_BACKGROUND,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_SEMICOLON,
    TOKEN_EOF
} token_type_t;

// Token structure
typedef struct {
    token_type_t type;
    char value[MAX_TOKEN_LENGTH];
} token_t;

// Command structure
typedef struct {
    char *args[MAX_TOKENS];  // Command arguments
    int argc;                // Argument count
    char *input_file;        // Input redirection file
    char *output_file;       // Output redirection file
    int append_output;       // Append to output file (1) or overwrite (0)
    int background;          // Run in background
} command_t;

// Pipeline structure
typedef struct {
    command_t commands[MAX_TOKENS];
    int num_commands;
    int background;
} pipeline_t;
typedef struct {
    pipeline_t pipelines[MAX_TOKENS];
    int num_pipelines;
} command_sequence_t;


int parse_input(const char *input);
int tokenize(const char *input, token_t tokens[]);
void init_command(command_t *cmd);
int parse_command(token_t tokens[], int *token_index, command_t *cmd);
int parse_pipeline(token_t tokens[], pipeline_t *pipeline);
void free_command(command_t *cmd);
void free_pipeline(pipeline_t *pipeline) ;
void print_command(const command_t *cmd);
void print_pipeline(const pipeline_t *pipeline);
// sequential for
int parse_command_sequence(token_t tokens[], command_sequence_t *sequence);
void free_command_sequence(command_sequence_t *sequence);
int execute_command_sequence(command_sequence_t *sequence);
void init_pipeline(pipeline_t *pipeline);
void init_command_sequence(command_sequence_t *sequence);
int parse_command_with_multiple_redirections(token_t tokens[], int *token_index, command_t *cmd);
#endif
