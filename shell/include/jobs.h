#include "prompt.h"
#ifndef JOBS_H
#define JOBS_H

#define MAX_JOBS 100
#define MAX_COMMAND_NAME 256

typedef enum {
    JOB_RUNNING,
    JOB_STOPPED,
    JOB_TERMINATED
} job_state_t;

typedef struct {
    int job_id;
    pid_t pid;
    char command_name[MAX_COMMAND_NAME];
    job_state_t state;
    int active;
} job_t;

// Global job management
extern job_t jobs[MAX_JOBS];
extern int next_job_id;
extern pid_t current_foreground_pid;  // Track current foreground process
extern pid_t current_foreground_pgid; // Track current foreground process group

// Function declarations
int add_background_job(pid_t pid, const char *command_name);
void check_background_jobs();
void init_job_system();
int activities_command(); // New function for activities command
void update_job_states(); // New function to update job states
int add_stopped_job(pid_t pid, const char *command_name);
void setup_signal_handlers();
void kill_all_children();
int fg_command(int argc, char *argv[]);
int bg_command(int argc, char *argv[]);
job_t* find_job_by_id(int job_id);
int get_most_recent_job_id();

#endif