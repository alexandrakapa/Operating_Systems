#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "tree.h"
#include "proc-common.h"

#define SLEEP_TREE_SEC 2
#define SLEEP_PROC_SEC 6



void forkprocs (struct tree_node *node)
{
int i;
    int status;
    pid_t pid;
printf("Hello, this is node %s and I have one or more children \n", node->name);

    change_pname(node->name);

for (i=0;i<node->nr_children;i++)   //we traverse all the children of the node
{  
if (fork()==0)    //create a child
{
change_pname((node->children + i)->name);  //for example in tree A->B->D , nr_children=2, children has B and children+1 has C    
					   //                    |->C
if ((node->children + i)->nr_children != 0) //This node has more children
{
forkprocs(node->children+i);
}
else if ((node->children + i)->nr_children == 0) //This node doesn't have children
{
printf("Hello, this is node %s, I am a leaf and now I will sleep\n",(node->children+i)->name);
sleep(SLEEP_PROC_SEC);  //put leaves to sleep
exit(0);
}
}
}

for (i=0; i<node->nr_children; i++){
        pid = wait(&status);
        explain_wait_status(pid, status);
    }
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
	sleep (SLEEP_TREE_SEC);
	show_pstree (pid);
	pid = wait (&status);
	explain_wait_status (pid, status);

	return 0;
}
