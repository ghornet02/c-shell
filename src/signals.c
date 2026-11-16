/**
 * @file signals.c
 * @brief Signal handling and foreground/background job control.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include "signals.h"
#include "activities.h"
#include "background.h"

pid_t current_foreground_pid = 0;
pid_t current_foreground_pgid = 0;

/**
 * @brief Forward Ctrl-C to the foreground process group; the shell itself keeps running.
 */
void sigint_handler(int sig)
{
    if (current_foreground_pgid > 0)
    {

        kill(-current_foreground_pgid, SIGINT);
    }

    write(STDOUT_FILENO, "\n", 1);
}

/**
 * @brief Forward Ctrl-Z to the foreground process group and mark it as stopped.
 */
void sigtstp_handler(int sig)
{
    if (current_foreground_pgid > 0)
    {

        kill(-current_foreground_pgid, SIGTSTP);
        printf("Stopped [%d]\n", current_foreground_pid);

        for (int i = 0; i < MAX_BACKGROUND_JOBS; i++)
        {
            if (background_jobs[i].active && background_jobs[i].pid == current_foreground_pid)
            {
                printf("[%d] Stopped %s\n", background_jobs[i].job_id, background_jobs[i].command_name);
                break;
            }
        }

        update_process_state(current_foreground_pid, PROCESS_STOPPED);

        clear_foreground_process();

        tcsetpgrp(STDIN_FILENO, getpgrp());
    }
    write(STDOUT_FILENO, "\n", 1);
}

/**
 * @brief Install the SIGINT and SIGTSTP handlers.
 */
void setup_signal_handlers(void)
{
    struct sigaction sa_int, sa_tstp;

    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa_int, NULL);

    sa_tstp.sa_handler = sigtstp_handler;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa_tstp, NULL);
}

/**
 * @brief Record the process (group) currently running in the foreground.
 */
void set_foreground_process(pid_t pid, pid_t pgid)
{
    current_foreground_pid = pid;
    current_foreground_pgid = pgid;
}

/**
 * @brief Clear the foreground process record.
 */
void clear_foreground_process(void)
{
    current_foreground_pid = 0;
    current_foreground_pgid = 0;
}

/**
 * @brief Entry point for `fg [job]`: resume a job in the foreground and wait for it.
 *
 * With no argument, the most recent job is used.
 * @param cmd Parsed command.
 */
void execute_fg_command(Command *cmd)
{
    int job_number = -1;

    if (cmd->argc == 1)
    {

        int max_job_id = 0;
        for (int i = 0; i < MAX_BACKGROUND_JOBS; i++)
        {
            if (background_jobs[i].active && background_jobs[i].job_id > max_job_id)
            {
                max_job_id = background_jobs[i].job_id;
                job_number = background_jobs[i].job_id;
            }
        }

        if (job_number == -1)
        {
            printf("No such job\n");
            return;
        }
    }
    else if (cmd->argc == 2)
    {
        job_number = atoi(cmd->argv[1]);
        if (job_number <= 0)
        {
            printf("No such job\n");
            return;
        }
    }
    else
    {
        printf("fg: Invalid Syntax!\n");
        return;
    }

    BackgroundJob *job = NULL;
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++)
    {
        if (background_jobs[i].active && background_jobs[i].job_id == job_number)
        {
            job = &background_jobs[i];
            break;
        }
    }

    if (!job)
    {
        printf("No such job\n");
        return;
    }

    printf("%s\n", job->command_name);

    set_foreground_process(job->pid, job->pid);

    ProcessInfo *proc_info = find_process_by_pid(job->pid);
    if (proc_info && proc_info->state == PROCESS_STOPPED)
    {
        kill(job->pid, SIGCONT);
        update_process_state(job->pid, PROCESS_RUNNING);
    }

    int status;
    pid_t result = waitpid(job->pid, &status, WUNTRACED);

    if (result == job->pid)
    {
        if (WIFSTOPPED(status))
        {

            update_process_state(job->pid, PROCESS_STOPPED);
            printf("[%d] Stopped %s\n", job->job_id, job->command_name);
        }
        else
        {

            job->active = 0;
            num_background_jobs--;
            ProcessInfo *proc = find_process_by_pid(job->pid);
            if (proc)
            {
                proc->active = 0;
            }
        }
    }

    clear_foreground_process();
}

/**
 * @brief Entry point for `bg <job>`: resume a stopped job in the background.
 * @param cmd Parsed command.
 */
void execute_bg_command(Command *cmd)
{
    int job_number = -1;

    if (cmd->argc != 2)
    {
        printf("bg: Invalid Syntax!\n");
        return;
    }

    job_number = atoi(cmd->argv[1]);
    if (job_number <= 0)
    {
        printf("No such job\n");
        return;
    }

    BackgroundJob *job = NULL;
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++)
    {
        if (background_jobs[i].active && background_jobs[i].job_id == job_number)
        {
            job = &background_jobs[i];
            break;
        }
    }

    if (!job)
    {
        printf("No such job\n");
        return;
    }

    ProcessInfo *proc_info = find_process_by_pid(job->pid);
    if (proc_info && proc_info->state == PROCESS_RUNNING)
    {
        printf("Job already running\n");
        return;
    }

    if (kill(job->pid, SIGCONT) == 0)
    {
        update_process_state(job->pid, PROCESS_RUNNING);
        printf("[%d] %s &\n", job->job_id, job->command_name);
    }
    else
    {
        printf("No such job\n");
    }
}
