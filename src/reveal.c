/**
 * @file reveal.c
 * @brief Implementation of the `reveal` built-in.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include "shellPrompt.h"
#include "hop.h"
#include "token.h"
#include "reveal.h"

/**
 * @brief qsort comparator for case-insensitive alphabetical ordering.
 */
int compare(const void *a, const void *b)
{
    const char *str1 = *(const char **)a;
    const char *str2 = *(const char **)b;

    return strcasecmp(str1, str2);
}

/**
 * @brief Print the entries of a directory in sorted order.
 * @param path Directory to list.
 * @param show_all Include hidden entries when non-zero.
 * @param line_by_line Print one entry per line when non-zero.
 */
void list_directory(const char *path, int show_all, int line_by_line)
{
    DIR *dir = opendir(path);
    if (!dir)
    {
        printf("No such directory!\n");
        return;
    }

    struct dirent *entry;
    char *entries[1024];
    int count = 0;

    while ((entry = readdir(dir)) != NULL && count < 1024)
    {
        const char *name = entry->d_name;
        if (!show_all && name[0] == '.')
            continue;
        entries[count++] = strdup(name);
    }

    closedir(dir);

    if (count == 0)
    {
        return;
    }

    qsort(entries, count, sizeof(char *), compare);

    if (line_by_line)
    {
        for (int i = 0; i < count; i++)
        {
            printf("%s\n", entries[i]);
            free(entries[i]);
        }
    }
    else
    {
        for (int i = 0; i < count; i++)
        {
            printf("%s", entries[i]);
            if (i < count - 1)
                printf(" ");
            free(entries[i]);
        }
        printf("\n");
    }
}

/**
 * @brief Resolve a reveal target (~, ., .., - or a path) and list it.
 * @param arg Target, or NULL for the current directory.
 * @param show_all Include hidden entries.
 * @param line_by_line One entry per line.
 */
void handle_reveal_argument(const char *arg, int show_all, int line_by_line)
{
    char resolved_path[PATH_MAX];

    if (arg == NULL)
    {

        if (directory_exists(current_dir))
        {
            list_directory(current_dir, show_all, line_by_line);
        }
        else
        {
            printf("No such directory!\n");
        }
        return;
    }

    if (strcmp(arg, "~") == 0)
    {

        if (directory_exists(shell_home))
        {
            list_directory(shell_home, show_all, line_by_line);
        }
        else
        {
            printf("No such directory!\n");
        }
        return;
    }

    if (strcmp(arg, ".") == 0)
    {
        if (directory_exists(current_dir))
        {
            list_directory(current_dir, show_all, line_by_line);
        }
        else
        {
            printf("No such directory!\n");
        }
        return;
    }

    if (strcmp(arg, "..") == 0)
    {

        strncpy(resolved_path, current_dir, PATH_MAX - 1);
        resolved_path[PATH_MAX - 1] = '\0';
        char *last_slash = strrchr(resolved_path, '/');
        if (last_slash != NULL && last_slash != resolved_path)
        {
            *last_slash = '\0';
        }
        else if (last_slash == resolved_path)
        {
            resolved_path[1] = '\0';
        }

        if (directory_exists(resolved_path))
        {
            list_directory(resolved_path, show_all, line_by_line);
        }
        else
        {
            printf("No such directory!\n");
        }
        return;

    }

    if (strcmp(arg, "-") == 0)
    {
        if (has_previous_dir && directory_exists(previous_dir))
        {
            list_directory(previous_dir, show_all, line_by_line);
        }
        else
        {
            printf("No such directory!\n");
        }
        return;
    }

    if (directory_exists(arg))
    {
        list_directory(arg, show_all, line_by_line);
    }
    else
    {
        printf("No such directory!\n");
    }
}

/**
 * @brief Entry point for `reveal [-al] [target]`.
 * @param cmd Parsed command.
 */
void execute_reveal_command(Command *cmd)
{
    int show_all = 0;
    int line_by_line = 0;
    int arg_start = 1;

    while (arg_start < cmd->argc && cmd->argv[arg_start][0] == '-' && strlen(cmd->argv[arg_start]) > 1)
    {
        const char *flag_str = cmd->argv[arg_start] + 1;

        for (int i = 0; flag_str[i] != '\0'; i++)
        {
            if (flag_str[i] == 'a')
            {
                show_all = 1;
            }
            else if (flag_str[i] == 'l')
            {
                line_by_line = 1;
            }
        }
        arg_start++;
    }

    int remaining_args = cmd->argc - arg_start;
    if (remaining_args > 1)
    {
        printf("reveal: Invalid Syntax!\n");
        return;
    }

    const char *target_arg = NULL;
    if (remaining_args == 1)
    {
        target_arg = cmd->argv[arg_start];
    }

    handle_reveal_argument(target_arg, show_all, line_by_line);
}
