/**
 * @file shellPrompt.c
 * @brief Prompt rendering and tracking of the shell's home and current directories.
 */

#include "shellPrompt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

char shell_home[PATH_MAX];
char current_dir[PATH_MAX];

/**
 * @brief Record the directory the shell was started in as its home (~).
 */
void init_shell_home()
{
    if (!getcwd(shell_home, sizeof(shell_home)))
    {
        perror("getcwd");
        exit(1);
    }

    if (!getcwd(current_dir, sizeof(current_dir)))
    {
        perror("getcwd");
        strncpy(current_dir, "?", sizeof(current_dir));
        current_dir[sizeof(current_dir) - 1] = '\0';
    }

}

/**
 * @brief Refresh the cached current working directory.
 */
void update_current_dir()
{
    if (!getcwd(current_dir, sizeof(current_dir)))
    {
        perror("getcwd");
        strncpy(current_dir, "?", sizeof(current_dir));
        current_dir[sizeof(current_dir) - 1] = '\0';
    }
}

/**
 * @brief Print the prompt as <user@host:path>, abbreviating the shell home as ~.
 */
void printShellPrompt()
{
    char hostname[256];
    char display_path[PATH_MAX];
    char *username = getenv("USER");

    update_current_dir();

    if (gethostname(hostname, sizeof(hostname)) != 0)
    {
        strncpy(hostname, "unknown", sizeof(hostname));
        hostname[sizeof(hostname) - 1] = '\0';
    }

    size_t home_len = strlen(shell_home);

    if (strcmp(current_dir, shell_home) == 0)
    {
        strncpy(display_path, "~", sizeof(display_path));
        display_path[sizeof(display_path) - 1] = '\0';
    }

    else if (home_len > 0 && strncmp(current_dir, shell_home, home_len) == 0 &&
             (current_dir[home_len] == '/' || current_dir[home_len] == '\0'))
    {
        if (current_dir[home_len] == '/')
        {
            snprintf(display_path, sizeof(display_path), "~%s", current_dir + home_len);
        }
        else
        {
            strncpy(display_path, "~", sizeof(display_path));
            display_path[sizeof(display_path) - 1] = '\0';
        }
    }

    else
    {

        strncpy(display_path, current_dir, sizeof(display_path));
        display_path[sizeof(display_path) - 1] = '\0';
    }

    printf("<%s@%s:%s> ", username ? username : "user", hostname, display_path);
    fflush(stdout);
}
