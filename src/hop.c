/**
 * @file hop.c
 * @brief Implementation of the `hop` built-in.
 */

#include "shellPrompt.h"
#include "token.h"
#include <sys/stat.h>
#include <paths.h>
#include "hop.h"

char previous_dir[PATH_MAX] = "";
int has_previous_dir = 0;

/**
 * @brief Check whether a path exists and is a directory.
 * @param path Path to test.
 * @return Non-zero if it is a directory.
 */
int directory_exists(const char *path)
{
    struct stat statbuf;
    return (stat(path, &statbuf) == 0 && S_ISDIR(statbuf.st_mode));
}

/**
 * @brief chdir() to a path, remembering the previous directory for `hop -`.
 * @param path Target directory.
 * @return 1 on success, 0 on failure.
 */
int change_directory(const char *path)
{
    if (chdir(path) == 0)
    {
        strcpy(previous_dir, current_dir);
        has_previous_dir = 1;

        update_current_dir();
        return 1;
    }
    return 0;
}

/**
 * @brief Perform a single hop step: ~, ., .., - or an explicit path.
 * @param cmd Hop target, or NULL for the shell home.
 */
void handle_single_hop_command(const char *cmd)
{
    if (cmd == NULL || strcmp(cmd, "~") == 0)
    {
        if (shell_home[0] != '\0' && change_directory(shell_home))
        {
            return;
        }
        else
        {
            printf("No such directory!\n");
        }
    }
    else if (strcmp(cmd, ".") == 0)
    {
        return;
    }
    else if (strcmp(cmd, "..") == 0)
    {
        if (change_directory("..") == 0)
        {
            printf("No such directory!\n");
        }
    }
    else if (strcmp(cmd, "-") == 0)
    {
        if (has_previous_dir)
        {
            char temp_dir[PATH_MAX];
            strcpy(temp_dir, current_dir);

            if (chdir(previous_dir) == 0)
            {
                strcpy(previous_dir, temp_dir);
                previous_dir[PATH_MAX - 1] = '\0';
                update_current_dir();
            }
            else
            {
                printf("No such directory!\n");
            }
        }
        else
        {
            printf("No such directory!\n");
        }
    }
    else
    {
        if (directory_exists(cmd))
        {
            if (change_directory(cmd) == 0)
            {
                printf("No such directory!\n");
            }
        }
        else
        {
            printf("No such directory!\n");
        }
    }
}

/**
 * @brief Process one hop argument, expanding chained shortcuts such as "..~".
 * @param arg Argument text, or NULL for the shell home.
 */
void handle_hop_argument(const char *arg)
{
    if (arg == NULL)
    {
        handle_single_hop_command(NULL);
        return;
    }

    char temp_arg[PATH_MAX];
    strncpy(temp_arg, arg, sizeof(temp_arg) - 1);
    temp_arg[sizeof(temp_arg) - 1] = '\0';

    int i = 0;
    int len = strlen(temp_arg);

    while (i < len)
    {
        if (i + 1 < len && temp_arg[i] == '.' && temp_arg[i + 1] == '.')
        {
            handle_single_hop_command("..");
            i += 2;
        }

        else if (temp_arg[i] == '~')
        {
            handle_single_hop_command("~");
            i += 1;
        }

        else if (temp_arg[i] == '.')
        {
            handle_single_hop_command(".");
            i += 1;
        }

        else if (temp_arg[i] == '-')
        {
            handle_single_hop_command("-");
            i += 1;
        }

        else
        {
            handle_single_hop_command(temp_arg + i);
            break;
        }
    }
}

/**
 * @brief Entry point for `hop [target...]`; with no target, go to the shell home.
 * @param cmd Parsed command.
 */
void execute_hop_command(Command *cmd)
{
    if (cmd->argc == 1)
    {
        handle_hop_argument(NULL);
    }
    else
    {
        for (int i = 1; i < cmd->argc; i++)
        {
            handle_hop_argument(cmd->argv[i]);
        }
    }
}
