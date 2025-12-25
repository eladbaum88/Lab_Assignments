#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

static void handler(int sig)
{
	printf("\nRecieved Signal : %s\n", strsignal(sig));
    fflush(stdout);

    if (sig == SIGINT){
        signal(SIGINT, SIG_DFL); // reset to default handler
        raise(SIGINT); // re-raise the signal
    }

	else if (sig == SIGTSTP)
	{
        signal(SIGCONT, handler); // set handler for SIGCONT
		signal(SIGTSTP, SIG_DFL); // reset to default handler
        raise(SIGTSTP); // stop the process
	}
	else if (sig == SIGCONT)
	{
        signal(SIGTSTP, handler); //new handler for SIGTSTP
		signal(SIGCONT, SIG_DFL); // reset to default handler
        raise(SIGCONT); // continue the process
	}
	else {
        signal(sig, SIG_DFL);
	    raise(sig);
    }
}

int main(int argc, char **argv)
{

	printf("Starting the program\n");
	signal(SIGINT, handler);
	signal(SIGTSTP, handler);
	signal(SIGCONT, handler);

	while (1)
	{
		sleep(1);
	}

	return 0;
}