#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     // pipe(), fork(), close(), dup(), execvp()
#include <sys/types.h>  // pid_t
#include <sys/wait.h>   // waitpid

int main(void) {
    int pipefd[2];     // pipefd[0] = read end, pipefd[1] = write end
    pid_t cpid1, cpid2;

    // Create the pipe
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(1);
    }

    // Fork first child (child1)
    fprintf(stderr, "(parent_process>forking...)\n");
    cpid1 = fork();

    if (cpid1 < 0) {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        exit(1);
    }

    if (cpid1 == 0) {
        // CHILD 1
        fprintf(stderr,
                "(child1>redirecting stdout to the write end of the pipe...)\n");

        // Close stdout (fd 1)
        close(STDOUT_FILENO);

        // Duplicate write end of pipe into fd 1
        if (dup(pipefd[1]) == -1) {
            perror("dup");
            exit(1);
        }

        // Close original write end
        close(pipefd[1]);

        // Child1 does not need read end
        close(pipefd[0]);

        fprintf(stderr,
                "(child1>going to execute cmd: ls -lsa)\n");

        char *ls_args[] = {"ls", "-lsa", NULL};
        execvp(ls_args[0], ls_args);

        perror("execvp ls");
        _exit(1);
    }

    // PARENT after first fork
    fprintf(stderr,
            "(parent_process>created process with id: %d)\n", cpid1);

    // Parent closes the write end of the pipe
    fprintf(stderr,
            "(parent_process>closing the write end of the pipe...)\n");
    close(pipefd[1]);

    // Fork second child (child2)
    fprintf(stderr, "(parent_process>forking...)\n");
    cpid2 = fork();

    if (cpid2 < 0) {
        perror("fork");
        close(pipefd[0]);
        exit(1);
    }

    if (cpid2 == 0) {
        // CHILD 2
        fprintf(stderr,
                "(child2>redirecting stdin to the read end of the pipe...)\n");

        // Close stdin (fd 0)
        close(STDIN_FILENO);

        // Duplicate read end of pipe into fd 0
        if (dup(pipefd[0]) == -1) {
            perror("dup");
            exit(1);
        }

        // Close original read end
        close(pipefd[0]);

        fprintf(stderr,
                "(child2>going to execute cmd: tail -n 3)\n");

        char *tail_args[] = {"tail", "-n", "3", NULL};
        execvp(tail_args[0], tail_args);

        perror("execvp tail");
        _exit(1);
    }

    // PARENT after second fork 
    fprintf(stderr,
            "(parent_process>created process with id: %d)\n", cpid2);

    // Parent closes the read end of the pipe
    fprintf(stderr,
            "(parent_process>closing the read end of the pipe...)\n");
    close(pipefd[0]);

    // Parent waits for children (in order)
    fprintf(stderr,
            "(parent_process>waiting for child processes to terminate...)\n");
    waitpid(cpid1, NULL, 0);
    waitpid(cpid2, NULL, 0);

    fprintf(stderr, "(parent_process>exiting...)\n");
    return 0;
}
