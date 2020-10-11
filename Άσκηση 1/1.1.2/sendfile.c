#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/sendfile.h>
void doWrite(int fd, const char *buff, int len)
{

}

void write_file(int fd, const char *infile)
{
int fd_infile;
struct stat stbuf;
fd_infile=open(infile,O_RDWR);  //mporei na thelei kai sketo O_RDONLY
if (fd_infile == -1){
	perror("open");
	exit(1);
	}
fstat(fd_infile, &stbuf);
sendfile(fd,fd_infile,0,stbuf.st_size);
}

int main(int argc, char **argv)
{
int fd1,fd2,fd3;
char *output;                                    

if (argc<=2)                     /*ola ta pithana sfalmata */
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
