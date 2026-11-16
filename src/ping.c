/**
 * @file ping.c
 * @brief Implementation of the `ping` built-in.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include "ping.h"

/**
 * @brief Check whether a string is an optionally negative decimal integer.
 * @return Non-zero if valid.
 */
int is_valid_number(const char *str)
{
    if (!str || *str == '\0')
    {
        return 0;
    }

    if (*str == '-')
    {
        str++;
        if (*str == '\0')
        {
            return 0;
        }
    }

    while (*str)
    {
        if (!isdigit(*str))
        {
            return 0;
        }
        str++;
    }

    return 1;
}

/**
 * @brief Check whether a process exists, using kill(pid, 0).
 * @return Non-zero if it exists.
 */
int process_exists(pid_t pid)
{

    if (kill(pid, 0) == 0)
    {
        return 1;
    }
    else if (errno == EPERM)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/**
 * @brief Entry point for `ping <pid> <signal>`; the signal number is taken modulo 32.
 * @param cmd Parsed command.
 */
void execute_ping_command(Command *cmd)
{
    if (cmd->argc != 3)
    {
        printf("Invalid syntax!\n");
        return;
    }

    if (!is_valid_number(cmd->argv[1]))
    {
        printf("Invalid syntax!\n");
        return;
    }

    pid_t pid = (pid_t)atoi(cmd->argv[1]);
    if (pid <= 0)
    {
        printf("Invalid syntax!\n");
        return;
    }

    if (!is_valid_number(cmd->argv[2]))
    {
        printf("Invalid syntax!\n");
        return;
    }

    int signal_number = atoi(cmd->argv[2]);
    int actual_signal = signal_number % 32;

    if (!process_exists(pid))
    {
        printf("No such process found\n");
        return;
    }

    if (kill(pid, actual_signal) == 0)
    {
        printf("Sent signal %d to process with pid %d\n", signal_number, pid);
    }
    else
    {
        printf("No such process found\n");
    }
}
