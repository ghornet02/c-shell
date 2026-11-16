/**
 * @file cshell.c
 * @brief Entry point and read-eval loop of the shell.
 */

#include "shellPrompt.h"
#include "token.h"
#include "log.h"
#include "activities.h"
#include "signals.h"
#include "background.h"
#include <signal.h>

/**
 * @brief Handle Ctrl-D: kill all child processes and exit the shell.
 * @return Never returns.
 */
int handle_eof_condition(void)
{
    printf("logout\n");

    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++)
    {
        if (background_jobs[i].active)
        {
            kill(background_jobs[i].pid, SIGKILL);
        }
    }

    for (int i = 0; i < MAX_ACTIVITIES; i++)
    {
        if (process_list[i].active)
        {
            kill(process_list[i].pid, SIGKILL);
        }
    }

    cleanup_background_jobs();
    exit(0);
    return 0;
}

/**
 * @brief Initialise subsystems and run the read-lex-parse-execute loop until EOF.
 */
int main()
{
    init_shell_home();
    init_background_jobs();
    init_activities();
    setup_signal_handlers();
    log_init();

    while (1)
    {

        check_background_jobs();

        printShellPrompt();
        char input[1024];

        if (!fgets(input, sizeof(input), stdin))
        {
            if (feof(stdin))
            {
                handle_eof_condition();
            }
            else
            {
                perror("Could not read input");
                continue;
            }
        }

        input[strcspn(input, "\n")] = '\0';

        if (strlen(input) == 0)
        {
            continue;
        }

        Token tokens[1024];
        int token_count = lexer(input, tokens);

        if (token_count < 0)
        {
            perror("Lexer error\n");
            continue;
        }

        JobList jobs = {0};
        if (parse_tokens(tokens, token_count, &jobs) < 0)
        {
            continue;
        }

        log_maybe_add(input, &jobs);

        if (jobs.count > 0)
        {
            execute_jobs(&jobs);
        }

        for (int i = 0; i < jobs.count; i++)
        {
            for (int j = 0; j < jobs.pipelines[i].cmd_count; j++)
            {
                for (int k = 0; k < jobs.pipelines[i].cmds[j].argc; k++)
                {
                    if (jobs.pipelines[i].cmds[j].argv[k])
                    {
                        free(jobs.pipelines[i].cmds[j].argv[k]);
                        jobs.pipelines[i].cmds[j].argv[k] = NULL;
                    }
                }
            }
        }

    }

    cleanup_background_jobs();
    log_shutdown();
    return 0;
}
