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
#define SHELL_EXECUTABLE_NAME "shell" /* executable for shell */

typedef struct proc  //our struct is now a linked list
{
	int id;
	pid_t pid;
	char task_name[TASK_NAME_SZ];
	struct proc * next; //index to the next process
	char priority;
} process;

/* Global Variables. */

static process *current;
static process *temp;
static int num_high=0;

static void insert_process(int id, pid_t p, char* name)  //insert a process to the list
{
        process *temp = (struct proc *) malloc(sizeof(struct proc));
        temp->id = id;
        temp->pid = p;
	temp->priority='l'; 	//by default we want it to be low
        strcpy(temp->task_name,name);
        temp->next = current->next;	//since we have inserted a process,we have to make it current
        current->next = temp;      
        current = temp;
}

static void remove_process(int id)    //remove a process from the list
{       temp = current;
        while (temp->next->id!=id)	//we are searching for the specific id
        {
        temp=temp->next;
	}
        temp->next = temp->next->next;  //we remove the node
}

static int sched_high_task(int id)	//it takes the id and changes the priority to high
{
	int valid=1;
	temp=current;
	while (temp->id != id)
	{
		temp=temp->next;
		if (temp==current) return -1; 	//we didnt find the process 
	}
	if(temp->priority=='h')
	{
	valid=0;
	}
	if(valid==1){
	temp->priority='h';
	num_high++;
	if(num_high==1)
	{
		process *shell;
		shell=temp;
		while (strcmp(shell->task_name,  SHELL_EXECUTABLE_NAME)!=0)
		{
		shell=shell->next;
		if (temp==current) return -1; 	//we didnt find the process 
		}
		shell->priority='h';
		printf("We have changed the priority of process %s with id = %d and PID = %d to HIGH. It is the first process with HIGH priority so we changed the shell to HIGH also.\n",temp->task_name,temp->id,temp->pid);
		num_high++;
	}
	else
	{
	printf("We have changed the priority of process %s with id = %d and PID = %d to HIGH\n",temp->task_name,temp->id,temp->pid);
	}
	}
	return (temp->id); 
}

static int sched_low_task(int id)      //it takes the id and changes the priority to low
{
	int valid=1;
        temp=current;
        while (temp->id != id)
        {
 	        temp=temp->next;
       		if (temp==current) return -1;    //we didnt find the process 
        }
	if(temp->priority=='l'){
	valid=0;
	}
	if(valid==1){
        temp->priority='l';
	num_high--;
	if(num_high==1)
	{
		process *shell;
		shell=temp;
		while (strcmp(shell->task_name,  SHELL_EXECUTABLE_NAME)!=0)
		{
		shell=shell->next;
		if (temp==current) return -1; 	//we didnt find the process 
		}
		shell->priority='l';
		printf("We have changed the priority of process %s with id = %d and PID = %d to LOW. It was the last process with HIGH priority so we changed the shell to LOW also.\n",temp->task_name,temp->id,temp->pid);
		num_high--;
	}
	else
	{
	printf("We have changed the priority of process %s with id = %d and PID = %d to LOW\n",temp->task_name,temp->id,temp->pid);
	}
	}
        return (temp->id);
}


static int nproc;	//number of processes

/* Print a list of all tasks currently being scheduled.  */
static void
sched_print_tasks(void)
{
        temp = current;
	char *prio="low";//=malloc(sizeof(char));
	if (temp->priority == 'h') prio="high";
	else prio="low";
        printf("Process with id = %d, PID = %d, name = %s and %s priority, is the current process\n",temp->id,temp->pid,temp->task_name,prio);
	while(temp->next!=current){	//we have a circular list
                        temp=temp->next;
			if (temp->priority == 'h') prio="high";
		        else prio="low";
                        printf("Process with id = %d, PID = %d, name = %s and %s priority is about to run\n",temp->id,temp->pid,temp->task_name,prio);
        }
}

/* Send SIGKILL to a task determined by the value of its
 * scheduler-specific id.
 */
static int
sched_kill_task_by_id(int id)
{
        temp = current;
        while (temp->id!=id){   //we search for the process based on its id
                temp=temp->next;
                if(temp==current) return -1;      //if we dont find it we exit
        }
        printf("We now have killed process with id = %d and pid = %d\n",id,temp->pid);
        kill(temp->pid, SIGKILL);
        return(id);
}

static void
create_process(char *executable)	//as in 4.1
{
        char *newargv[] = { executable , NULL, NULL, NULL};
        char *newenviron[] = { NULL };
        raise(SIGSTOP);      
        execve( executable , newargv, newenviron);
        /* execve() only returns on error */
        perror("scheduler: child: execve");
        exit(1);
}

/* Create a new task.  */
static void
sched_create_task(char *executable)
{
        nproc++;
        printf("We are going to create process with name = %s and id = %d\n",executable,nproc-1);
        pid_t p = fork();       //as in main of 4.1
        if (p<0){
                perror("waitpid");
        }
        if (p==0){
                create_process(executable);   //create_process
        }
        else{
                insert_process(nproc-1,p,executable);    //if it exists just put it in the list
        }
}

/* Process requests by the shell.  */
static int
process_request(struct request_struct *rq)
{
	switch (rq->request_no) {
		case REQ_PRINT_TASKS:
			sched_print_tasks();
			return 0;

		case REQ_KILL_TASK:
			return sched_kill_task_by_id(rq->task_arg);

		case REQ_EXEC_TASK:
			sched_create_task(rq->exec_task_arg);			
			return 0;

		case REQ_HIGH_TASK:		//we include these two requests for high and low priority
			sched_high_task(rq->task_arg);
			return 0;
		case REQ_LOW_TASK:
			sched_low_task(rq->task_arg);
			return 0;
		default:
			return -ENOSYS;
	}
}

/* 
 * SIGALRM handler
 */
static void
sigalrm_handler(int signum)
{
//	printf("--------!!!The time quantum has ended!!!--------\n");  //Arranges for a SIGALRM signal to be delivered in SCED_TQ_SEC seconds
        //When time quantum ends we need to stop the process
	kill(current->pid, SIGSTOP);
}

/* 
 * SIGCHLD handler
 */
static void
sigchld_handler(int signum)
{
	pid_t p;
	int status;
	for(;;){
		p = waitpid(-1, &status, WUNTRACED | WNOHANG); //returns the ID of the child that has changed
                        //We chose options WNOHANG because it returns immediately if no child has exited and WUNTRACED
                        //because it returns if a child has stopped 
		if(p==0) break;
		//explain_wait_status(p, status);
		while(current->pid != p)	//we find the process with the exact pid
		{
		current=current->next;
		}
		if (WIFEXITED(status)  || WIFSIGNALED(status)) {	//WIFEXITED(status) : returns true if the child terminated normally
                                                                //WIFSIGNALED(status) : returns true if the child process was terminated by a signal
			/* A child has died */
			if(current->next == current){
				printf("Now all %d processes are dead\n",nproc); //there arent any processes left
				exit(0);		
			}
			remove_process(current->id); //since the process has terminated,we have to remove it
		}
		temp=current->next;
		while (temp->priority!='h')
		{
			temp=temp->next;	//we have to find high priority to execute
			if (temp==current)	//we returned to the beginning
			{
				if (current->priority=='h') break; 	//we found one so we stop
				else 
				{
				temp=temp->next;	//we didnt find any high and we break
				break;
				}
			}	
		}
		current=temp;
		/*
		char *h_l;
		if (current->priority=='l') h_l="low";
		else h_l="high";
		printf("We now start process %s with id = %d, PID = %d and %s priority \n",current->task_name, current->id,current->pid,h_l);
		*/
		alarm(SCHED_TQ_SEC);	
		kill(current->pid, SIGCONT); 
	}
}

/* Disable delivery of SIGALRM and SIGCHLD. */
static void
signals_disable(void)
{
        sigset_t sigset;

        sigemptyset(&sigset);
        sigaddset(&sigset, SIGALRM);
        sigaddset(&sigset, SIGCHLD);
        if (sigprocmask(SIG_BLOCK, &sigset, NULL) < 0) {
                perror("signals_disable: sigprocmask");
                exit(1);
        }
}

/* Enable delivery of SIGALRM and SIGCHLD.  */
static void
signals_enable(void)
{
        sigset_t sigset;

        sigemptyset(&sigset);
        sigaddset(&sigset, SIGALRM);
        sigaddset(&sigset, SIGCHLD);
        if (sigprocmask(SIG_UNBLOCK, &sigset, NULL) < 0) {
                perror("signals_enable: sigprocmask");
                exit(1);
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
do_shell(char *executable, int wfd, int rfd)
{
	char arg1[10], arg2[10];
	char *newargv[] = { executable, NULL, NULL, NULL };
	char *newenviron[] = { NULL };

	sprintf(arg1, "%05d", wfd);
	sprintf(arg2, "%05d", rfd);
	newargv[1] = arg1;
	newargv[2] = arg2;

	raise(SIGSTOP);
	execve(executable, newargv, newenviron);

	/* execve() only returns on error */
	perror("scheduler: child: execve");
	exit(1);
}

/* Create a new shell task.
 *
 * The shell gets special treatment:
 * two pipes are created for communication and passed
 * as command-line arguments to the executable.
 */
static void
sched_create_shell(char *executable, int *request_fd, int *return_fd)
{
	pid_t p;
	int pfds_rq[2], pfds_ret[2];

	if (pipe(pfds_rq) < 0 || pipe(pfds_ret) < 0) {
		perror("pipe");
		exit(1);
	}

	p = fork();
	if (p < 0) {
		perror("scheduler: fork");
		exit(1);
	}

	if (p == 0) {
		/* Child */
		close(pfds_rq[0]);
		close(pfds_ret[1]);
		do_shell(executable, pfds_rq[1], pfds_ret[0]);	/*create the shell*/
		assert(0);
	}
	/* Parent */
	current=(struct proc *) malloc(sizeof(struct proc));
	current->pid = p;	
	current->priority='l';	//because the first process we have is shell so it has to be low
	strcpy(current->task_name,SHELL_EXECUTABLE_NAME);
	current->next = current;	/*insert shell to list*/
	close(pfds_rq[1]);
	close(pfds_ret[0]);
	*request_fd = pfds_rq[0];
	*return_fd = pfds_ret[1];
}

static void
shell_request_loop(int request_fd, int return_fd)
{
	int ret;
	struct request_struct rq;

	/*
	 * Keep receiving requests from the shell.
	 */
	for (;;) {
		if (read(request_fd, &rq, sizeof(rq)) != sizeof(rq)) {
			perror("scheduler: read from shell");
			fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
			break;
		}
		signals_disable();
		ret = process_request(&rq);
		signals_enable();

		if (write(return_fd, &ret, sizeof(ret)) != sizeof(ret)) {
			perror("scheduler: write to shell");
			fprintf(stderr, "Scheduler: giving up on shell request processing.\n");
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	nproc=argc;
	/* Two file descriptors for communication with the shell */
	static int request_fd, return_fd;
	pid_t p;
	int i;
	/* Create the shell. */
	sched_create_shell(SHELL_EXECUTABLE_NAME, &request_fd, &return_fd);
	/* TODO: add the shell to the scheduler's tasks */

	for (i=1; i < nproc; i++){
		p = fork();
		if (p<0)
        	{
                perror("waitpid");
                exit(1);
        	}
		if (p == 0){
			create_process(argv[i]);
		}
		else{
			insert_process(i,p,argv[i]);
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
		kill(current->pid, SIGCONT);	
	}

	shell_request_loop(request_fd, return_fd);

	/* Now that the shell is gone, just loop forever
	 * until we exit from inside a signal handler.
	 */
	while (pause())
		;

	/* Unreachable */
	fprintf(stderr, "Internal error: Reached unreachable point\n");
	return 1;
}
