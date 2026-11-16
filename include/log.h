/**
 * @file log.h
 * @brief The `log` built-in: persistent command history with replay support.
 */

#ifndef LOG_H
#define LOG_H
#include "token.h"

void log_init(void);
void log_shutdown(void);
void log_maybe_add(const char *full_cmdline, const JobList *jobs);
void execute_log_command(Command *cmd);

#endif
