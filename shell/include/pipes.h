#include "functs.h"
#include "parser.h"
#ifndef PIPES_H
#define PIPES_H

int execute_external_command(char *args[], char *input_file, char *output_file, int append_output, int background);
int execute_single_command(command_t *cmd);
int execute_simple_pipeline(pipeline_t *pipeline);
int execute_command_line(char *input_line);
// Function declarations
int parse_command_with_multiple_redirections(token_t tokens[], int *token_index, command_t *cmd);
int parse_single_pipeline_from_tokens(token_t tokens[], int *token_index, pipeline_t *pipeline);

#endif // PIPES_H