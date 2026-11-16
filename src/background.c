/**
 * @file background.c
 * @brief Background job table management.
 */

#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "background.h"
#include "activities.h"

BackgroundJob background_jobs[MAX_BACKGROUND_JOBS];
int next_job_id = 1;
int num_background_jobs = 0;

/**
 * @brief Clear the job table and reset job numbering.
 */
void init_background_jobs(void)
{
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++)
    {
        background_jobs[i].active = 0;
    }
    next_job_id = 1;
    num_background_jobs = 0;
}

/**
 * @brief Register a newly launched background process.
 * @param pid Process ID.
 * @param command_name Name shown in job messages.
 */
void add_background_job(pid_t pid, const char *command_name)
{
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++)
    {
        if (!background_jobs[i].active)
        {
            background_jobs[i].pid = pid;
            background_jobs[i].job_id = next_job_id++;
            strncpy(background_jobs[i].command_name, command_name, sizeof(background_jobs[i].command_name) - 1);
            background_jobs[i].command_name[sizeof(background_jobs[i].command_name) - 1] = '\0';
            background_jobs[i].active = 1;
            num_background_jobs++;

            break;
        }
    }
}

/**
 * @brief Reap finished background jobs and report whether each exited normally.
 */
void check_background_jobs(void)
{
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++)
    {
        if (background_jobs[i].active)
        {
            int status;
            pid_t result = waitpid(background_jobs[i].pid, &status, WNOHANG);

            if (result == background_jobs[i].pid)
            {

                if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
                {
                    printf("%s with pid %d exited normally\n",
                           background_jobs[i].command_name, background_jobs[i].pid);
                }
                else
                {
                    printf("%s with pid %d exited abnormally\n",
                           background_jobs[i].command_name, background_jobs[i].pid);
                }

                background_jobs[i].active = 0;
                num_background_jobs--;

                ProcessInfo *proc = find_process_by_pid(background_jobs[i].pid);
                if (proc)
                {
                    proc->active = 0;
                }
            }
            else if (result == -1 && errno != ECHILD)
            {

                printf("%s with pid %d exited abnormally\n",
                       background_jobs[i].command_name, background_jobs[i].pid);
                background_jobs[i].active = 0;
                num_background_jobs--;

                ProcessInfo *proc = find_process_by_pid(background_jobs[i].pid);
                if (proc)
                {
                    proc->active = 0;
                }
            }
        }
    }
}

/**
 * @brief Block until every remaining background job has exited.
 */
void cleanup_background_jobs(void)
{

    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++)
    {
        if (background_jobs[i].active)
        {
            int status;
            waitpid(background_jobs[i].pid, &status, 0);
        }
    }
}
