/**
 * @file execute.c
 * @brief Execution of built-ins, external commands and pipelines in the foreground or background.
 */

#include "shellPrompt.h"
#include "token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <spawn.h>
#include <fcntl.h>
#include <unistd.h>
#include "hop.h"
#include "reveal.h"
#include <sys/wait.h>
#include "log.h"
#include "redirection.h"
#include "activities.h"
#include "ping.h"
#include "signals.h"
#include "background.h"

extern char **environ;

static void execute_pipeline_commands(Pipeline *pipeline);
static void setup_child_pipes_and_redirection(int cmd_index, int num_cmds, int pipes[][2], RedirectionInfo *redir);
static void execute_pipeline_commands_background(Pipeline *pipeline);

/**
 * @brief Check whether a command name is a shell built-in.
 * @param cmd Command name.
 * @return Non-zero if built-in.
 */
int is_builtin_command(const char *cmd)
{
    return (strcmp(cmd, "hop") == 0 || strcmp(cmd, "EXIT") == 0 || strcmp(cmd, "reveal") == 0 ||
            strcmp(cmd, "log") == 0 || strcmp(cmd, "activities") == 0 || strcmp(cmd, "ping") == 0 ||
            strcmp(cmd, "fg") == 0 || strcmp(cmd, "bg") == 0);
}

/**
 * @brief Dispatch a built-in command to its handler.
 * @param cmd Parsed command.
 * @return 1 if a built-in ran, 0 otherwise.
 */
int execute_builtin(Command *cmd)
{
    if (strcmp(cmd->argv[0], "hop") == 0)
    {
        execute_hop_command(cmd);
        return 1;
    }
    else if (strcmp(cmd->argv[0], "reveal") == 0)
    {
        execute_reveal_command(cmd);
        return 1;
    }
    else if (strcmp(cmd->argv[0], "log") == 0)
    {
        execute_log_command(cmd);
        return 1;
    }
    else if (strcmp(cmd->argv[0], "activities") == 0)
    {
        execute_activities_command(cmd);
        return 1;
    }
    else if (strcmp(cmd->argv[0], "ping") == 0)
    {
        execute_ping_command(cmd);
        return 1;
    }
    else if (strcmp(cmd->argv[0], "fg") == 0)
    {
        execute_fg_command(cmd);
        return 1;
    }
    else if (strcmp(cmd->argv[0], "bg") == 0)
    {
        execute_bg_command(cmd);
        return 1;
    }
    else if (strcmp(cmd->argv[0], "EXIT") == 0)
    {
        exit(0);
    }
    return 0;
}

/**
 * @brief Run a pipeline in the foreground and wait for it to finish or stop.
 *
 * Single built-ins run in the shell process with temporary redirection;
 * everything else is forked and exec'd.
 * @param pipeline Pipeline to run.
 */
void execute_pipeline(Pipeline *pipeline)
{
    if (pipeline->cmd_count == 1)
    {
        Command *cmd = &pipeline->cmds[0];

        RedirectionInfo redir;
        if (parse_redirection(cmd, &redir) < 0)
        {
            free_redirection_info(&redir);
            return;
        }

        if (cmd->argc > 0 && is_builtin_command(cmd->argv[0]))
        {

            if (redir.has_input_redirect || redir.has_output_redirect)
            {
                if (setup_redirection(&redir) == -1)
                {
                    free_redirection_info(&redir);
                    return;
                }
            }

            execute_builtin(cmd);

            if (redir.has_input_redirect || redir.has_output_redirect)
            {
                cleanup_redirection(&redir);
            }
        }
        else
        {

            pid_t pid = fork();

            if (pid == 0)
            {

                if (redir.has_input_redirect)
                {
                    int input_fd = open(redir.input_file, O_RDONLY);
                    if (input_fd == -1)
                    {
                        printf("No such file or directory\n");
                        exit(1);
                    }

                    if (dup2(input_fd, STDIN_FILENO) == -1)
                    {
                        fprintf(stderr,"dup2 input");
                        close(input_fd);
                        exit(1);
                    }
                    close(input_fd);
                }

                if (redir.has_output_redirect)
                {
                    int flags = O_WRONLY | O_CREAT;
                    if (redir.output_append)
                    {
                        flags |= O_APPEND;
                    }
                    else
                    {
                        flags |= O_TRUNC;
                    }

                    int output_fd = open(redir.output_file, flags, 0644);
                    if (output_fd == -1)
                    {
                        printf("Unable to create file for writing\n");
                        exit(1);
                    }

                    if (dup2(output_fd, STDOUT_FILENO) == -1)
                    {
                        fprintf(stderr,"dup2 output");
                        close(output_fd);
                        exit(1);
                    }
                    close(output_fd);
                }

                char *argv_exec[MAX_ARG];
                for (int i = 0; i < cmd->argc; i++)
                {
                    argv_exec[i] = cmd->argv[i];
                }
                argv_exec[cmd->argc] = NULL;

                execvp(argv_exec[0], argv_exec);

                printf("Command not found!\n");
                exit(127);
            }
            else if (pid > 0)
            {

                set_foreground_process(pid, pid);
                add_process_activity(pid, cmd->argv[0], next_job_id);

                int status;
                if (waitpid(pid, &status, WUNTRACED) < 0)
                {
                    fprintf(stderr,"waitpid");
                }

                clear_foreground_process();
            }
            else
            {

                fprintf(stderr,"fork");
                free_redirection_info(&redir);
                return;
            }
        }

        free_redirection_info(&redir);
    }
    else
    {

        execute_pipeline_commands(pipeline);
    }
}

/**
 * @brief Run every pipeline in a job list, in the foreground or background.
 * @param jobs Parsed job list.
 */
void execute_jobs(JobList *jobs)
{
    if (jobs->background)
    {

        for (int i = 0; i < jobs->count; i++)
        {
            execute_pipeline_background(&jobs->pipelines[i]);
        }
    }
    else
    {

        for (int i = 0; i < jobs->count; i++)
        {
            execute_pipeline(&jobs->pipelines[i]);
        }
    }
}

/**
 * @brief Launch a pipeline in the background and register it as a job.
 *
 * The child's stdin is redirected from /dev/null.
 * @param pipeline Pipeline to launch.
 */
void execute_pipeline_background(Pipeline *pipeline)
{
    if (pipeline->cmd_count == 1)
    {

        Command *cmd = &pipeline->cmds[0];

        RedirectionInfo redir;
        if (parse_redirection(cmd, &redir) < 0)
        {
            free_redirection_info(&redir);
            return;
        }

        if (cmd->argc > 0 && is_builtin_command(cmd->argv[0]))
        {
            printf("Background built-in commands not supported\n");
            free_redirection_info(&redir);
            return;
        }

        pid_t pid = fork();

        if (pid == 0)
        {

            int null_fd = open("/dev/null", O_RDONLY);
            if (null_fd >= 0)
            {
                dup2(null_fd, STDIN_FILENO);
                close(null_fd);
            }

            if (redir.has_output_redirect)
            {
                int flags = O_WRONLY | O_CREAT;
                if (redir.output_append)
                {
                    flags |= O_APPEND;
                }
                else
                {
                    flags |= O_TRUNC;
                }

                int output_fd = open(redir.output_file, flags, 0644);
                if (output_fd >= 0)
                {
                    dup2(output_fd, STDOUT_FILENO);
                    close(output_fd);
                }
            }

            if (redir.has_input_redirect)
            {
                int input_fd = open(redir.input_file, O_RDONLY);
                if (input_fd >= 0)
                {
                    dup2(input_fd, STDIN_FILENO);
                    close(input_fd);
                }
            }

            char *argv_exec[MAX_ARG];
            for (int i = 0; i < cmd->argc; i++)
            {
                argv_exec[i] = cmd->argv[i];
            }
            argv_exec[cmd->argc] = NULL;

            execvp(argv_exec[0], argv_exec);

            printf("Command not found: %s\n", argv_exec[0]);
            exit(127);
        }
        else if (pid > 0)
        {

            add_background_job(pid, cmd->argv[0]);
            add_process_activity(pid, cmd->argv[0], next_job_id - 1);
        }
        else
        {

            fprintf(stderr,"fork");
        }

        free_redirection_info(&redir);
    }
    else
    {

        execute_pipeline_commands_background(pipeline);
    }
}

/**
 * @brief Fork one process per command, connect them with pipes and wait for all of them.
 * @param pipeline Multi-command pipeline.
 */
static void execute_pipeline_commands(Pipeline *pipeline)
{
    int num_cmds = pipeline->cmd_count;
    int pipes[num_cmds - 1][2];
    pid_t pids[num_cmds];

    for (int i = 0; i < num_cmds - 1; i++)
    {
        if (pipe(pipes[i]) == -1)
        {
            fprintf(stderr,"pipe");

            for (int j = 0; j < i; j++)
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            return;
        }
    }

    for (int i = 0; i < num_cmds; i++)
    {
        Command *cmd = &pipeline->cmds[i];

        RedirectionInfo redir;
        if (parse_redirection(cmd, &redir) < 0)
        {
            free_redirection_info(&redir);
            pids[i] = -1;
            continue;
        }

        pids[i] = fork();

        if (pids[i] == 0)
        {

            setup_child_pipes_and_redirection(i, num_cmds, pipes, &redir);

            char *argv_exec[MAX_ARG];
            for (int j = 0; j < cmd->argc; j++)
            {
                argv_exec[j] = cmd->argv[j];
            }
            argv_exec[cmd->argc] = NULL;

            if (cmd->argc > 0 && is_builtin_command(cmd->argv[0]))
            {

                execute_builtin(cmd);
                exit(0);
            }
            else
            {

                execvp(argv_exec[0], argv_exec);

                printf("Command not found!\n");
                exit(127);
            }
        }
        else if (pids[i] > 0)
        {

            if (i == 0)
            {
                set_foreground_process(pids[i], pids[i]);
                add_process_activity(pids[i], cmd->argv[0], next_job_id);
            }
        }
        else
        {
            fprintf(stderr,"fork");
        }

        free_redirection_info(&redir);
    }

    for (int i = 0; i < num_cmds - 1; i++)
    {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    for (int i = 0; i < num_cmds; i++)
    {
        if (pids[i] > 0)
        {
            int status;
            if (waitpid(pids[i], &status, WUNTRACED) < 0)
            {
                fprintf(stderr,"waitpid");
            }
        }
    }

    clear_foreground_process();
}

/**
 * @brief Run a multi-command pipeline inside a forked child so it does not block the shell.
 * @param pipeline Multi-command pipeline.
 */
static void execute_pipeline_commands_background(Pipeline *pipeline)
{

    pid_t pipeline_pid = fork();

    if (pipeline_pid == 0)
    {

        int null_fd = open("/dev/null", O_RDONLY);
        if (null_fd >= 0)
        {
            dup2(null_fd, STDIN_FILENO);
            close(null_fd);
        }

        execute_pipeline_commands(pipeline);
        exit(0);
    }
    else if (pipeline_pid > 0)
    {

        char pipeline_name[256];
        snprintf(pipeline_name, sizeof(pipeline_name), "pipeline_%d", pipeline_pid);
        add_background_job(pipeline_pid, pipeline_name);
        add_process_activity(pipeline_pid, pipeline_name, next_job_id - 1);
    }
    else
    {

        fprintf(stderr,"fork");
    }
}

/**
 * @brief In a pipeline child, wire stdin/stdout to the neighbouring pipes and apply file redirection.
 *
 * Input redirection applies only to the first command and output redirection
 * only to the last.
 * @param cmd_index Position of this command in the pipeline.
 * @param num_cmds Total commands in the pipeline.
 * @param pipes Pipe descriptor pairs.
 * @param redir Redirection info for this command.
 */
static void setup_child_pipes_and_redirection(int cmd_index, int num_cmds, int pipes[][2], RedirectionInfo *redir)
{

    if (cmd_index > 0)
    {
        if (dup2(pipes[cmd_index - 1][0], STDIN_FILENO) == -1)
        {
            fprintf(stderr,"dup2 stdin pipe");
            exit(1);
        }
    }

    if (cmd_index < num_cmds - 1)
    {
        if (dup2(pipes[cmd_index][1], STDOUT_FILENO) == -1)
        {
            fprintf(stderr,"dup2 stdout pipe");
            exit(1);
        }
    }

    for (int i = 0; i < num_cmds - 1; i++)
    {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    if (redir->has_input_redirect && cmd_index == 0)
    {
        int input_fd = open(redir->input_file, O_RDONLY);
        if (input_fd == -1)
        {
            printf("No such file or directory\n");
            exit(1);
        }
        if (dup2(input_fd, STDIN_FILENO) == -1)
        {
            fprintf(stderr,"dup2 input file");
            exit(1);
        }
        close(input_fd);
    }

    if (redir->has_output_redirect && cmd_index == num_cmds - 1)
    {
        int flags = O_WRONLY | O_CREAT;
        if (redir->output_append)
        {
            flags |= O_APPEND;
        }
        else
        {
            flags |= O_TRUNC;
        }
        int output_fd = open(redir->output_file, flags, 0644);
        if (output_fd == -1)
        {
            printf("Unable to create file for writing\n");
            exit(1);
        }
        if (dup2(output_fd, STDOUT_FILENO) == -1)
        {
            fprintf(stderr,"dup2 output file");
            exit(1);
        }
        close(output_fd);
    }
}
