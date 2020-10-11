/*
 * mandel.c
 *
 * A program to draw the Mandelbrot Set on a 256-color xterm.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "mandel-lib.h"
#include <errno.h>
#include <signal.h>
#define perror_pthread(ret, msg) \
        do { errno = ret; perror(msg); } while (0)
#define MANDEL_MAX_ITERATION 100000

/***************************
 * Compile-time parameters *
 ***************************/

void signalhandler(int ctrl) 		//What to do if the user presses Ctrl-C
{
signal(ctrl, SIG_IGN);			//SIG_IGN is used to ignore the signal
reset_xterm_color(1);			//Reset color
exit(0);
}


/*
 * Output at the terminal is x_chars wide by y_chars long
*/
int y_chars = 40;
int x_chars = 130;
int thread_no;//=3;      //the number of threads

typedef struct		//rename the struct
{
	pthread_t thread_id;	//id for every thread that is created
	int line;		//the line for every thread
	sem_t semaphore;	//semaphore between line and line+1
}pthread_struct;

pthread_struct *thread;


/*
 * The part of the complex plane to be drawn:
 * upper left corner is (xmin, ymax), lower right corner is (xmax, ymin)
*/
double xmin = -1.8, xmax = 1.0;
double ymin = -1.0, ymax = 1.0;
	
/*
 * Every character in the final output is
 * xstep x ystep units wide on the complex plane.
 */
double xstep;
double ystep;

/*
 * This function computes a line of output
 * as an array of x_char color values.
 */
void compute_mandel_line(int line, int color_val[])
{
	/*
	 * x and y traverse the complex plane.
	 */
	double x, y;

	int n;
	int val;

	/* Find out the y value corresponding to this line */
	y = ymax - ystep * line;

	/* and iterate for all points on this line */
	for (x = xmin, n = 0; n < x_chars; x+= xstep, n++) {

		/* Compute the point's color value */
		val = mandel_iterations_at_point(x, y, MANDEL_MAX_ITERATION);
		if (val > 255)
			val = 255;

		/* And store it in the color_val[] array */
		val = xterm_color(val);
		color_val[n] = val;
	}
}

/*
 * This function outputs an array of x_char color values
 * to a 256-color xterm.
 */
void output_mandel_line(int fd, int color_val[])
{
	int i;
	
	char point ='@';
	char newline='\n';

	for (i = 0; i < x_chars; i++) {
		/* Set the current color, then output the point */
		set_xterm_color(fd, color_val[i]);
		if (write(fd, &point, 1) != 1) {
			perror("compute_and_output_mandel_line: write point");
			exit(1);
		}
	}

	/* Now that the line is done, output a newline character */
	if (write(fd, &newline, 1) != 1) {
		perror("compute_and_output_mandel_line: write newline");
		exit(1);
	}
}

void *compute_and_output_mandel_line(void *arg)  //arg points to an instance of pthread_struct
{
	int i;	//i is the line we have now
	int line2=*(int *)arg;	//line2 is the first line of the thread(0,1,...,n-1)
				//some explanation about pointers: arg points to pthread_struct,
				//with (int *)arg we are dereferencing arg to get the address of the
				//instance and with *(int *)arg we get the real value of the instance
	for (i=line2;i<y_chars;i=i+thread_no)	//the next line of the thread is given by i,i+n,i+2n etc
	{
	int color_val[x_chars];
	compute_mandel_line(i, color_val);	//compute i line
	sem_wait(&thread[(i%thread_no)].semaphore);		//syncronization begins,we wait until semaphore of the specific thread comes
	output_mandel_line(1, color_val);	        //draw the line to the standart output
	sem_post(&thread[((i%thread_no)+1)%thread_no].semaphore);	//increase the semaphore of the next thread
	}
/*
An example to explain the above semaphores:
We have 2 threads(thread_no=2) so line 0,2,4,.. is painted by thread 0 and line 1,3,5,..
is painted by thread 1.
If for example we want to compute line 5(i=5),then we have to bring semaphore
for thread 1(to lock) and we do it with i%thread_no=1.Then we print.Then we unlock and we have to bring 
the semaphore for the next thread(thread 0 again) and we do it with ((i%thread_no)+1)%thread_no=(1+1)%2=0.
*/
return 0;
}

void *safe_malloc(size_t size)
{
    void *p;

    if ((p = malloc(size)) == NULL) {
        fprintf(stderr, "Out of memory, failed to allocate %zd bytes\n",
            size);
        exit(1);
    }

    return p;
}

void usage(char *argv0)
{
    fprintf(stderr, "Usage: %s thread_no\n\n"
        "Exactly one argument required:\n"
        "    thread_no: The number of threads to create.\n",
        argv0);
    exit(1);
}

int main(int argc, char *argv[])
{
	signal(SIGINT, signalhandler);
	int line2,create,join;
	
	if (argc != 2)
	{
	 usage(argv[0]);
	}		

	xstep = (xmax - xmin) / x_chars;
	ystep = (ymax - ymin) / y_chars;
	
	/*
	 * draw the Mandelbrot Set, one line at a time.
	 * Output is sent to file descriptor '1', i.e., standard output.
	 */
	int threads = atoi(argv[1]);
	thread_no=threads;

	if (thread_no < 1 || thread_no > y_chars-1)
	{
	printf("Usage: %s thread_no\n\n"
        "Out of borders:\n"
        "    thread_no: The number of threads to create.\n",argv[0]);
    	exit(1);
	}

	thread=(pthread_struct *)safe_malloc(thread_no*sizeof(pthread_struct));
	sem_init(&thread[0].semaphore,0,1);	//We initialise semaphore(semaphore 0) with value 1
	if (thread_no>1)	//If we have many threads then we have the same number of semaphores
	{
	for (line2 =1; line2 < thread_no;line2++)
		{
		sem_init(&thread[line2].semaphore,0,0);	//these semaphores have value 0 because they wait
		}
	}
	
	for (line2=0;line2<thread_no;line2++)	//We create thread_no threads and each one goes to the
	{					//respective line to begin from there
	thread[line2].line=line2;		//So if we have 2 threads then the 1st(thread[0])
                                                //begins at line=0 and the second
                                                //(thread[1]) begins at line=1
	create=pthread_create(&thread[line2].thread_id,NULL,compute_and_output_mandel_line,&thread[line2].line);
		if (create) {
		perror_pthread(create, "pthread_create");		
        	exit(1);
		}
	}
	
	for (line2=0;line2<thread_no;line2++)	//We join threads for the output
	{
	join=pthread_join(thread[line2].thread_id,NULL);
		if (join){
		perror_pthread(join, "pthread_join");	
		}
	}
	reset_xterm_color(1);
	return 0;
}
