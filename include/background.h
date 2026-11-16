/**
 * @file background.h
 * @brief Background job table and completion reporting.
 */

#ifndef BACKGROUND_H
#define BACKGROUND_H

#include <sys/types.h>
#include <string.h>

#define MAX_BACKGROUND_JOBS 64

/**
 * @brief A job launched with '&' that the shell is still tracking.
 */
typedef struct
{
    pid_t pid;
    int job_id;
    char command_name[256];
    int active;
} BackgroundJob;

extern BackgroundJob background_jobs[MAX_BACKGROUND_JOBS];
extern int next_job_id;
extern int num_background_jobs;

void cleanup_background_jobs(void);
void check_background_jobs(void);
void add_background_job(pid_t pid, const char *command_name);
void init_background_jobs(void);

#endif
