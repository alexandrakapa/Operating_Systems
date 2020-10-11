#include <unistd.h>
#include <stdio.h>
void zing()
{
char *lgn;
lgn=getlogin();
printf("this is %s\n",lgn);
}


