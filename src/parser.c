/**
 * @file parser.c
 * @brief Recursive-descent parser that builds a JobList from tokens.
 *
 * Grammar:
 *   shell_cmd -> cmd_group ((';' | '&') cmd_group)* ['&' | ';']
 *   cmd_group -> atomic ('|' atomic)*
 *   atomic    -> name (name | input | output)*
 *   input     -> '<' name
 *   output    -> ('>' | '>>') name
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "token.h"
#include "shellPrompt.h"

static Token *T = NULL;
static int N = 0;

static int pos = 0;

/**
 * @brief Check whether the current token has the given type.
 */
static int at(tokenType k) { return pos < N && T[pos].type == k; }

/**
 * @brief Consume the current token if it has the given type.
 * @return 1 if a token was consumed, 0 otherwise.
 */
static int accept(tokenType k)
{
    if (at(k))
    {
        pos++;
        return 1;
    }
    return 0;
}

/**
 * @brief Consume a word token, copying its text into shcmd.
 * @return 1 on success, 0 if the current token is not a word.
 */
static int name(char *shcmd)
{
    if (at(TOKEN_name))
    {

        strcpy(shcmd, T[pos].Cfg_word);
        pos++;

        return 1;
    }
    return 0;
}

/**
 * @brief Parse an input redirection (< file).
 * @param out Receives the file name.
 * @return 1 if parsed, 0 otherwise.
 */
static int input(char *out)
{
    if (accept(TOKEN_input))
    {
        if (!name(out))
        {
            fprintf(stderr, "Expected filename after <\n");
            return 0;
        }
        return 1;
    }
    return 0;
}

/**
 * @brief Parse an output redirection (> file or >> file).
 * @param out Receives the file name.
 * @param append Set to 1 for >>, 0 for >.
 * @return 1 if parsed, 0 otherwise.
 */
static int output(char *out, int *append)
{

    if (at(TOKEN_output))
    {
        *append = 0;
        accept(TOKEN_output);
    }

    else if (at(TOKEN_output_append))
    {
        *append = 1;
        accept(TOKEN_output_append);
    }

    else
    {
        return 0;
    }

    if (!name(out))
    {
        fprintf(stderr, "Expected filename after > or >>\n");
        return 0;
    }

    return 1;
}

/**
 * @brief Parse a simple command and its redirections into an argv array.
 * @param argv Output argument vector (heap-allocated strings, NULL-terminated).
 * @return Number of arguments, or 0 if no command was found.
 */
static int atomic(char **argv)
{
    int argc = 0;
    int append = 0;
    char buffer[1024];

    if (!name(buffer))
        return 0;

    argv[argc++] = strdup(buffer);

    while (1)
    {
        if (name(buffer))
        {
            argv[argc++] = strdup(buffer);
        }

        else if (input(buffer))
        {
            argv[argc++] = strdup("<");
            argv[argc++] = strdup(buffer);
        }

        else if (output(buffer, &append))
        {
            if (append)
            {
                argv[argc++] = strdup(">>");
            }

            else
            {
                argv[argc++] = strdup(">");
            }
            argv[argc++] = strdup(buffer);
        }
        else
        {
            break;
        }
    }

    argv[argc] = NULL;
    return argc;
}

/**
 * @brief Parse a pipeline of one or more commands joined by '|'.
 * @return 1 on success, 0 on a syntax error.
 */
static int cmd_group(Pipeline *pipeline)
{

    pipeline->cmd_count = 0;

    int argc = atomic(pipeline->cmds[pipeline->cmd_count].argv);

    if (!argc)
        return 0;

    pipeline->cmds[pipeline->cmd_count].argc = argc;
    pipeline->cmd_count++;

    while (accept(TOKEN_pipeline))
    {
        argc = atomic(pipeline->cmds[pipeline->cmd_count].argv);

        if (!argc)
            return 0;

        pipeline->cmds[pipeline->cmd_count].argc = argc;
        pipeline->cmd_count++;
    }
    return 1;
}

/**
 * @brief Parse a full command line of pipelines separated by ';' or '&'.
 *
 * Pipelines followed by '&' are launched in the background immediately and
 * removed from the job list.
 * @return 1 on success, 0 on a syntax error.
 */
static int shell_cmd(JobList *jobs)
{
    if (!cmd_group(&jobs->pipelines[jobs->count]))
        return 0;
    jobs->count++;

    while (at(TOKEN_semicolon) || at(TOKEN_ampersand))
    {

        int is_background = accept(TOKEN_ampersand);
        accept(TOKEN_semicolon);

        if (is_background)
        {

            JobList bg_jobs = {0};
            bg_jobs.pipelines[0] = jobs->pipelines[jobs->count - 1];
            bg_jobs.count = 1;
            bg_jobs.background = 1;

            execute_jobs(&bg_jobs);

            jobs->count--;
        }

        if (at(TOKEN_name) || at(TOKEN_input) || at(TOKEN_output))
        {
            if (!cmd_group(&jobs->pipelines[jobs->count]))
                return 0;
            jobs->count++;
        }
        else
        {
            break;
        }
    }

    return 1;
}

/**
 * @brief Parse a token array into a job list.
 * @param tokens Tokens produced by lexer().
 * @param token_count Number of tokens.
 * @param jobs Output job list.
 * @return 0 on success, -1 on invalid syntax.
 */
int parse_tokens(Token *tokens, int token_count, JobList *jobs)
{
    T = tokens;
    N = token_count;
    pos = 0;

    jobs->count = 0;
    jobs->background = 0;

    if (!shell_cmd(jobs))
    {
        fprintf(stderr, "Invalid Syntax!\n");
        return -1;
    }

    if (pos < token_count)
    {
        fprintf(stderr, "Invalid Syntax!\n");
        return -1;
    }

    return 0;
}
