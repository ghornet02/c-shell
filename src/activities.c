/**
 * @file activities.c
 * @brief Process table behind the `activities` built-in.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include "activities.h"

ProcessInfo process_list[MAX_ACTIVITIES];
int num_processes = 0;

/**
 * @brief Clear the process table.
 */
void init_activities(void)
{
    for (int i = 0; i < MAX_ACTIVITIES; i++)
    {
        process_list[i].active = 0;
    }
    num_processes = 0;
}

/**
 * @brief Add a process to the table if it is not already tracked.
 * @param pid Process ID.
 * @param command_name Command name.
 * @param job_number Associated job number.
 */
void add_process_activity(pid_t pid, const char *command_name, int job_number)
{

    for (int i = 0; i < MAX_ACTIVITIES; i++)
    {
        if (process_list[i].active && process_list[i].pid == pid)
        {
            return;
        }
    }

    for (int i = 0; i < MAX_ACTIVITIES; i++)
    {
        if (!process_list[i].active)
        {
            process_list[i].pid = pid;
            strncpy(process_list[i].command_name, command_name, sizeof(process_list[i].command_name) - 1);
            process_list[i].command_name[sizeof(process_list[i].command_name) - 1] = '\0';
            process_list[i].state = PROCESS_RUNNING;
            process_list[i].active = 1;
            process_list[i].job_number = job_number;
            num_processes++;
            break;
        }
    }
}

/**
 * @brief Mark a tracked process as running or stopped.
 * @param pid Process ID.
 * @param new_state New state.
 */
void update_process_state(pid_t pid, ProcessState new_state)
{
    for (int i = 0; i < MAX_ACTIVITIES; i++)
    {
        if (process_list[i].active && process_list[i].pid == pid)
        {
            process_list[i].state = new_state;
            break;
        }
    }
}

/**
 * @brief Drop processes that have exited from the table.
 */
void remove_terminated_processes(void)
{
    for (int i = 0; i < MAX_ACTIVITIES; i++)
    {
        if (process_list[i].active)
        {
            int status;
            pid_t result = waitpid(process_list[i].pid, &status, WNOHANG);

            if (result == process_list[i].pid || (result == -1 && errno == ECHILD))
            {

                process_list[i].active = 0;
                num_processes--;
            }
        }
    }
}

/**
 * @brief Look up a tracked process by PID.
 * @return Pointer to the entry, or NULL.
 */
ProcessInfo *find_process_by_pid(pid_t pid)
{
    for (int i = 0; i < MAX_ACTIVITIES; i++)
    {
        if (process_list[i].active && process_list[i].pid == pid)
        {
            return &process_list[i];
        }
    }
    return NULL;
}

/**
 * @brief Look up a tracked process by job number.
 * @return Pointer to the entry, or NULL.
 */
ProcessInfo *find_process_by_job_number(int job_number)
{
    for (int i = 0; i < MAX_ACTIVITIES; i++)
    {
        if (process_list[i].active && process_list[i].job_number == job_number)
        {
            return &process_list[i];
        }
    }
    return NULL;
}

/**
 * @brief qsort comparator ordering processes by command name.
 */
int compare_command_names(const void *a, const void *b)
{
    const ProcessInfo *proc_a = (const ProcessInfo *)a;
    const ProcessInfo *proc_b = (const ProcessInfo *)b;
    return strcmp(proc_a->command_name, proc_b->command_name);
}

/**
 * @brief Entry point for `activities`: list live processes as [pid] : name - state.
 * @param cmd Parsed command.
 */
void execute_activities_command(Command *cmd)
{
    if (cmd->argc != 1)
    {
        fprintf(stderr, "activities: Invalid Syntax!\n");
        return;
    }

    remove_terminated_processes();

    if (num_processes == 0)
    {
        return;
    }

    ProcessInfo active_processes[MAX_ACTIVITIES];
    int active_count = 0;

    for (int i = 0; i < MAX_ACTIVITIES; i++)
    {
        if (process_list[i].active)
        {
            active_processes[active_count++] = process_list[i];
        }
    }

    qsort(active_processes, active_count, sizeof(ProcessInfo), compare_command_names);

    for (int i = 0; i < active_count; i++)
    {
        const char *state_str = (active_processes[i].state == PROCESS_RUNNING) ? "Running" : "Stopped";
        printf("[%d] : %s - %s\n",
               active_processes[i].pid,
               active_processes[i].command_name,
               state_str);
    }
}
