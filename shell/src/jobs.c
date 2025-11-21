#include "jobs.h"

// Global job tracking
job_t jobs[MAX_JOBS];
int next_job_id = 1;

pid_t current_foreground_pid;  // Track current foreground process
pid_t current_foreground_pgid;
char current_foreground_command[MAX_COMMAND_NAME] = "";
// LLM CODE BEGINS
void init_job_system() {
    for (int i = 0; i < MAX_JOBS; i++) {
        jobs[i].active = 0;
        jobs[i].state = JOB_TERMINATED;
    }
    next_job_id = 1;
    current_foreground_pid = 0;
    current_foreground_pgid = 0;
    
    // Setup signal handlers
    setup_signal_handlers();
}
void sigint_handler(int sig) {
    if (current_foreground_pgid > 0) {
        // Send SIGINT to the foreground process group
        kill(-current_foreground_pgid, SIGINT);
    }
    // Don't terminate the shell itself
    printf("\n");
    fflush(stdout);
}
// Signal handler for SIGTSTP (Ctrl-Z)
void sigtstp_handler(int sig) {
    if (current_foreground_pid > 0) {
        // Send SIGTSTP to the foreground process group
        kill(-current_foreground_pgid, SIGTSTP);
        
        // Add the stopped process to job list
        int job_id = add_stopped_job(current_foreground_pid, current_foreground_command);
        if (job_id > 0) {
            printf("\n[%d] Stopped %s\n", job_id, current_foreground_command);
        }
        
        // Clear foreground process
        current_foreground_pid = 0;
        current_foreground_pgid = 0;
        current_foreground_command[0] = '\0';
    } else {
        printf("\n");
    }
    fflush(stdout);
}
void setup_signal_handlers() {
    struct sigaction sa_int, sa_tstp;
    
    // Setup SIGINT handler (Ctrl-C)
    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa_int, NULL);
    
    // Setup SIGTSTP handler (Ctrl-Z)
    sa_tstp.sa_handler = sigtstp_handler;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa_tstp, NULL);
}

int add_background_job(pid_t pid, const char *command_name) {
    // Find an empty slot
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!jobs[i].active) {
            jobs[i].job_id = next_job_id++;
            jobs[i].pid = pid;
            strncpy(jobs[i].command_name, command_name, MAX_COMMAND_NAME - 1);
            jobs[i].command_name[MAX_COMMAND_NAME - 1] = '\0';
            jobs[i].state = JOB_RUNNING;
            jobs[i].active = 1;
            
            // Print job started message
            printf("[%d] %d\n", jobs[i].job_id, pid);
            
            return jobs[i].job_id;
        }
    }
    return -1; // No slots available
}

int add_stopped_job(pid_t pid, const char *command_name) {
    // Find an empty slot
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!jobs[i].active) {
            jobs[i].job_id = next_job_id++;
            jobs[i].pid = pid;
            strncpy(jobs[i].command_name, command_name, MAX_COMMAND_NAME - 1);
            jobs[i].command_name[MAX_COMMAND_NAME - 1] = '\0';
            jobs[i].state = JOB_STOPPED;
            jobs[i].active = 1;
            
            return jobs[i].job_id;
        }
    }
    return -1; // No slots available
}

void kill_all_children() {
    // Kill all active background jobs
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].active) {
            kill(jobs[i].pid, SIGKILL);
            jobs[i].active = 0;
        }
    }
    
    // Also kill current foreground process if any
    if (current_foreground_pid > 0) {
        kill(-current_foreground_pgid, SIGKILL);
    }
}

void update_job_states() {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].active) {
            int status;
            pid_t result = waitpid(jobs[i].pid, &status, WNOHANG | WUNTRACED);
            
            if (result == jobs[i].pid) {
                if (WIFEXITED(status) || WIFSIGNALED(status)) {
                    // Process has terminated
                    jobs[i].state = JOB_TERMINATED;
                    jobs[i].active = 0; // Remove from active list
                } else if (WIFSTOPPED(status)) {
                    // Process is stopped
                    jobs[i].state = JOB_STOPPED;
                }
            } else if (result == -1) {
                // Process no longer exists
                jobs[i].state = JOB_TERMINATED;
                jobs[i].active = 0;
            }
            // result == 0 means process is still running (state remains JOB_RUNNING)
        }
    }
}

void check_background_jobs() {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].active) {
            int status;
            pid_t result = waitpid(jobs[i].pid, &status, WNOHANG | WUNTRACED);
            
            if (result == jobs[i].pid) {
                if (WIFEXITED(status) || WIFSIGNALED(status)) {
                    // Process has completed - print completion message
                    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                        printf("%s with pid %d exited normally\n", 
                               jobs[i].command_name, jobs[i].pid);
                    } else {
                        printf("%s with pid %d exited abnormally\n", 
                               jobs[i].command_name, jobs[i].pid);
                    }
                    
                    // Mark job as inactive
                    jobs[i].state = JOB_TERMINATED;
                    jobs[i].active = 0;
                } else if (WIFSTOPPED(status)) {
                    // Process is stopped
                    jobs[i].state = JOB_STOPPED;
                }
            } else if (result == -1) {
                // Process no longer exists
                jobs[i].active = 0;
                jobs[i].state = JOB_TERMINATED;
            }
        }
    }
}

int compare_jobs(const void *a, const void *b) {
    const job_t *job_a = (const job_t *)a;
    const job_t *job_b = (const job_t *)b;
    
    // Inactive jobs go to the end
    if (!job_a->active && job_b->active) return 1;
    if (job_a->active && !job_b->active) return -1;
    if (!job_a->active && !job_b->active) return 0;
    
    // Compare by command name lexicographically
    return strcmp(job_a->command_name, job_b->command_name);
}

int activities_command() {
    // Update job states first
    update_job_states();
    
    // Create a copy of jobs array for sorting
    job_t sorted_jobs[MAX_JOBS];
    int active_count = 0;
    
    // Copy only active jobs
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].active) {
            sorted_jobs[active_count] = jobs[i];
            active_count++;
        }
    }
    
    if (active_count == 0) {
        // No active processes
        return 0;
    }
    
    // Sort jobs by command name
    qsort(sorted_jobs, active_count, sizeof(job_t), compare_jobs);
    
    // Print sorted jobs
    for (int i = 0; i < active_count; i++) {
        const char *state_str;
        switch (sorted_jobs[i].state) {
            case JOB_RUNNING:
                state_str = "Running";
                break;
            case JOB_STOPPED:
                state_str = "Stopped";
                break;
            default:
                state_str = "Unknown";
                break;
        }
        
        printf("[%d] : %s - %s\n", 
               sorted_jobs[i].pid, 
               sorted_jobs[i].command_name, 
               state_str);
    }
    
    return 0;
}
// Helper function to find job by job ID
job_t* find_job_by_id(int job_id) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].active && jobs[i].job_id == job_id) {
            return &jobs[i];
        }
    }
    return NULL;
}

// Helper function to get most recent job ID
int get_most_recent_job_id() {
    int most_recent_id = -1;
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].active && jobs[i].job_id > most_recent_id) {
            most_recent_id = jobs[i].job_id;
        }
    }
    return most_recent_id;
}

// Implementation of fg command
int fg_command(int argc, char *argv[]) {
    int target_job_id;
    job_t *target_job;
    
    // Determine which job to bring to foreground
    if (argc == 1) {
        // No job number provided, use most recent
        target_job_id = get_most_recent_job_id();
        if (target_job_id == -1) {
            printf("No jobs to bring to foreground\n");
            return 1;
        }
    } else if (argc == 2) {
        // Job number provided
        char *endptr;
        target_job_id = (int)strtol(argv[1], &endptr, 10);
        if (*endptr != '\0' || target_job_id <= 0) {
            printf("Invalid job number: %s\n", argv[1]);
            return 1;
        }
    } else {
        printf("Usage: fg [job_number]\n");
        return 1;
    }
    
    // Find the job
    target_job = find_job_by_id(target_job_id);
    if (target_job == NULL) {
        printf("No such job\n");
        return 1;
    }
    
    // Print the command being brought to foreground
    printf("%s\n", target_job->command_name);
    
    // Set as current foreground process
    current_foreground_pid = target_job->pid;
    current_foreground_pgid = target_job->pid;
    
    // If job is stopped, send SIGCONT to resume it
    if (target_job->state == JOB_STOPPED) {
        if (kill(target_job->pid, SIGCONT) == -1) {
            perror("Failed to resume job");
            current_foreground_pid = 0;
            current_foreground_pgid = 0;
            return 1;
        }
    }
    
    // Update job state to running
    target_job->state = JOB_RUNNING;
    
    // Wait for the job to complete or stop
    int status;
    do {
        waitpid(target_job->pid, &status, WUNTRACED);
        
        if (WIFSTOPPED(status)) {
            // Process was stopped again
            target_job->state = JOB_STOPPED;
            printf("\n[%d] Stopped %s\n", target_job->job_id, target_job->command_name);
            break;
        } else if (WIFEXITED(status) || WIFSIGNALED(status)) {
            // Process completed
            target_job->active = 0;
            target_job->state = JOB_TERMINATED;
            break;
        }
    } while (1);
    
    // Clear foreground process
    current_foreground_pid = 0;
    current_foreground_pgid = 0;
    
    return 0;
}

// Implementation of bg command
int bg_command(int argc, char *argv[]) {
    int target_job_id;
    job_t *target_job;
    
    // Determine which job to resume in background
    if (argc == 1) {
        // No job number provided, use most recent
        target_job_id = get_most_recent_job_id();
        if (target_job_id == -1) {
            printf("No jobs to resume in background\n");
            return 1;
        }
    } else if (argc == 2) {
        // Job number provided
        char *endptr;
        target_job_id = (int)strtol(argv[1], &endptr, 10);
        if (*endptr != '\0' || target_job_id <= 0) {
            printf("Invalid job number: %s\n", argv[1]);
            return 1;
        }
    } else {
        printf("Usage: bg [job_number]\n");
        return 1;
    }
    
    // Find the job
    target_job = find_job_by_id(target_job_id);
    if (target_job == NULL) {
        printf("No such job\n");
        return 1;
    }
    
    // Check if job is already running
    if (target_job->state == JOB_RUNNING) {
        printf("Job already running\n");
        return 1;
    }
    
    // Check if job is stopped (only stopped jobs can be resumed with bg)
    if (target_job->state != JOB_STOPPED) {
        printf("Job is not stopped\n");
        return 1;
    }
    
    // Send SIGCONT to resume the job
    if (kill(target_job->pid, SIGCONT) == -1) {
        perror("Failed to resume job");
        return 1;
    }
    
    // Update job state to running
    target_job->state = JOB_RUNNING;
    
    // Print resumption message
    printf("[%d] %s &\n", target_job->job_id, target_job->command_name);
    
    return 0;
}
// LLM CODE ENDS
