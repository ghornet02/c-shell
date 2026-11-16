/**
 * @file log.c
 * @brief Implementation of the `log` built-in.
 *
 * Keeps the last LOG_CAP commands in a ring buffer persisted to
 * ~/.cshell_history. Supports `log`, `log purge` and `log execute <n>`.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "token.h"
#include "shellPrompt.h"

#define LOG_CAP 15

static char *hist[LOG_CAP];
static int count = 0;
static int start = 0;
static int loaded = 0;
static char hist_path[PATH_MAX];

/**
 * @brief Reset the in-memory history buffer.
 */
void log_init(void)
{

  for (int i = 0; i < LOG_CAP; i++)
  {
    hist[i] = NULL;
  }
  count = 0;
  start = 0;
  loaded = 0;
}

/**
 * @brief Free all history entries.
 */
void log_shutdown(void)
{

  for (int i = 0; i < count; i++)
  {
    int idx = (start + i) % LOG_CAP;
    if (hist[idx])
    {
      free(hist[idx]);
      hist[idx] = NULL;
    }
  }
  count = 0;
  start = 0;
  loaded = 0;
}

/**
 * @brief Build the path of the history file under $HOME.
 */
static void compute_hist_path(void)
{
  const char *home = getenv("HOME");
  if (home && strlen(home) + 20 < sizeof(hist_path))
  {
    snprintf(hist_path, sizeof(hist_path), "%s/%s", home, ".cshell_history");
  }
  else
  {
    snprintf(hist_path, sizeof(hist_path), "%s", ".cshell_history");
  }
}

/**
 * @brief Discard every entry in the history buffer.
 */
static void hist_free_all(void)
{
  for (int i = 0; i < count; i++)
  {
    int idx = (start + i) % LOG_CAP;
    free(hist[idx]);
    hist[idx] = NULL;
  }
  count = 0;
  start = 0;
}

/**
 * @brief Load the history file into the ring buffer (only once per session).
 */
static void load_history(void)
{
  if (loaded)
    return;
  compute_hist_path();
  FILE *f = fopen(hist_path, "r");
  if (!f)
  {
    loaded = 1;
    return;
  }
  char *line = NULL;
  size_t n = 0;
  ssize_t m;
  while ((m = getline(&line, &n, f)) != -1)
  {
    if (m > 0 && line[m - 1] == '\n')
      line[m - 1] = '\0';
    if (count < LOG_CAP)
    {
      int idx = (start + count) % LOG_CAP;
      hist[idx] = strdup(line);
      count++;
    }
    else
    {
      free(hist[start]);
      hist[start] = strdup(line);
      start = (start + 1) % LOG_CAP;
    }
  }
  free(line);
  fclose(f);
  loaded = 1;
}

/**
 * @brief Write the ring buffer back to the history file.
 */
static void save_history(void)
{
  if (!loaded)
    return;
  FILE *f = fopen(hist_path, "w");
  if (!f)
    return;
  for (int i = 0; i < count; i++)
  {
    int idx = (start + i) % LOG_CAP;
    fprintf(f, "%s\n", hist[idx]);
  }
  fclose(f);
}

/**
 * @brief Check whether a line equals the most recent history entry.
 * @return Non-zero if it matches.
 */
static int hist_last_equals(const char *line)
{
  if (count == 0)
    return 0;
  int last = (start + count - 1) % LOG_CAP;
  return strcmp(hist[last], line) == 0;
}

/**
 * @brief Append a line to history, evicting the oldest entry when full, then persist.
 */
static void hist_push_and_save(const char *line)
{
  if (count < LOG_CAP)
  {
    int idx = (start + count) % LOG_CAP;
    hist[idx] = strdup(line);
    count++;
  }
  else
  {
    free(hist[start]);
    hist[start] = strdup(line);
    start = (start + 1) % LOG_CAP;
  }
  save_history();
}

/**
 * @brief Print the history from oldest to newest.
 */
static void hist_print_oldest_to_newest(void)
{
  load_history();
  for (int i = 0; i < count; i++)
  {
    int idx = (start + i) % LOG_CAP;
    printf("%s\n", hist[idx]);
  }
}

/**
 * @brief Convert a 1-based newest-first index into a ring-buffer slot.
 * @return Slot index, or -1 if out of range.
 */
static int hist_index_newest_to_oldest(int one_indexed)
{
  if (one_indexed <= 0 || one_indexed > count)
    return -1;
  int newest_pos = (start + count - 1) % LOG_CAP;
  int pos = newest_pos - (one_indexed - 1);
  while (pos < 0)
    pos += LOG_CAP;
  return pos % LOG_CAP;
}

/**
 * @brief Check whether any command in the job list is `log` (such lines are not recorded).
 */
static int jobs_contains_log(const JobList *jobs)
{
  for (int p = 0; p < jobs->count; p++)
  {
    for (int c = 0; c < jobs->pipelines[p].cmd_count; c++)
    {
      const Command *cmd = &jobs->pipelines[p].cmds[c];
      if (cmd->argc > 0 && strcmp(cmd->argv[0], "log") == 0)
      {
        return 1;
      }
    }
  }
  return 0;
}

/**
 * @brief Record a command line unless it is empty, repeats the last entry or invokes `log`.
 * @param full_cmdline Raw input line.
 * @param jobs Parsed form of the line.
 */
void log_maybe_add(const char *full_cmdline, const JobList *jobs)
{
  load_history();
  if (!full_cmdline || *full_cmdline == '\0')
    return;
  if (jobs_contains_log(jobs))
    return;
  if (hist_last_equals(full_cmdline))
    return;
  hist_push_and_save(full_cmdline);
}

/**
 * @brief Lex, parse and execute a stored history line.
 * @param line Command line to replay.
 */
static void replay_command(const char *line)
{
  if (!line || !*line)
    return;
  Token tokens[1024];
  char buf[1024];
  strncpy(buf, line, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';
  int tc = lexer(buf, tokens);
  if (tc < 0)
    return;
  JobList jobs = (JobList){0};
  if (parse_tokens(tokens, tc, &jobs) < 0)
    return;
  execute_jobs(&jobs);

  for (int i = 0; i < jobs.count; i++)
  {
    for (int j = 0; j < jobs.pipelines[i].cmd_count; j++)
    {
      for (int k = 0; k < jobs.pipelines[i].cmds[j].argc; k++)
      {
        if (jobs.pipelines[i].cmds[j].argv[k])
        {
          free(jobs.pipelines[i].cmds[j].argv[k]);
        }
      }
      if (jobs.pipelines[i].cmds[j].input_file)
        free(jobs.pipelines[i].cmds[j].input_file);
      if (jobs.pipelines[i].cmds[j].output_file)
        free(jobs.pipelines[i].cmds[j].output_file);
    }
  }
}

/**
 * @brief Entry point for `log`, `log purge` and `log execute <n>`.
 * @param cmd Parsed command.
 */
void execute_log_command(Command *cmd)
{
  load_history();
  if (cmd->argc == 1)
  {
    hist_print_oldest_to_newest();
    return;
  }
  if (cmd->argc == 2 && strcmp(cmd->argv[1], "purge") == 0)
  {
    hist_free_all();
    save_history();
    return;
  }

  if (cmd->argc == 3 && strcmp(cmd->argv[1], "execute") == 0)
  {
    char *end = NULL;
    long v = strtol(cmd->argv[2], &end, 10);

    if (end == cmd->argv[2] || *end != '\0' || v <= 0)
      return;

    int pos = hist_index_newest_to_oldest((int)v);

    if (pos < 0)
      return;

    const char *line = hist[pos];
    replay_command(line);
    return;
  }

}
