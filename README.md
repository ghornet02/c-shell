# C-Shell

A POSIX shell written from scratch in C. It has its own lexer and recursive-descent parser, pipelines, I/O redirection, background jobs, job control and a set of built-in commands.

## Features

- **Pipelines:** `cmd1 | cmd2 | cmd3`
- **Redirection:** `<`, `>` and `>>`, including inside pipelines
- **Command chaining:** sequential `;` and background `&`
- **Job control:** Ctrl-C and Ctrl-Z are forwarded to the foreground job; `fg` and `bg` resume jobs
- **Background job tracking:** the shell reports when each job exits normally or abnormally
- **Persistent history:** the last 15 commands are stored in `~/.cshell_history`
- **Custom prompt:** `<user@host:path>`, with the shell's start directory shown as `~`

## Built-in commands

| Command | Description |
|---|---|
| `hop [path ...]` | Change directory. Supports `~`, `.`, `..`, `-` and chained shortcuts. |
| `reveal [-a] [-l] [path]` | List a directory (`-a` shows hidden files, `-l` prints one entry per line). |
| `log` / `log purge` / `log execute <n>` | Show, clear or re-run command history. |
| `activities` | List processes started by the shell and their state. |
| `ping <pid> <signal>` | Send a signal to a process. |
| `fg [job]` / `bg <job>` | Bring a job to the foreground or resume it in the background. |
| `EXIT` | Exit the shell. Ctrl-D also exits and kills child processes. |

## Build and run

```sh
make
./cshell
```

Requires `gcc` and a POSIX system (Linux or macOS).

## Project structure

```
include/         Public headers for each module
src/cshell.c     Read-eval loop
src/lexer.c      Tokeniser
src/parser.c     Recursive-descent parser -> JobList
src/execute.c    Fork/exec, pipelines, background execution
src/redirection.c, src/signals.c, src/background.c, src/activities.c
src/hop.c, src/reveal.c, src/log.c, src/ping.c    Built-ins
```

## Grammar

```
shell_cmd -> cmd_group ((';' | '&') cmd_group)* ['&' | ';']
cmd_group -> atomic ('|' atomic)*
atomic    -> name (name | input | output)*
input     -> '<' name
output    -> ('>' | '>>') name
```
