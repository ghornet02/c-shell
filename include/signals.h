/**
 * @file signals.h
 * @brief Ctrl-C / Ctrl-Z handling and the `fg` / `bg` job-control built-ins.
 */

#ifndef SIGNALS_H
#define SIGNALS_H

#include <sys/types.h>
#include "token.h"

extern pid_t current_foreground_pid;
extern pid_t current_foreground_pgid;

void sigint_handler(int sig);
void sigtstp_handler(int sig);

void setup_signal_handlers(void);

void set_foreground_process(pid_t pid, pid_t pgid);
void clear_foreground_process(void);

void execute_fg_command(Command *cmd);
void execute_bg_command(Command *cmd);

#endif
