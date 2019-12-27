#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "../include/sf_readline.h"
#include "../include/debug.h"
#include "jobber.h"
#include "task.h"
#include "helper.h"

/*
 * "Jobber" job spooler.
 */

int main(int argc, char *argv[])
{
    // necessary variables here (global)
    // call job init once
    char *command_buf;
    int buf_size;
    char *command;
    char *argument;
    char *to_be_converted_arg;
    char *space = " ";
    running_jobs = 0;
    sigset_t mask_all, mask_child, prev_one;
    sigfillset(&mask_all);
    sigemptyset(&mask_child);
    sigaddset(&mask_child, SIGCHLD);
    signal(SIGCHLD, chld_handler);
    jobs_init();

    // start the prompt and command processing

    while(1) {
        // if enabler flag is on, jobs will keep processing
        if (jobs_get_enabled() != 0) {
            // tasks + job_id array
            TASK *tasksv[MAX_JOBS - 1] = {NULL};

            // check to see how many jobs are in job table
            // and how many can be set to running

            int i;
            for (i = 0; i < MAX_JOBS; i++) {
                if (job_table[i].job_state == WAITING && running_jobs < MAX_RUNNERS) {
                    //incrememt running_jobs
                    running_jobs++;

                    // add task name to vector
                    tasksv[i] = job_table[i].job_task;

                    // create an ARGV for the task at i
                    char *argumentV[50] = {NULL};
                    struct WORD_LIST *rest_pointer;

                    if(tasksv[i] != NULL) {
                        int j = 0;
                        argumentV[j] = tasksv[i]->pipelines->first->commands->first->words->first;
                        if (tasksv[i]->pipelines->first->commands->first->words->rest != NULL) {
                            // non-null pointer
                            rest_pointer = tasksv[i]->pipelines->first->commands->first->words->rest;

                            // loop while non-null
                            while (rest_pointer->first != NULL) {
                                argumentV[j + 1] = rest_pointer->first;
                                j++;
                                if (rest_pointer->rest == NULL) {
                                    break;
                                }
                                else {
                                    rest_pointer = rest_pointer->rest;
                                }
                            }
                        }

                        // proc
                        sigprocmask(SIG_BLOCK, &mask_child, &prev_one);

                        // for this NON-NULL task, we will execute it using fork
                        pid_t pid = fork();

                        if (pid == 0) {
                            // this is the child process
                            // proc
                            sigprocmask(SIG_SETMASK, &prev_one, NULL);

                            // execvp system call
                            if (execvp(*argumentV, argumentV) == -1) {
                                perror("execvp");
                                exit(1);
                            }
                        }
                        else if (pid > 0) {
                                // this is the parent process (main/runner process?)
                                // proc
                                sigprocmask(SIG_BLOCK, &mask_all, NULL);

                                // set process group of runner
                                if (setpgid(pid, pid) == -1) {
                                    perror("setpgid");
                                    exit(1);
                                }

                                // job start
                                sf_job_start(i, getpgid(pid));

                                // change state to running
                                job_table[i].job_state = RUNNING;

                                // call sf_job_status_change
                                sf_job_status_change(i, WAITING, RUNNING);

                                // initialize pgid in task struct
                                set_job_pgid(i, getpgid(pid));

                                // proc
                                sigprocmask(SIG_SETMASK, &prev_one, NULL);
                        }
                        else {
                            // fork failure
                            perror("fork");
                            exit(1);
                        }

                    }

                }

            }

        }

        // command processing starts here
        command_buf = sf_readline("jobber> ");
        // save the original buff and size for later
        int i = 0;
        while (command_buf[i] != '\0') {
            i++;
        }
        buf_size = i;
        char *saved_buf = (char *) malloc(buf_size);
        for (i = 0; i < buf_size; i++) {
            saved_buf[i] = command_buf[i];
        }

        // first check if null buf
        if (!strcmp(command_buf, "")) {
            // nothing was entered, continue the loop
            continue;
        }

        // command buf not null, separate spaces as 2 arguments
        command = strtok(command_buf, space);

        // commands below
        if (!strcmp(command, "help")) { //----------------------------------------
            // error checker for number of args (must be 0)
            argument = strtok(NULL, space);
            int i = 0;
            while (argument != NULL) {
                argument = strtok(NULL, space);
                i++;
            }
            if (i > 0) {
                printf("Wrong number of args (given: %d, required: 0) for command '%s'\n", i, command);
            }
            else {
                // print help message
                printf(HELP_MESSAGE);
            }
        }
        else if (!strcmp(command, "quit")) { //----------------------------------------
            // error checker for number of args (must be 0)
            argument = strtok(NULL, space);
            int i = 0;
            while (argument != NULL) {
                argument = strtok(NULL, space);
                i++;
            }
            if (i > 0) {
                printf("Wrong number of args (given: %d, required: 0) for command '%s'\n", i, command);
            }
            else {
                // finalize spooler
                jobs_fini();

                // exit successfully
                exit(EXIT_SUCCESS);
            }
        }
        else if (!strcmp(command, "status")) { //----------------------------------------
            // error checker for number of args (must be 1)
            argument = strtok(NULL, space);
            to_be_converted_arg = argument;
            int i = 0;
            while (argument != NULL) {
                argument = strtok(NULL, space);
                i++;
            }
            if (i == 0 || i > 1) {
                printf("Wrong number of args (given: %d, required: 1) for command '%s'\n", i, command);
            }
            else {
                // change arg to int
                int int_arg = atoi(to_be_converted_arg);

                // validate int argument
                if (int_arg >= 0 && int_arg < MAX_JOBS) {
                    if (job_table[int_arg].job_task != NULL) {
                        // print job status of int argument
                        print_job(int_arg);
                    }
                    else {
                        // handle empty job slot
                        printf("No job available at position %d\n", int_arg);
                    }
                }
                else {
                    // handle incorrect int args number
                    printf("Invalid second argument: %d\n", int_arg);
                }
            }
        }
        else if (!strcmp(command, "jobs")) { //----------------------------------------
            // error checker for number of args (must be 0)
            argument = strtok(NULL, space);
            int i = 0;
            while (argument != NULL) {
                argument = strtok(NULL, space);
                i++;
            }
            if (i > 0) {
                printf("Wrong number of args (given: %d, required: 0) for command '%s'\n", i, command);
            }
            else {
                // check if enabled or not
                int check_enabled = jobs_get_enabled();
                if (check_enabled != 0) {
                    printf("Starting jobs is enabled\n");
                }
                else {
                    printf("Starting jobs is disabled\n");
                }

                // print out all jobs and their corresponding statuses
                i = 0;
                while (job_table[i].job_task != NULL) {
                    print_job(i);
                    i++;
                }
            }
        }
        else if (!strcmp(command, "enable")) { //----------------------------------------
            // error checker for number of args (must be 0)
            argument = strtok(NULL, space);
            int i = 0;
            while (argument != NULL) {
                argument = strtok(NULL, space);
                i++;
            }
            if (i > 0) {
                printf("Wrong number of args (given: %d, required: 0) for command '%s'\n", i, command);
            }
            else {
                // enable flag and start executing jobs right away
                jobs_set_enabled(0);
            }
        }
        else if (!strcmp(command, "disable")) { //----------------------------------------
            // error checker for number of args (must be 0)
            argument = strtok(NULL, space);
            int i = 0;
            while (argument != NULL) {
                argument = strtok(NULL, space);
                i++;
            }
            if (i > 0) {
                printf("Wrong number of args (given: %d, required: 0) for command '%s'\n", i, command);
            }
            else {
                // disable
                jobs_set_enabled(1);
            }
        }
        else if (!strcmp(command, "spool")) { //----------------------------------------
            // error checker for number of args (must be 1)
            // variables needed
            char *first_arg;
            char *last_arg;
            char *unquoted_arg;
            argument = strtok(NULL, space);
            // save this argument for later
            unquoted_arg = argument;
            i = 0;
            // save first argument
            first_arg = argument;
            while (argument != NULL) {
                i++;
                last_arg = argument;
                argument = strtok(NULL, space);
            }
            // if first argument is null
            if (first_arg == NULL) {
                printf("Wrong number of args (given: 0, required: 1) for command '%s'\n", command);
                continue;
            }
            // save chars
            char first_arg_char = *first_arg;
            // size of last argument
            int j = 0;
            while (last_arg[j] != '\0') {
                j++;
            }
            char last_arg_char = *(last_arg + j - 1);
            // before checking for valid number of args,
            // check if the argument is enclosed in brackets first
            if (i > 1) {
                if (first_arg_char == 39 && last_arg_char == 39) {
                    // arguments are enclosed in brackets
                    // command + space "spool '" = 7
                    j = 0;
                    while (*(saved_buf + j + 8) != 39) {
                        j++;
                    }
                    char *finished_arg = (char *) malloc(j + 1);
                    for (i = 0; i < j + 1; i++) {
                        finished_arg[i] = saved_buf[i + 7];
                    }

                    // create job with finished arg (spool)
                    int spool_check = job_create(finished_arg);

                    // error in spool
                    if (spool_check == -1) {
                        printf("Error: spool\n");
                    }

                    // free all pointers used
                    //free(saved_buf);
                    //free(finished_arg);
                }
                else {
                    // too many arguments
                    printf("Wrong number of args (given: %d, required: 1) for command '%s'\n", i, command);
                }
            }
            else if (i == 1 && first_arg_char == 39 && last_arg_char == 39) {
                    // arguments are enclosed in brackets
                    char *quote_to_unquote = strtok(unquoted_arg, "'");

                    // create job with quote to unquoted arg (spool)
                    int spool_check = job_create(quote_to_unquote);

                    // error in spool
                    if (spool_check == -1) {
                        printf("Error: spool\n");
                    }
            }
            else {
                 // create job with unquoted arg (spool)
                int spool_check = job_create(unquoted_arg);

                // error in spool
                if (spool_check == -1) {
                    printf("Error: spool\n");
                }
            }
        }
        else if (!strcmp(command, "pause")) { //----------------------------------------
            // error checker for number of args (must be 1)
            argument = strtok(NULL, space);
            to_be_converted_arg = argument;
            int i = 0;
            while (argument != NULL) {
                argument = strtok(NULL, space);
                i++;
            }
            if (i == 0 || i > 1) {
                printf("Wrong number of args (given: %d, required: 1) for command '%s'\n", i, command);
            }
            else {
                // change arg to int
                int int_arg = atoi(to_be_converted_arg);

                // pause job
                int pause_check = job_pause(int_arg);

                // error in pause
                if (pause_check == -1) {
                    printf("Error: pause\n");
                }
            }
        }
        else if (!strcmp(command, "resume")) { //----------------------------------------
            // error checker for number of args (must be 1)
            argument = strtok(NULL, space);
            to_be_converted_arg = argument;
            int i = 0;
            while (argument != NULL) {
                argument = strtok(NULL, space);
                i++;
            }
            if (i == 0 || i > 1) {
                printf("Wrong number of args (given: %d, required: 1) for command '%s'\n", i, command);
            }
            else {
                // change arg to int
                int int_arg = atoi(to_be_converted_arg);

                // resume job
                int resume_check = job_resume(int_arg);

                // error in resume
                if (resume_check == -1) {
                    printf("Error: resume\n");
                }
            }
        }
        else if (!strcmp(command, "cancel")) { //----------------------------------------
            // error checker for number of args (must be 1)
            argument = strtok(NULL, space);
            to_be_converted_arg = argument;
            int i = 0;
            while (argument != NULL) {
                argument = strtok(NULL, space);
                i++;
            }
            if (i == 0 || i > 1) {
                printf("Wrong number of args (given: %d, required: 1) for command '%s'\n", i, command);
            }
            else {
                // change arg to int
                int int_arg = atoi(to_be_converted_arg);

                // cancel job
                int cancel_check = job_cancel(int_arg);

                // error in cancel
                if (cancel_check == -1) {
                    printf("Error: cancel\n");
                }
            }
        }
        else if (!strcmp(command, "expunge")) { //----------------------------------------
            // error checker for number of args (must be 1)
            argument = strtok(NULL, space);
            to_be_converted_arg = argument;
            int i = 0;
            while (argument != NULL) {
                argument = strtok(NULL, space);
                i++;
            }
            if (i == 0 || i > 1) {
                printf("Wrong number of args (given: %d, required: 1) for command '%s'\n", i, command);
            }
            else {
                // change arg to int
                int int_arg = atoi(to_be_converted_arg);

                // expunge job
                int expunge_check = job_expunge(int_arg);

                // error in expunge
                if (expunge_check == -1) {
                    printf("Error: expunge\n");
                }
            }
        }
        else {
            // unrecognized commands in terminal
            printf("Unrecognized command: %s\n", command_buf);
        }
    }

    // only reaches here upon failure
    exit(EXIT_FAILURE);
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
