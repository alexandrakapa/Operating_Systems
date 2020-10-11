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




pid_t forkprocs (struct tree_node *node,int fd[2])

{
	int i=0;
	int status;
        pid_t pid;

	int fd1[2], fd2[2]; //We need two pipes,one for the first child and one for the second

	int num1, num2;  //The two numbers of every caculation
	pid = fork();
	if (pid<0){	
		perror("fork");
		exit(-1);
	}
	if (pid==0){
		change_pname(node->name);	
		printf("We have created %s \n", node->name);	
		if (node->nr_children==0){	//This node doesn't have children                    
                        num1 = atoi(node->name);   //converts the string to an int
                        printf("Now %s with pid %d sends %d to pipe \n", node->name,getpid(),num1);
                        if (write(fd[1],&num1,sizeof(num1))<0){	//We send the name to the father
				perror("write pipe");
				exit(-1);
			}
                        sleep(SLEEP_PROC_SEC);  
                        exit(0);
                }		
		if (pipe(fd1)<0){                //pipe for the first child
                	perror ("pipe");
                	exit(-1);
       		}
		forkprocs(node->children, fd1);	//for the first child
		
		if (pipe(fd2)<0){
                        perror ("pipe");	//another pipe for the second child
                        exit(-1);
                }
                forkprocs(node->children+1, fd2); //for the second child	
		for(i=0;i<2;i++)
		{
		pid = wait(&status);            //because we have two children
 		explain_wait_status(pid, status);
		}
       	        
		if (read(fd1[0],&num1,sizeof(num1)) <0){	//read from first pipe the first child
                                perror("read pipe");
                                exit(-1);
                        }
		if (read(fd2[0],&num2,sizeof(num2)) <0){	//read from second pipe the second child
                                perror("read pipe");
                                exit(-1);
                        }

		printf("Calculation %s with pid %d reads %d and %d from pipe \n" , node->name, getpid(),num1,num2);
		i=num1;   //to save for the output
		if (node->name[0]=='+') num1=num1+num2;            //choose operation
		else if (node->name[0]=='*') num1=num1*num2;
	
		printf("Calculation %s with pid %d sends the result %d (%d %s %d) to pipe \n",node->name,getpid(),num1,i,node->name,num2);
		if (write(fd[1],&num1,sizeof(num1))<0){	    //send the result
                                perror("write pipe");
                                exit(-1);
                        }
        	exit(0);
	}
	return pid;

}


int main(int argc, char *argv[])
{
        struct tree_node *root;
        pid_t pid;
	int fd[2];   //pipe between father and child        
	int num1;
	int status;
        if (argc != 2) {
                fprintf(stderr, "Usage: %s <input_tree_file>\n\n", argv[0]);
                exit(1);
        }

        root = get_tree_from_file(argv[1]);
	
	if (pipe(fd)<0)
	{
	perror("pipe");
	exit(1);
	}

	if (pipe(fd)<0)
    	{
      		perror ("pipe");
      		return EXIT_FAILURE;
    	}
	pid=forkprocs(root,fd);   //We have the pid of the root
	sleep (SLEEP_TREE_SEC);
        show_pstree (pid);
        pid = wait (&status);
        explain_wait_status (pid, status);
	
	if (read(fd[0],&num1,sizeof(num1))<0)
	{
	perror("read pipe");                  //We read the result from the pipe
	exit(-1);
	}
	printf("The result is %d! \n",num1);
        return 0;
}

