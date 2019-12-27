/*
 * Job manager for "jobber".
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include "jobber.h"
#include "task.h"
#include "helper.h"

int jobs_init(void) {
    // initialize the job spooler (job table)
    JOB job_table[MAX_JOBS - 1] = {{0}};
    //JOB *job_table = (JOB *) malloc(MAX_JOBS * sizeof(JOB));

    if (job_table[MAX_JOBS].job_index) {
    }

    return 0;
}

void jobs_fini(void) {
    // cancel all jobs that aren't already completed or aborted
    int i = 0;
    while (job_table[i].job_task != NULL) {
        if (job_table[i].job_state != COMPLETED || job_table[i].job_state != ABORTED) {
            job_cancel(i);
        }
        i++;
    }

    // expunge all jobs
    i = 0;
    while (job_table[i].job_task != NULL) {
        job_expunge(i);
        i++;
    }

    // void
    return;
}

int jobs_set_enabled(int val) {
    // set the enable flag
    if (val == 0) {
        enable_flag = 1;
    }
    else {
        enable_flag = 0;
    }

    // return previous flag value
    return val;
}

int jobs_get_enabled() {
    // check if flag is enabled
    if (enable_flag == 1) {
        return 1;
    }
    else {
        return 0;
    }
}

int job_create(char *command) {
    // create a new job to run the task

    // allocate memory for command copy
    char *command_cpy = (char *) malloc(256);
    strcpy(command_cpy, command);

    // parse the task
    TASK *task_to_run;
    task_to_run = parse_task(&command_cpy);

    // null task
    if(task_to_run == NULL) {
        return -1;
    }

    // job table is full
    if (job_table[MAX_JOBS - 1].job_task != NULL) {
        return -1;
    }

    // syntactically well formed check ------

    // allocate entry in job table for this new job
    int i = 0;
    while(job_table[i].job_task != NULL) {
        i++;
    }

    // index of an empty job slot starts here
    // set job index to i
    job_table[i].job_index = i;

    // set status to NEW
    job_table[i].job_state = NEW;

    // set task
    job_table[i].job_task = task_to_run;

    // set task spec string
    job_table[i].task_spec = command;

    // PRINT TASK OUT HERE BECAUSE SF_JOB_CREATE ISN'T PRINTING IT
    printf("TASK: %s\n", command);

    // call to sf_job_create because job has been INIT
    sf_job_create(i);

    // now change the status to WAITING
    job_table[i].job_state = WAITING;

    // call to sf_job_status_change after changes
    sf_job_status_change(i, NEW, WAITING);

    // return the job id
    return job_table[i].job_index;
}

int job_expunge(int jobid) {
    // empty JOBS struct
    static const JOB clear_job;

    // check if id exists
    if (jobid < 0 || jobid > 7) {
        return -1;
    }
    else if (job_table[jobid].job_index != jobid) {
        // non-existent job id
        return -1;
    }
    else if (job_table[jobid].job_state == COMPLETED ||
             job_table[jobid].job_state == ABORTED) {
        // free task because it is no longer needed
        //free_task(job_table[jobid].job_task);

        // clear out the items in jobid (expunge)
        job_table[jobid] = clear_job;

        // call sf_job_expunge
        sf_job_expunge(jobid);

        // success
        return 0;
    }
    else {
        // not in correct state
        return -1;
    }

    // failed point here
    return -1;
}

int job_cancel(int jobid) {
    if (jobid < 0 || jobid > 7) {
        return -1;
    }
    else if (job_table[jobid].job_index != jobid) {
        return -1;
    }
    else if (job_table[jobid].job_state == RUNNING ||
             job_table[jobid].job_state == PAUSED) {
        // change state to CANCELED
        job_table[jobid].job_state = CANCELED;

        // call sf_job_status_change
        if (job_table[jobid].job_state == RUNNING) {
            sf_job_status_change(jobid, RUNNING, CANCELED);
        }
        else if (job_table[jobid].job_state == PAUSED) {
            sf_job_status_change(jobid, PAUSED, CANCELED);
        }

        // call killpg with SIGKILL
        if (killpg(job_get_pgid(jobid), SIGKILL) == -1) {
            perror("killpg");
            return -1;
        }

        // success
        return 0;
    }
    else if (job_table[jobid].job_state == WAITING) {
        // change state to CANCELED
        job_table[jobid].job_state = ABORTED;

        // call sf_job_status_change
        sf_job_status_change(jobid, WAITING, ABORTED);

        // success
        return 0;
    }
    else {
        return -1;
    }
}

int job_pause(int jobid) {
    if (jobid < 0 || jobid > 7) {
        return -1;
    }
    else if (job_table[jobid].job_index != jobid) {
        return -1;
    }
    else if (job_table[jobid].job_state == RUNNING) {
        // change state to PAUSED
        job_table[jobid].job_state = PAUSED;

        // call sf_job_status_change
        sf_job_status_change(jobid, RUNNING, PAUSED);

        // call killpg with SIGSTOP
        if (killpg(job_get_pgid(jobid), SIGSTOP) == -1) {
            perror("killpg");
            return -1;
        }

        // success
        return 0;
    }
    else {
        return -1;
    }
}

int job_resume(int jobid) {
    if (jobid < 0 || jobid > 7) {
        return -1;
    }
    else if (job_table[jobid].job_index != jobid) {
        return -1;
    }
    else if (job_table[jobid].job_state == PAUSED) {
        // change state to RUNNING
        job_table[jobid].job_state = RUNNING;

        // call sf_job_status_change
        sf_job_status_change(jobid, PAUSED, RUNNING);

        // call killpg with SIGSTOP
        if (killpg(job_get_pgid(jobid), SIGCONT) == -1) {
            perror("killpg");
            return -1;
        }

        // success
        return 0;
    }
    else {
        return -1;
    }
}

int job_get_pgid(int jobid) {
    // get the process group ID of a job's runner process
    if (jobid < 0 || jobid > 7) {
        return -1;
    }
    else if (job_table[jobid].job_index != jobid) {
        return -1;
    }
    else if (job_table[jobid].job_state == RUNNING ||
             job_table[jobid].job_state == PAUSED ||
             job_table[jobid].job_state == CANCELED) {
        // return pgid

        return job_table[jobid].job_pgid;
    }
    else {
        return -1;
    }
}

JOB_STATUS job_get_status(int jobid) {
    // get the current status of jobid
    if (jobid < 0 || jobid > 7) {
        return -1;
    }
    else if (job_table[jobid].job_index != jobid) {
        return -1;
    }
    else {
        return job_table[jobid].job_state;
    }
}

int job_get_result(int jobid) {
    // get the exit status of jobid
    // get return value from waitpid function call
    int exit_status = waitpid(job_get_pgid(jobid), NULL, 0);

    if (jobid < 0 || jobid > 7) {
        return -1;
    }
    else if (job_table[jobid].job_index != jobid) {
        return -1;
    }
    else if (job_table[jobid].job_state != COMPLETED) {
        return -1;
    }
    else {
        return exit_status;
    }
}

int job_was_canceled(int jobid) {
    // determine if job was successfully canceled
    // if it is in ABORTED state, return 1, else return 0
    if (jobid < 0 || jobid > 7) {
        return 0;
    }
    else if (job_table[jobid].job_index != jobid) {
        return 0;
    }
    else if (job_table[jobid].job_state != ABORTED) {
        return 0;
    }
    else {
        return 1; // canceled properly
    }
}

char *job_get_taskspec(int jobid) {
    // get task from array struct
    if (jobid < 0 || jobid > 7) {
        return NULL;
    }
    else if (job_table[jobid].job_index != jobid) {
        return NULL;
    }
    else if (job_table[jobid].task_spec != NULL) {
        return job_table[jobid].task_spec;
    }
    else {
        return NULL;
    }
}
