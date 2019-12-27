#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

// help message for when a user types in command "help"
#define HELP_MESSAGE "Available commands:\n" \
"help (0 args) Print this help message\n" \
"quit (0 args) Quit the program\n" \
"enable (0 args) Allow jobs to start\n" \
"disable (0 args) Prevent jobs from starting\n" \
"spool (1 args) Spool a new job\n" \
"pause (1 args) Pause a running job\n" \
"resume (1 args) Resume a paused job\n" \
"cancel (1 args) Cancel an unfinished job\n" \
"expunge (1 args) Expunge a finished job\n" \
"status (1 args) Print the status of the job\n" \
"jobs (0 args) Print the status of all jobs\n"

typedef struct JOB {
    int job_index; // slot index of job on the table
    JOB_STATUS job_state; // status of job in table index (enclosed in brackets)
    TASK *job_task; // task at hand (from spool command)
    char *task_spec; // task string
    pid_t job_pgid; // process group of job
} JOB;

// array of structs used to initialize job table
struct JOB job_table[MAX_JOBS];

// enable/disable flag
int enable_flag;

// print JOB contents
void print_job(int job_id);

// number of running jobs
int running_jobs;

// atomic variable used by handler
volatile sig_atomic_t pid_test;

// SIGCHLD handler implementation
void chld_handler(int num);

// set process group id of job
void set_job_pgid(int jobid, pid_t pid);

// get the job id (needed for signal handling)
int get_job_from_pgid(pid_t pid);

