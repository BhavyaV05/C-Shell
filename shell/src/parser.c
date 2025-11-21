#include "parser.h"
// LLM CODE BEGINS
void init_pipeline(pipeline_t *pipeline) {
    pipeline->num_commands = 0;
    pipeline->background = 0;
    for (int i = 0; i < MAX_PIPELINE_COMMANDS; i++) {
        init_command(&pipeline->commands[i]);
    }
}

// Initialize a command sequence structure
void init_command_sequence(command_sequence_t *sequence) {
    sequence->num_pipelines = 0;
    for (int i = 0; i < MAX_SEQUENCE_PIPELINES; i++) {
        init_pipeline(&sequence->pipelines[i]);
    }
}

// void parse_input(const char *input){
    // skip whitespace
static const char* skip_ws(const char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    return s;
}

// check if a word is NAME
static int is_name_char(char c) {
    return (c && !isspace((unsigned char)c) && c!='|' && c!='&' && c!=';' && c!='<' && c!='>');
}

// --- main parser
int parse_input(const char *input) {
    const char *s = input;
    int expect_name = 1;   // after pipe/semicolon/input/output we need a NAME
    int last_was_op = 0;

    s = skip_ws(s);

    while (*s) {
        if (*s == '|') {
            if (expect_name) return 0;  // invalid: operator without name before
            expect_name = 1;
            last_was_op = 1;
            s++;
        }
        else if (*s == ';') {
            if (expect_name) return 0;  // invalid
            expect_name = 1;
            last_was_op = 1;
            s++;
        }
        else if (*s == '&') {
            // & can be end OR separator
            if (expect_name) return 0;
            expect_name = 1;
            last_was_op = 1;
            s++;
        }
        else if (*s == '<') {
            s++;
            s = skip_ws(s);
            if (!is_name_char(*s)) return 0;
            while (is_name_char(*s)) s++;
            expect_name = 0;
            last_was_op = 0;
        }
        else if (*s == '>') {
            s++;
            if (*s == '>') s++;  // handle >>
            s = skip_ws(s);
            if (!is_name_char(*s)) return 0;
            while (is_name_char(*s)) s++;
            expect_name = 0;
            last_was_op = 0;
        }
        else if (is_name_char(*s)) {
            while (is_name_char(*s)) s++;
            expect_name = 0;
            last_was_op = 0;
        }
        else if (isspace((unsigned char)*s)) {
            s = skip_ws(s);
            continue;
        }
        else {
            return 0; // invalid char
        }
        s = skip_ws(s);
    }

    // final checks
    if (expect_name && !last_was_op) return 0;

    return 1; // valid
}


// Tokenizer function
int tokenize(const char *input, token_t tokens[]) {
    int token_count = 0;
    int i = 0;
    int len = strlen(input);
    
    while (i < len && token_count < MAX_TOKENS - 1) {
        // Skip whitespace
        while (i < len && isspace(input[i])) {
            i++;
        }
        
        if (i >= len) break;
        
        token_t *current_token = &tokens[token_count];
        int value_index = 0;
        
        // Check for special characters
        switch (input[i]) {
            case '|':
                current_token->type = TOKEN_PIPE;
                strcpy(current_token->value, "|");
                i++;
                break;
                
            case '<':
                current_token->type = TOKEN_REDIRECT_IN;
                strcpy(current_token->value, "<");
                i++;
                break;
                
            case '>':
                if (i + 1 < len && input[i + 1] == '>') {
                    current_token->type = TOKEN_REDIRECT_APPEND;
                    strcpy(current_token->value, ">>");
                    i += 2;
                } else {
                    current_token->type = TOKEN_REDIRECT_OUT;
                    strcpy(current_token->value, ">");
                    i++;
                }
                break;
                
            case '&':
                if (i + 1 < len && input[i + 1] == '&') {
                    current_token->type = TOKEN_AND;
                    strcpy(current_token->value, "&&");
                    i += 2;
                } else {
                    current_token->type = TOKEN_BACKGROUND;
                    strcpy(current_token->value, "&");
                    i++;
                }
                break;
                
            case ';':
                current_token->type = TOKEN_SEMICOLON;
                strcpy(current_token->value, ";");
                i++;
                break;
                
            case '"':
                // Handle quoted strings
                current_token->type = TOKEN_WORD;
                i++; // Skip opening quote
                while (i < len && input[i] != '"' && value_index < MAX_TOKEN_LENGTH - 1) {
                    if (input[i] == '\\' && i + 1 < len) {
                        i++; // Skip backslash
                        current_token->value[value_index++] = input[i++];
                    } else {
                        current_token->value[value_index++] = input[i++];
                    }
                }
                if (i < len && input[i] == '"') i++; // Skip closing quote
                current_token->value[value_index] = '\0';
                break;
                
            case '\'':
                // Handle single quoted strings
                current_token->type = TOKEN_WORD;
                i++; // Skip opening quote
                while (i < len && input[i] != '\'' && value_index < MAX_TOKEN_LENGTH - 1) {
                    current_token->value[value_index++] = input[i++];
                }
                if (i < len && input[i] == '\'') i++; // Skip closing quote
                current_token->value[value_index] = '\0';
                break;
                
            default:
                // Handle regular words
                current_token->type = TOKEN_WORD;
                while (i < len && !isspace(input[i]) && 
                       input[i] != '|' && input[i] != '<' && input[i] != '>' && 
                       input[i] != '&' && input[i] != ';' && input[i] != '"' && 
                       input[i] != '\'' && value_index < MAX_TOKEN_LENGTH - 1) {
                    current_token->value[value_index++] = input[i++];
                }
                current_token->value[value_index] = '\0';
                break;
        }
        
        token_count++;
    }
    
    // Add EOF token
    tokens[token_count].type = TOKEN_EOF;
    strcpy(tokens[token_count].value, "");
    
    return token_count;
}

// Initialize a command structure
void init_command(command_t *cmd) {
    cmd->argc = 0;
    cmd->input_file = NULL;
    cmd->output_file = NULL;
    cmd->append_output = 0;
    cmd->background = 0;
    for (int i = 0; i < MAX_TOKENS; i++) {
        cmd->args[i] = NULL;
    }
}

// Parse tokens into commands
int parse_command(token_t tokens[], int *token_index, command_t *cmd) {
    init_command(cmd);
    
    while (tokens[*token_index].type != TOKEN_EOF &&
           tokens[*token_index].type != TOKEN_PIPE &&
           tokens[*token_index].type != TOKEN_SEMICOLON &&
           tokens[*token_index].type != TOKEN_AND &&
           tokens[*token_index].type != TOKEN_OR) {
        
        token_t *current = &tokens[*token_index];
        
        switch (current->type) {
            case TOKEN_WORD:
                if (cmd->argc < MAX_TOKENS - 1) {
                    cmd->args[cmd->argc] = malloc(strlen(current->value) + 1);
                    strcpy(cmd->args[cmd->argc], current->value);
                    cmd->argc++;
                }
                break;
                
            case TOKEN_REDIRECT_IN:
                (*token_index)++;
                if (tokens[*token_index].type == TOKEN_WORD) {
                    cmd->input_file = malloc(strlen(tokens[*token_index].value) + 1);
                    strcpy(cmd->input_file, tokens[*token_index].value);
                }
                break;
                
            case TOKEN_REDIRECT_OUT:
                (*token_index)++;
                if (tokens[*token_index].type == TOKEN_WORD) {
                    cmd->output_file = malloc(strlen(tokens[*token_index].value) + 1);
                    strcpy(cmd->output_file, tokens[*token_index].value);
                    cmd->append_output = 0;
                }
                break;
                
            case TOKEN_REDIRECT_APPEND:
                (*token_index)++;
                if (tokens[*token_index].type == TOKEN_WORD) {
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

// Parse a complete pipeline
int parse_pipeline(token_t tokens[], pipeline_t *pipeline) {
    init_pipeline(pipeline);
    
    int token_index = 0;
    int command_count = 0;
    
    while (tokens[token_index].type != TOKEN_EOF && 
           tokens[token_index].type != TOKEN_SEMICOLON &&
           command_count < MAX_PIPELINE_COMMANDS) {
        
        command_t *cmd = &pipeline->commands[command_count];
        
        // Use the new parsing function instead of the old one
        if (!parse_command_with_multiple_redirections(tokens, &token_index, cmd)) {
            return 0; // Parse error
        }
        
        command_count++;
        
        // Check if we hit a pipe
        if (tokens[token_index].type == TOKEN_PIPE) {
            token_index++; // Skip the pipe token
        } else {
            break; // No more commands in this pipeline
        }
    }
    
    // Check for background operator at the end of pipeline
    if (tokens[token_index].type == TOKEN_BACKGROUND) {
        pipeline->background = 1;
        token_index++;
    }
    
    pipeline->num_commands = command_count;
    return (command_count > 0);
}
int parse_single_pipeline_from_tokens(token_t tokens[], int *token_index, pipeline_t *pipeline) {
    init_pipeline(pipeline);
    
    int command_count = 0;
    
    while (tokens[*token_index].type != TOKEN_EOF && 
           tokens[*token_index].type != TOKEN_SEMICOLON &&
           command_count < MAX_PIPELINE_COMMANDS) {
        
        command_t *cmd = &pipeline->commands[command_count];
        
        // Use the new parsing function
        if (!parse_command_with_multiple_redirections(tokens, token_index, cmd)) {
            return 0; // Parse error
        }
        
        command_count++;
        
        // Check if we hit a pipe
        if (tokens[*token_index].type == TOKEN_PIPE) {
            (*token_index)++; // Skip the pipe token
        } else {
            break; // No more commands in this pipeline
        }
    }
    
    // Check for background operator at the end of pipeline
    if (tokens[*token_index].type == TOKEN_BACKGROUND) {
        pipeline->background = 1;
        (*token_index)++;
    }
    
    pipeline->num_commands = command_count;
    return (command_count > 0);
}

// Free memory allocated for a command
void free_command(command_t *cmd) {
    for (int i = 0; i < cmd->argc; i++) {
        if (cmd->args[i]) {
            free(cmd->args[i]);
            cmd->args[i] = NULL;
        }
    }
    if (cmd->input_file) {
        free(cmd->input_file);
        cmd->input_file = NULL;
    }
    if (cmd->output_file) {
        free(cmd->output_file);
        cmd->output_file = NULL;
    }
}

// Free memory allocated for a pipeline
void free_pipeline(pipeline_t *pipeline) {
    for (int i = 0; i < pipeline->num_commands; i++) {
        free_command(&pipeline->commands[i]);
    }
}

// Print a command (for debugging)
void print_command(const command_t *cmd) {
    printf("Command: ");
    for (int i = 0; i < cmd->argc; i++) {
        printf("%s ", cmd->args[i]);
    }
    printf("\n");
    
    if (cmd->input_file) {
        printf("  Input: %s\n", cmd->input_file);
    }
    if (cmd->output_file) {
        printf("  Output: %s %s\n", cmd->output_file, 
               cmd->append_output ? "(append)" : "(overwrite)");
    }
    if (cmd->background) {
        printf("  Background: yes\n");
    }
}

// Print a pipeline (for debugging)
void print_pipeline(const pipeline_t *pipeline) {
    printf("Pipeline with %d command(s):\n", pipeline->num_commands);
    for (int i = 0; i < pipeline->num_commands; i++) {
        printf("Command %d:\n", i + 1);
        print_command(&pipeline->commands[i]);
    }
    if (pipeline->background) {
        printf("Pipeline runs in background\n");
    }
}

// for sequentials
// Helper function to parse a single pipeline from tokens starting at token_index
int parse_pipeline_from_tokens(token_t tokens[], int *token_index, pipeline_t *pipeline) {
    pipeline->num_commands = 0;
    pipeline->background = 0;
    
    while (tokens[*token_index].type != TOKEN_EOF && 
           tokens[*token_index].type != TOKEN_SEMICOLON &&
           pipeline->num_commands < MAX_TOKENS) {
        
        // Parse one command
        if (!parse_command(tokens, token_index, &pipeline->commands[pipeline->num_commands])) {
            return 0;
        }
        pipeline->num_commands++;
        
        // Check for pipe
        if (tokens[*token_index].type == TOKEN_PIPE) {
            (*token_index)++; // Skip the pipe
            continue;
        } else if (tokens[*token_index].type == TOKEN_BACKGROUND) {
            pipeline->background = 1;
            (*token_index)++; // Skip the &
            break;
        } else {
            break; // End of this pipeline
        }
    }
    
    return pipeline->num_commands > 0;
}

// Parse a sequence of commands separated by semicolons
int parse_command_sequence(token_t tokens[], command_sequence_t *sequence) {
    init_command_sequence(sequence);
    
    int token_index = 0;
    int pipeline_count = 0;
    
    while (tokens[token_index].type != TOKEN_EOF && 
           pipeline_count < MAX_SEQUENCE_PIPELINES) {
        
        pipeline_t *pipeline = &sequence->pipelines[pipeline_count];
        
        // Parse each pipeline using updated logic
        if (!parse_single_pipeline_from_tokens(tokens, &token_index, pipeline)) {
            return 0; // Parse error
        }
        
        pipeline_count++;
        
        // Check if we hit a semicolon
        if (tokens[token_index].type == TOKEN_SEMICOLON) {
            token_index++; // Skip the semicolon
        } else {
            break; // No more pipelines
        }
    }
    
    sequence->num_pipelines = pipeline_count;
    return (pipeline_count > 0);
}


// Free command sequence
void free_command_sequence(command_sequence_t *sequence) {
    for (int i = 0; i < sequence->num_pipelines; i++) {
        free_pipeline(&sequence->pipelines[i]);
    }
    sequence->num_pipelines = 0;
}
// LLM CODE ENDS
