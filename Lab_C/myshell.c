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

#define TERMINATED  -1
#define RUNNING      1
#define SUSPENDED    0

#define HISTLEN 10
char *history[HISTLEN] = {0}; // circular buffer for history
int history_start = 0;  
int history_count = 0;  



typedef struct process {
    cmdLine *cmd;           /* the parsed command line */
    pid_t pid;              /* the process id that is running the command */
    int status;             /* RUNNING / SUSPENDED / TERMINATED */
    struct process *next;   /* next process in chain */
} process;


 // Global process list head
process *process_list = NULL;

cmdLine *cloneCmdLine(const cmdLine *src) {
    if (src == NULL) {
        return NULL;
    }

    cmdLine *copy = (cmdLine *)calloc(1, sizeof(cmdLine));
    if (!copy) {
        perror("calloc");
        exit(1);
    }

    copy->argCount = src->argCount;
    copy->blocking = src->blocking;
    copy->idx      = src->idx;

    /* copy arguments */
    for (int i = 0; i < src->argCount; ++i) {
        ((char **)copy->arguments)[i] = strdup(src->arguments[i]);
    }

    /* copy redirections */
    if (src->inputRedirect) {
        copy->inputRedirect = strdup(src->inputRedirect);
    }
    if (src->outputRedirect) {
        copy->outputRedirect = strdup(src->outputRedirect);
    }

    /* recursively copy pipeline tail, if any */
    copy->next = cloneCmdLine(src->next);

    return copy;
}

static cmdLine *cloneCmdLineSingle(const cmdLine *src) {
    if (!src) return NULL;

    cmdLine *copy = (cmdLine *)calloc(1, sizeof(cmdLine));
    if (!copy) { perror("calloc"); exit(1); }

    copy->argCount  = src->argCount;
    copy->blocking  = src->blocking;
    copy->idx       = src->idx;
    copy->next      = NULL; // only clone single cmdLine, no pipeline

    for (int i = 0; i < src->argCount; i++) {
        ((char **)copy->arguments)[i] = strdup(src->arguments[i]);
    }

    if (src->inputRedirect)
        copy->inputRedirect = strdup(src->inputRedirect);
    if (src->outputRedirect)
        copy->outputRedirect = strdup(src->outputRedirect);

    return copy;
}

void addProcess(process **process_list, cmdLine *cmd, pid_t pid) {
    process *p = (process *)malloc(sizeof(process));
    if (!p) {
        perror("malloc");
        exit(1);
    }

    p->cmd = cloneCmdLineSingle(cmd); // clone only this cmdLine, not the whole pipeline

    p->pid    = pid;
    p->status = RUNNING;
    p->next   = *process_list;
    *process_list = p;  
}

void printCmdLine(const cmdLine *cmd) {
    const cmdLine *curr = cmd;

    while (curr) {
        for (int i = 0; i < curr->argCount; ++i) {
            fprintf(stdout, "%s", curr->arguments[i]);
            if (i < curr->argCount - 1) {
                fputc(' ', stdout);
            }
        }
        if (curr->next) {
            fprintf(stdout, " | ");
        }
        curr = curr->next;
    }
}


void updateProcessStatus(process* process_list, int pid, int status) {
    while (process_list) {
        if (process_list->pid == pid) {
            process_list->status = status;
            return;
        }
        process_list = process_list->next;
    }
}

void updateProcessList(process **process_list) {

    process *p = *process_list;
    int status = 0;

    while (p) {
        pid_t res = waitpid(p->pid, &status, WNOHANG | WUNTRACED | WCONTINUED);

        if (res > 0) {
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                p->status = TERMINATED;
            } else if (WIFSTOPPED(status)) {
                p->status = SUSPENDED;
            } else if (WIFCONTINUED(status)) {
                p->status = RUNNING;
            }
        } else if (res == -1) {
            // If the child doesn't exist anymore (already waited on somewhere),
            // we will treat it as terminated so it doesn't remain "Running" forever.
            if (errno == ECHILD) {
                p->status = TERMINATED;
            }
        }

        p = p->next;
    }
}

void deleteTerminated(process **process_list) {
    process *curr = *process_list;
    process *prev = NULL;

    while (curr) {
        if (curr->status == TERMINATED) {
            process *to_delete = curr;
            curr = curr->next;

            if (prev) {
                prev->next = curr;
            } else {
                *process_list = curr;
            }

            freeCmdLines(to_delete->cmd);
            free(to_delete);
        } else {
            prev = curr;
            curr = curr->next;
        }
    }
}

void freeProcessList(process *process_list) {
    while (process_list) {
        process *next = process_list->next;
        freeCmdLines(process_list->cmd);
        free(process_list);
        process_list = next;
    }
}

void printProcessList(process** process_list) {

    updateProcessList(process_list);

    fprintf(stdout, "PID\t\tCOMMAND\t\tSTATUS\n");

    process *p = *process_list;
    while (p) {
        fprintf(stdout, "%d\t\t", (int)p->pid);

        printCmdLine(p->cmd);   
        fprintf(stdout, "\t\t");

        if (p->status == RUNNING) {
            fprintf(stdout, "Running\t\t");
        } else if (p->status == SUSPENDED) {
            fprintf(stdout, "Suspended\t");
        } else if (p->status == TERMINATED) {
            fprintf(stdout, "Terminated\t");
        } else {
            fprintf(stdout, "Unknown\t\t");
        }

        fprintf(stdout, "\n");
        p = p->next;
    }

    deleteTerminated(process_list);
}

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

void executePipeline(cmdLine *cmd) {
    cmdLine *left = cmd;
    cmdLine *right = cmd->next;

    if (right == NULL) {
        // No actual pipe
        return;
    }

    // Only support a single pipe: cmd | cmd2
    if (right->next != NULL) {
        fprintf(stderr, "Error: only a single pipe is supported.\n");
        return;
    }

    // Lab rule: left side must NOT redirect output (would bypass pipe)
    if (left->outputRedirect != NULL) {
        fprintf(stderr,
                "Error: cannot redirect output of left-hand command when using a pipe.\n");
        return;
    }

    // Lab rule: right side must NOT redirect input (would bypass pipe)
    if (right->inputRedirect != NULL) {
        fprintf(stderr,
                "Error: cannot redirect input of right-hand command when using a pipe.\n");
        return;
    }

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return;
    }

    // First child: left side of pipe 
    pid_t pid1 = fork();
    if (pid1 < 0) {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    }

    if (pid1 == 0) {
        // CHILD 1: stdout -> pipe write end

        if (dup2(pipefd[1], STDOUT_FILENO) < 0) {
            perror("dup2 (child1 stdout)");
            _exit(1);
        }

        // No need for these in the child after dup2
        close(pipefd[0]);
        close(pipefd[1]);

        // execute() still handles input redirection (<) if any
        execute(left);

        // If exec failed inside execute()
        _exit(1);
    }

    // Second child: right side of pipe 
    pid_t pid2 = fork();
    if (pid2 < 0) {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    }

    if (pid2 == 0) {
        // CHILD 2: stdin <- pipe read end

        if (dup2(pipefd[0], STDIN_FILENO) < 0) {
            perror("dup2 (child2 stdin)");
            _exit(1);
        }

        // No need for these in the child after dup2
        close(pipefd[0]);
        close(pipefd[1]);

        // execute() still handles output redirection (>) if any
        execute(right);

        _exit(1);
    }

    // PARENT
    // Parent doesn't use the pipe ends
    close(pipefd[0]);
    close(pipefd[1]);

    addProcess(&process_list, left, pid1);
    addProcess(&process_list, right, pid2);


    // Decide whether to wait based on the LAST command in the pipe
    cmdLine *last = cmd;
    while (last->next != NULL) {
        last = last->next;
    }

    if (last->blocking) {
        int status;
        waitpid(pid1, &status, 0);
        updateProcessStatus(process_list, pid1, TERMINATED);
        waitpid(pid2, &status, 0);
        updateProcessStatus(process_list, pid2, TERMINATED);
    } else {
        if (debug_on) {
            fprintf(stderr,
                    "Running pipeline in background: %d | %d\n",
                    (int)pid1, (int)pid2);
        }
        
    }
}

process* findProcess(process *plist, pid_t pid) {
    while (plist) {
        if (plist->pid == pid)
            return plist;
        plist = plist->next;
    }
    return NULL;
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

    // if blast (SIGINT) is sent to a suspended process, wake it first
    if (sig == SIGINT) {
        process *p = findProcess(process_list, pid);
        if (p && p->status == SUSPENDED) {
            kill(pid, SIGCONT);   // wake it so SIGINT can take effect now
            //p->status = RUNNING;  // optional (updateProcessList will confirm anyway)
        }
    }

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

    if (strcmp(cmd->arguments[0], "procs") == 0) {
        printProcessList(&process_list);
        return 1;   // handled as builtin
}


    return 0;       
}


void add_history(const char *line) {
    if (!line || line[0] == '\0')
        return;

    // if full, overwrite oldest
    if (history_count == HISTLEN) {
        free(history[history_start]); // free memory
        history[history_start] = NULL; // clear pointer
        history_start = (history_start + 1) % HISTLEN; // advance start
        history_count--; // will be incremented below
    }

    int idx = (history_start + history_count) % HISTLEN; // next free slot
    history[idx] = strdup(line); // make a copy
    if (!history[idx]) { // check strdup success
        perror("strdup");
        exit(1);
    }
    history_count++; 
}

const char *history_get(int i) {
    // i is logical index: 0 = oldest, history_count-1 = newest
    if (i < 0 || i >= history_count) // out of bounds
        return NULL;

    int idx = (history_start + i) % HISTLEN; // map to circular buffer index
    return history[idx];
}

void print_history(void) {
    for (int i = 0; i < history_count; i++) {
        const char *cmd = history_get(i); 
        printf("%d %s\n", i, cmd); // print with logical index
    }
}

void free_history(void) {
    for (int i = 0; i < HISTLEN; i++) { 
        free(history[i]); // free each command string
        history[i] = NULL;
    }
    history_start = 0;
    history_count = 0;
}

// returns pointer to first non-space char inside s 
static char *trim_spaces(char *s) {
    while (*s == ' ' || *s == '\t') s++;

    char *end = s + strlen(s);
    while (end > s && (end[-1] == ' ' || end[-1] == '\t')) {
        end--;
    }
    *end = '\0';
    return s;
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
        
        // update process list: remove terminated processes
        if (process_list != NULL) {
            updateProcessList(&process_list); // update statuses
        }

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

        // trim spaces/tabs and ignore empty-after-trim lines
        char *line = trim_spaces(input);
        if (line[0] == '\0') {
            continue;
        }


        // history expansion handling
        char *exec_line = NULL;

        
        if (strcmp(line, "history") == 0) {
            print_history();
            continue;
        }

        // "!!" - retrive last command
        if (strcmp(line, "!!") == 0) {
            if (history_count == 0) {
                printf("No commands in history\n");
                continue;
            }

            const char *last = history_get(history_count - 1); // get last command
            exec_line = strdup(last); // make a copy
            if (!exec_line) { perror("strdup"); exit(1); } // copy last command
            add_history(exec_line); // store expanded command
        }

        // single "!" -> invalid
        else if (strcmp(line, "!") == 0) {
            printf("Invalid history command\n");
            continue;
        }

        // "!n"
        else if (line[0] == '!' && line[1] != '\0') {
            char *p = line + 1;
            while (*p == ' ' || *p == '\t') p++; // skip spaces
            char *endptr = NULL;
            long n = strtol(p, &endptr, 10); // convert to long

            while (*endptr == ' ' || *endptr == '\t') endptr++; // skip trailing spaces

            if (*p == '\0'|| *endptr != '\0' || n < 0 || n >= history_count) { // invalid index
                printf("Invalid history index\n");
                continue;
            }

            const char *entry = history_get((int)n); // get nth command
            exec_line = strdup(entry); // make a copy
            if (!exec_line) { perror("strdup"); exit(1); }
            add_history(exec_line); // store expanded command, not "!n"
        }
        // regular command
        else {
            add_history(line);
            exec_line = strdup(line);
            if (!exec_line) { perror("strdup"); exit(1); }
        }

        // parse the expanded line into cmdLine
        cmdLine *cmd = parseCmdLines(exec_line);
        if (cmd == NULL) {
            free(exec_line);
            continue;
        }

        // check for "quit" shell command
        if (strcmp(cmd->arguments[0], "quit") == 0) {
            freeCmdLines(cmd);
            free(exec_line);
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
            free(exec_line);
            continue;
        }

        // builtins: zzzz / kuku / blast
        if (handle_signals_builtins(cmd)) {
            freeCmdLines(cmd);
            free(exec_line);
            continue;
        }

         // pipeline support
        if (cmd->next != NULL) {
            // There is at least one pipe: cmd | cmd->next
            executePipeline(cmd);
            freeCmdLines(cmd);
            free(exec_line);
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

            // record this process in our process list 
            addProcess(&process_list, cmd, pid);

            if (debug_on) {
                fprintf(stderr, "PID: %d\n", pid);
                fprintf(stderr, "Executing command: %s\n", cmd->arguments[0]);
            }

            if (cmd -> blocking) {
                int status = 0;
                waitpid(pid, &status, 0);
                updateProcessStatus(process_list, pid, TERMINATED);
            }

            freeCmdLines(cmd);
            free(exec_line);
        }
        else {
            // FORK FAILED
            perror("fork failed");
            freeCmdLines(cmd);
            free(exec_line);
        }
    }
    free_history();
    freeProcessList(process_list);
    return 0;
}
