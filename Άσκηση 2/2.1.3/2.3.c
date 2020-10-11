#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "tree.h"
#include "proc-common.h"
#include <stdio.h>
#define SLEEP_TREE_SEC 2
#define SLEEP_PROC_SEC 6

int got_sigusr1=0;
void sighandler(int sugnum)
{
	got_sigusr1 = 1;
}

void forkprocs (struct tree_node *node)
{
	int i;
    	int status;
    	pid_t pid,pid2[node->nr_children];
        printf("PID = %ld, name %s,that has children,is starting...\n", (long)getpid(), node->name);
    	change_pname(node->name);

	for (i=0;i<node->nr_children;i++)   //we traverse all the children of the node
	{
	pid2[i]=fork();
		if(pid2[i]==0)//if (fork()==0)    //create a child
		{
		change_pname((node->children + i)->name);  //for example in tree A->B->D , //nr_children=2, children has B and children+1 has C    
                                                           //                    |->C
			if ((node->children + i)->nr_children != 0) //This node has more children
			{
			forkprocs(node->children+i);
			}
			else if ((node->children + i)->nr_children == 0) //This node doesn't have children
			{
			signal(SIGCONT, sighandler);
printf("I sent a signal to node %s with pid %ld to continue\n",(node->children+i)->name,(long)getpid());
			raise(SIGSTOP);
//printf("I sent a signal to node %s with pid %ld to stop\n",(node->children+i)->name,(long)getpid());
printf("PID = %ld, name = %s is awake\n",(long)getpid(), node->name);
			//printf("%s\n",(node->children + i)->name);
		//	printf("Hello, this is node %s, I am a leaf and now I will sleep\n",(node->children+i)->name);
		//	sleep(SLEEP_PROC_SEC);  //put leaves to sleep
			exit(0);
			}
		}
	}

signal(SIGCONT,sighandler);
//printf("I sent a signal to node %s with pid %ld to continue\n",(node->children+i)->name,(long)getpid());
wait_for_ready_children(node->nr_children);

raise(SIGSTOP);  //put current process to sleep
//printf("I put current process to sleep %ld \n",(long)getpid());
//printf("I sent a signal to node %s with pid %ld to stop\n",(node->children+i)->name,(long)getpid());
	for (i=0; i<node->nr_children; i++){
		kill(pid2[i],SIGCONT);
//	printf("I send a signal to %s to continue\n",(node->children+i)->name);		
     		pid = wait(&status);
        	explain_wait_status(pid, status);
    	}
//printf("%s\n",node->name);
    exit(0);
}

int main(int argc, char *argv[])
{
        struct tree_node *root;
        pid_t pid;
        int status;

        if (argc != 2) {
                fprintf(stderr, "Usage: %s <input_tree_file>\n\n", argv[0]);
                exit(1);
        }
	
	if (argc<2)
	{
	fprintf(stderr, "Usage:%s <tree_file>\n",argv[0]);
	exit(1);
	}
        root = get_tree_from_file(argv[1]);

        pid = fork();
      
	if (pid<0)
        {
        perror("fork");
        exit(EXIT_FAILURE);
        }
        if (pid==0)
        {
        forkprocs(root);
        }

        wait_for_ready_children(1);
        show_pstree (pid);
	
	kill(pid,SIGCONT);	

        pid = wait (&status);
        explain_wait_status (pid, status);

        return 0;
}

