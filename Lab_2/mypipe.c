#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     
#include <sys/types.h>  
#include <sys/wait.h>   

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s <message>\n", argv[0]);
        return 1;
    }

    const char *msg = argv[1]; // wrong - need to change
    int fd[2];

    // create the pipe: fd[0] = read end, fd[1] = write end
    if (pipe(fd) == -1) {
        perror("pipe failed - fork");
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        // close both ends of the pipe before exiting
        close(fd[0]);
        close(fd[1]);
        return 1;
    }

    if (pid == 0) {
        // CHILD: write message into the pipe
        close(fd[0]);  // child doesn't read

        size_t len = strlen(msg);

        // write the message (without '\0') into the pipe
        ssize_t written = write(fd[1], msg, len);
        if (written == -1) {
            perror("write");
            close(fd[1]);
            _exit(1);
        }

        close(fd[1]);
        _exit(0);
    } else {
        
        close(fd[1]);  // parent doesn't write

        char buffer[4096];
        ssize_t n = read(fd[0], buffer, sizeof(buffer) - 1);
        if (n == -1) {
            perror("read");
            close(fd[0]);
            waitpid(pid, NULL, 0);
            return 1;
        }

        buffer[n] = '\0';  // null-terminate so we can use printf safely

        printf("%s\n", buffer);

        close(fd[0]);
        waitpid(pid, NULL, 0);  // reap child
    }

    return 0;
}
