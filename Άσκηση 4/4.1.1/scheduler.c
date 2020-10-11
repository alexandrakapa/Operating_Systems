#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <assert.h>

#include <sys/wait.h>
#include <sys/types.h>

#include "proc-common.h"
#include "request.h"

/* Compile-time parameters. */
#define SCHED_TQ_SEC 2                /* time quantum */
#define TASK_NAME_SZ 60               /* maximum size for a task's name */

/* Global Variables. */
int *process_id, current, nproc;
int *array;		//array is our struct
/*
 * SIGALRM handler
 */
static void
sigalrm_handler(int signum)
{
	printf("--------!!!The time quantum has ended!!!--------\n");  //Arranges for a SIGALRM signal to be delivered in SCED_TQ_SEC seconds
	//When time quantum ends we need to stop the process
	kill(process_id[current], SIGSTOP);
}

/* 
 * SIGCHLD handler
 */
static void
sigchld_handler(int signum)
{
	int counter = 0;
	pid_t p;
	int status;
	for(;;){
		p = waitpid(-1, &status, WUNTRACED | WNOHANG); //returns the ID of the child that has changed
        		//We chose options WNOHANG because it returns immediately if no child has exited and WUNTRACED
        		//because it returns if a child has stopped 
		if(p==0) break;
		process_id[current] = (int) p;
                explain_wait_status(p, status);
		if (WIFEXITED(status)  || WIFSIGNALED(status)) { //WIFEXITED(status) : returns true if the child terminated normally
								//WIFSIGNALED(status) : returns true if the child process was terminated by a signal
			/* A child has died */
			printf("Now process number %d with id %d is dead.\n",current, process_id[current]);
			array[current] = 0;	//If it is ended then we remove it from our array[]
		}
		if (WIFSTOPPED(status)) {			//WIFSTOPPED(status) : returns true if the child process was stopped by delivery of a signal
			/* A child has stopped due to SIGSTOP/SIGTSTP, etc... */
			printf("Now process number %d with id %d has been stopped.We now either go to the next process or begin from the start.\n",current, process_id[current]);
		}

		do {
        	current = (current + 1) % nproc;
		//A little explanation about that.We want the processes in circular form. For example if we have 3 processes then nproc=3.Firstly we use process number 0,
		//then current=(0+1)%3=1,then current=(1+1)%3=2,then current=(2+1)%3=0 etc 
        	counter++;
    		} while (counter <= nproc && array[current]==0);	//We exit from this while loop if we traversed all the proccess and all the processes are dead
   		if (counter == nproc + 1) {
       		printf("Now all processes are dead.\n");	//All processes are dead
        	exit(0);
    		}
		else {
		printf("------>Moving on to process number %d<------\n", current);
		alarm(SCHED_TQ_SEC);
		kill(process_id[current], SIGCONT); 
		}
	}
}

/* Install two signal handlers.
 * One for SIGCHLD, one for SIGALRM.
 * Make sure both signals are masked when one of them is running.
 */
static void
install_signal_handlers(void)
{
	sigset_t sigset;
	struct sigaction sa;

	sa.sa_handler = sigchld_handler;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGCHLD);
	sigaddset(&sigset, SIGALRM);
	sa.sa_mask = sigset;
	if (sigaction(SIGCHLD, &sa, NULL) < 0) {
		perror("sigaction: sigchld");
		exit(1);
	}

	sa.sa_handler = sigalrm_handler;
	if (sigaction(SIGALRM, &sa, NULL) < 0) {
		perror("sigaction: sigalrm");
		exit(1);
	}

	/*
	 * Ignore SIGPIPE, so that write()s to pipes
	 * with no reader do not result in us being killed,
	 * and write() returns EPIPE instead.
	 */
	if (signal(SIGPIPE, SIG_IGN) < 0) {
		perror("signal: sigpipe");
		exit(1);
	}
}

static void
create_process(char *executable)	//It creates processes with fork+execve as in the example
{
	char *newargv[] = { executable , NULL, NULL, NULL};
	char *newenviron[] = { NULL };

	raise(SIGSTOP);
	execve( executable , newargv, newenviron);
	/* execve() only returns on error */
	perror("scheduler: child: execve");
	exit(1);
}

int main(int argc, char *argv[])
{
	pid_t p;
	int i;
	nproc = argc - 1;  	//The number of processes
	current= 0;		//The current process
	process_id = malloc((nproc)*sizeof(int)); //We malloc the dynamic arrays
        array = malloc((nproc)*sizeof(int));
	/*
	 * For each of argv[1] to argv[argc - 1],
	 * create a new child process, add it to the process list.
	 */
	printf("We will fork %d processes:\n",argc-1);
	for (i=0; i < nproc; i++){
		printf("Now we fork process number %d\n", i);
		p = fork();
		if (p<0)
		{
		perror("waitpid");
		exit(1);
		}
		if (p == 0)
		{
			create_process(argv[i+1]);
		}
		else
		{
			process_id[i] = (int) p;
			array[i] = 1;	//Because it was just created so it hasn't stopped yet
		}
	}

	/* Wait for all children to raise SIGSTOP before exec()ing. */
	wait_for_ready_children(nproc);
	/* Install SIGALRM and SIGCHLD handlers. */
	install_signal_handlers();

	if (nproc == 0) {
		fprintf(stderr, "Scheduler: No tasks. Exiting...\n");
		exit(1);
	}
	else{
		alarm(SCHED_TQ_SEC);	
		kill(process_id[0], SIGCONT);
		
	}

	/* loop forever  until we exit from inside a signal handler. */
	while (pause())
		;

	/* Unreachable */
	fprintf(stderr, "Internal error: Reached unreachable point\n");
	return 1;
}
