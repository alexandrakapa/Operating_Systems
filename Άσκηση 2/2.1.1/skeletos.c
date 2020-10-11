#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "proc-common.h"

#define SLEEP_PROC_SEC  10
#define SLEEP_TREE_SEC  3

/*
 * Create this process tree:
 * A-+-B---D
 *   `-C
 */
void fork_procs(void)
{
	pid_t b,c,d;
	int status;
	printf("Node %s with pid %ld starts...\n","A",(long)getpid());
	change_pname("A");
	b = fork();
	if (b < 0) 
	{
		perror("fork");
		exit(EXIT_FAILURE);
	}
	if (b==0)	
	{
	printf("Node %s with pid %ld starts...\n","B",(long)getpid());
	change_pname("B");
        d = fork();
	if (d < 0) 
        {
                perror("fork");
                exit(EXIT_FAILURE);
        }
	if (d==0)
	{
	printf("Node %s with pid %ld starts...\n","D",(long)getpid());
	change_pname ("D");
	printf("Node D is sleeping...\n");
	sleep(SLEEP_PROC_SEC);
	printf("Node D is exiting...\n");
	exit (13);
	}
	printf("Parent B is waiting for child %s with pid %ld to terminate...\n","D",(long)d);
        wait(&status);
        explain_wait_status(d, status);

        printf("Parent B: All done, exiting...\n");
	exit(19);	
	}
	c = fork();
	if (c<0)
	{
                perror("fork");
                exit(EXIT_FAILURE);
        }
	if (c==0)
        {
        printf("Node %s with pid %ld starts...\n","C",(long)getpid());
        change_pname ("C");
        printf("Node C is sleeping...\n");
        sleep(SLEEP_PROC_SEC);
        printf("Node C is exiting...\n");
        exit (17);
	}
	printf("Parent A is waiting for his children to terminate...\n");
        wait(&status);
        explain_wait_status(c, status);
	wait(&status);
	explain_wait_status(b, status);	

        printf("Parent A: All done, exiting...\n");
        exit(16);	
}


/*
 * The initial process forks the root of the process tree,
 * waits for the process tree to be completely created,
 * then takes a photo of it using show_pstree().
 *
 * How to wait for the process tree to be ready?
 * In ask2-{fork, tree}:
 *      wait for a few seconds, hope for the best.
 */
int main(void)
{
	pid_t pid;
	int status;

	/* Fork root of process tree */
	pid = fork();
	if (pid < 0) {
		perror("main: fork");
		exit(1);
	}
	if (pid == 0) {
		/* Child */
		fork_procs();
		exit(1);
	}

	/*
	 * Father
	 */

	/* for ask2-{fork, tree} */
	sleep(SLEEP_TREE_SEC);

	/* Print the process tree root at pid */
	show_pstree(pid);

	/* for ask2-signals */
	/* kill(pid, SIGCONT); */

	/* Wait for the root of the process tree to terminate */
	pid = wait(&status);
	explain_wait_status(pid, status);

	return 0;
}
