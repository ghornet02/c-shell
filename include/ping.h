/**
 * @file ping.h
 * @brief The `ping` built-in: send a signal to a process by PID.
 */

#ifndef PING_H
#define PING_H

#include "token.h"
#include <sys/types.h>

void execute_ping_command(Command *cmd);
int is_valid_number(const char *str);
int process_exists(pid_t pid);

#endif
