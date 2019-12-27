#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "../include/sf_readline.h"
#include "../include/debug.h"
#include "jobber.h"
#include "task.h"
#include "helper.h"

// print JOB descriptions
void print_job(int job_id) {
    if (job_table[job_id].job_state == NEW) {
        printf("job %d [%s]: ", job_table[job_id].job_index, job_status_names[0]);
        unparse_task(job_table[job_id].job_task, stdout);
        printf("\n");
    }
    else if (job_table[job_id].job_state == WAITING ) {
        printf("job %d [%s]: ", job_table[job_id].job_index, job_status_names[1]);
        unparse_task(job_table[job_id].job_task, stdout);
        printf("\n");
    }
    else if (job_table[job_id].job_state == RUNNING) {
        printf("job %d [%s]: ", job_table[job_id].job_index, job_status_names[2]);
        unparse_task(job_table[job_id].job_task, stdout);
        printf("\n");
    }
    else if (job_table[job_id].job_state == PAUSED) {
        printf("job %d [%s]: ", job_table[job_id].job_index, job_status_names[3]);
        unparse_task(job_table[job_id].job_task, stdout);
        printf("\n");
    }
    else if (job_table[job_id].job_state == CANCELED) {
        printf("job %d [%s]: ", job_table[job_id].job_index, job_status_names[4]);
        unparse_task(job_table[job_id].job_task, stdout);
        printf("\n");
    }
    else if (job_table[job_id].job_state == COMPLETED) {
        printf("job %d [%s]: ", job_table[job_id].job_index, job_status_names[5]);
        unparse_task(job_table[job_id].job_task, stdout);
        printf("\n");
    }
    else if (job_table[job_id].job_state == ABORTED) {
        printf("job %d [%s]: ", job_table[job_id].job_index, job_status_names[6]);
        unparse_task(job_table[job_id].job_task, stdout);
        printf("\n");
    }
    else {
        return;
    }
}

// handle child processes
void chld_handler(int num) {
    // save errno for return
    int errno_saved = errno;

    // check the various options with status
    int status;

    // waitpid is necessary for all signals, get pid_test (runner pid)
    pid_test = waitpid(-1, &status, WUNTRACED | WCONTINUED);

    // child process was killed
    if(WIFSIGNALED(status)) {
        // change state to ABORTED
        job_table[get_job_from_pgid(pid_test)].job_state = ABORTED;

        // call sf_job_status_change
        sf_job_status_change(get_job_from_pgid(pid_test), COMPLETED, ABORTED);

        // call sf_job_end with signal from wtermsig
        sf_job_end(get_job_from_pgid(pid_test), pid_test, WTERMSIG(status));

        // decrease number of runners
        running_jobs--;

        errno = errno_saved;

        // no need to execute below so we return
        return;
    }

    // child process was stopped (paused)
    if(WIFSTOPPED(status)) {
        // call to sf_job_pause
        sf_job_pause(get_job_from_pgid(pid_test), job_get_pgid(get_job_from_pgid(pid_test)));

        errno = errno_saved;

        // no need to execute below so we return
        return;
    }

    // child process was continued (resumed)
    if(WIFCONTINUED(&status)) {
        // call to sf_job_resume
        sf_job_resume(get_job_from_pgid(pid_test), job_get_pgid(get_job_from_pgid(pid_test)));

        errno = errno_saved;

        // no need to execute below so we return
        return;
    }

    // change state to COMPLETED
    job_table[get_job_from_pgid(pid_test)].job_state = COMPLETED;

    // call sf_job_status_change
    sf_job_status_change(get_job_from_pgid(pid_test), RUNNING, COMPLETED);

    // call sf_job_end after execution
    sf_job_end(get_job_from_pgid(pid_test), pid_test, WEXITSTATUS(status));

    // decrease number of runners
    running_jobs--;

    errno = errno_saved;
}

// set process group id of job
void set_job_pgid(int jobid, pid_t pid) {
    // set process group id onto the task struct
    job_table[jobid].job_pgid = pid;
}

// get job id from process group id of job
int get_job_from_pgid(pid_t pid) {
    // return length - 1 aka the index of job
    int i = 0;

    while (job_table[i].job_pgid != pid) {
        i++;
    }

    return i;
}