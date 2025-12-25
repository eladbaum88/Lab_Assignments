#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>  // for PATH_MAX
#include "LineParser.h"
#include <stdbool.h> 
#include <sys/types.h>  // pid_t, etc.
#include <sys/wait.h>   // waitpid
#include <fcntl.h>      // open, O_* flags
#include <sys/stat.h>   // mode_t for open(..., O_CREAT, mode)
#include <signal.h>   // for SIGSTOP, SIGCONT, SIGINT, kill()
#include <errno.h>    // for errno

#define MAX_LINE_LEN 2048
bool debug_on = false; // Global variable to control debug mode


void execute(cmdLine *pCmdLine) {
    // only at child case
    // "< file" read from file 
    if (pCmdLine->inputRedirect != NULL) {
        int fd_in = open(pCmdLine->inputRedirect, O_RDONLY);
        if (fd_in < 0) {
            perror("open input file failed");
            _exit(1); // child exits, parent shell stays alive
        }

        // Make this file become stdin (fd 0)
        if (dup2(fd_in, STDIN_FILENO) < 0) {
            perror("dup2 input failed");
            close(fd_in);
            _exit(1);
        }
        close(fd_in); // no longer need the original fd
    }

    // --- Output redirection: "> file" ---
    if (pCmdLine->outputRedirect != NULL) {
        int fd_out = open(pCmdLine->outputRedirect,
                          O_WRONLY | O_CREAT | O_TRUNC,
                          0644);
        if (fd_out < 0) {
            perror("open output file failed");
            _exit(1);
        }

        // Make this file become stdout (fd 1)
        if (dup2(fd_out, STDOUT_FILENO) < 0) {
            perror("dup2 output failed");
            close(fd_out);
            _exit(1);
        }
        close(fd_out);
    }

    int result = execvp(pCmdLine->arguments[0], pCmdLine->arguments);

    if (result == -1) {
        perror("exec failed");
   
        _exit(1);  // abnormal exit from the shell process
    }

    // If exec succeeds, this code is never reached
}

int handle_signal_command(cmdLine *cmd, int sig, const char *cmd_name) {
    // error checking
    if (cmd->argCount < 2 || cmd->arguments[1] == NULL) {
        fprintf(stderr, "usage: %s <pid>\n", cmd_name);
        return -1;
    }
    if (cmd->argCount > 2) {
        fprintf(stderr, "%s: too many arguments\n", cmd_name);
        return -1;
    }
    char *endptr = NULL;
    long pid_long = strtol(cmd->arguments[1], &endptr, 10); // convert to long

    // validate that the PID is a positive integer with no garbage
    if (cmd->arguments[1][0] == '\0' || *endptr != '\0' || pid_long <= 0) {
        fprintf(stderr, "%s: invalid pid: %s\n", cmd_name, cmd->arguments[1]);
        return -1;
    }

    pid_t pid = (pid_t)pid_long;

    if (kill(pid, sig) == -1) {
        perror("kill");
        return -1;
    }

    if (debug_on) {
        fprintf(stderr, "%s: sent signal %d to pid %d\n",
                cmd_name, sig, (int)pid);
    }

    return 0;
}

int handle_signals_builtins(cmdLine *cmd) {

    if (cmd == NULL || cmd->argCount == 0 || cmd->arguments[0] == NULL) {
        return 0;   // nothing to do
    }

    if (strcmp(cmd->arguments[0], "zzzz") == 0) {
        // pause a process with SIGSTOP
        handle_signal_command(cmd, SIGSTOP, "zzzz");
        return 1;   // handled as builtin
    }

    if (strcmp(cmd->arguments[0], "kuku") == 0) {
        // continue a stopped process with SIGCONT
        handle_signal_command(cmd, SIGCONT, "kuku");
        return 1;   // handled as builtin
    }

    if (strcmp(cmd->arguments[0], "blast") == 0) {
        // interrupt/terminate with SIGINT
        handle_signal_command(cmd, SIGINT, "blast");
        return 1;   // handled as builtin
    }

    return 0;       
}

// Reap any zombie child processes
void handle_zombies() { 
    int status;
    pid_t child;

    while ((child = waitpid(-1, &status, WNOHANG)) > 0) {
        if (debug_on) {
            fprintf(stderr, "Reaped child: %d\n", child);
        }
    }
}


int main(int argc, char *argv[]) {

    char input[MAX_LINE_LEN];
    char cwd[PATH_MAX];

    for (int i = 0; i < argc; ++ i) {

        const unsigned char *curr_arg = (const unsigned char *)argv[i];

        if (curr_arg[0] == '-' && curr_arg[1] == 'd' && curr_arg[2] == '\0') {
            debug_on = true;
        }
    }   
    while (1) {
        
        handle_zombies(); // reap any zombie children

        // show prompt: current working directory
        if (getcwd(cwd, PATH_MAX) == NULL) {
            perror("getcwd failed");
            // if we can't get cwd, just show a generic prompt
            printf("myshell> ");
        } else {
            printf("%s> ", cwd);
        }
        fflush(stdout);

        // read a line from stdin
        if (fgets(input, MAX_LINE_LEN, stdin) == NULL) {
            // EOF (Ctrl+D) or error → exit shell
            printf("\n");
            break;
        }

        // strip trailing newline
        if (input[strlen(input) - 1] == '\n') {
            input[strlen(input) - 1] = '\0';
        }

        // ignore empty lines
        if (strlen(input) == 0) {
            continue;
        }

        // parse the line into cmdLine
        cmdLine *cmd = parseCmdLines(input);
        if (cmd == NULL) {
            continue;
        }

        // check for "quit" shell command
        if (strcmp(cmd->arguments[0], "quit") == 0) {
            freeCmdLines(cmd);
            break;  // normal exit from shell loop
        }
        
        
        // cd implementation
        if (strcmp(cmd->arguments[0], "cd") == 0) {
            if (cmd->argCount < 2) {
                fprintf(stderr, "cd: missing argument\n");
            } else {
                if (chdir(cmd->arguments[1]) == -1) {
                    perror("cd failed");
                }
            }
            freeCmdLines(cmd);
            continue;
        }

        // builtins: zzzz / kuku / blast
        if (handle_signals_builtins(cmd)) {
            freeCmdLines(cmd);
            continue;
        }

        // fork so the shell stays alive
        pid_t pid = fork();

        if (pid == 0) {
            // CHILD PROCESS
            execute(cmd);
            // never returns
        }
        else if (pid > 0) {
            // PARENT PROCESS

            if (debug_on) {
                fprintf(stderr, "PID: %d\n", pid);
                fprintf(stderr, "Executing command: %s\n", cmd->arguments[0]);
            }

            if (cmd -> blocking) {
                int status = 0;
                waitpid(pid, &status, 0);
            }

            freeCmdLines(cmd);
        }
        else {
            // FORK FAILED
            perror("fork failed");
            freeCmdLines(cmd);
        }
    }

    return 0;
}