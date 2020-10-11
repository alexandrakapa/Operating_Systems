#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

void doWrite (int fd, const char *buff, int len)
{
	size_t  idx;
	ssize_t wcnt;
	idx = 0;
	do {
		wcnt = write(fd, buff+idx, len - idx);
		if (wcnt == -1){
			perror("write");
		}
		idx += wcnt;
	} while(idx <  len);
}

void write_file (int fd, const char *infile)
{
	int fd2, oflags;
	oflags = O_RDWR | O_APPEND;
	fd2 = open(infile, oflags);
	if (fd == -1){
                perror("open");
                exit(1);
        }

	char buff2[1024];
	ssize_t rcnt;
	rcnt = read (fd2, buff2, sizeof(buff2)-1);
	if (rcnt == 0) /* end‐of‐file */
	if (rcnt == -1){ /* error */
	perror("read");
	}	
	buff2[rcnt] = '\0';
	doWrite(fd, buff2, strlen(buff2));
}

int main(int argc, char **argv)
{
int fd1,fd2,fd3;
char *output;

	if ((argc<=2) || (argc>=5))                /*ola ta pithana sfalmata */
	{
	printf("Usage: ./fconc infile1 infile2 [outfile (default:fconc.out)]\n");
	exit(1);
	}

fd1=open(argv[1],O_RDONLY);
	if (fd1==-1){
        printf("No such file or directory\n");
        exit(1);
        }

fd2=open(argv[2],O_RDONLY);
	if (fd2==-1){
        printf("No such file or directory\n");
        exit(1);
        }

	if (argc!=4) output="fconc.out";
	else output=argv[3];
fd3=open(output,O_WRONLY | O_CREAT |O_TRUNC , S_IRUSR | S_IWUSR);
write_file(fd3,argv[1]);
write_file(fd3,argv[2]);
}

