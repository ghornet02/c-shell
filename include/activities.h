/**
 * @file activities.h
 * @brief Tracking of processes spawned by the shell for the `activities` built-in.
 */

#ifndef ACTIVITIES_H
#define ACTIVITIES_H

#include "token.h"
#include <sys/types.h>

#define MAX_ACTIVITIES 64

typedef enum {
    PROCESS_RUNNING,
    PROCESS_STOPPED
} ProcessState;

/**
 * @brief Bookkeeping record for a process started by the shell.
 */
typedef struct {
    pid_t pid;
    char command_name[256];
    ProcessState state;
    int active;
    int job_number;
} ProcessInfo;

extern ProcessInfo process_list[MAX_ACTIVITIES];
extern int num_processes;

void init_activities(void);
void add_process_activity(pid_t pid, const char* command_name, int job_number);
void update_process_state(pid_t pid, ProcessState new_state);
void remove_terminated_processes(void);
void execute_activities_command(Command* cmd);
ProcessInfo* find_process_by_pid(pid_t pid);
ProcessInfo* find_process_by_job_number(int job_number);

#endif
