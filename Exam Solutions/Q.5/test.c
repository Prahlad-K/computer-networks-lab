#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/shm.h>
#include<sys/wait.h>
#include<sys/poll.h>
#include<sys/stat.h>
#include<semaphore.h>
#include<fcntl.h>
#include<sys/msg.h>
#include <netinet/in.h>
#include <string.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<pthread.h>

int main()
{
int cnt=0;
char *po;
char *ex;
char *port[10];
char buffer[1024] = {0};
int fd=open("inetd.conf",O_RDWR,0666);
if(fd<0)
printf("\n fileopen failed \n");

read(fd,buffer,1024);
po=strtok(buffer,"|");
int i=0;

while (1)
{
     i++;
     po = strtok(NULL,"|");
     ex = strtok(NULL,"|");
     if(cnt+1==i)
     {
       cnt++;
       break;
     }
 }
 port[0]=po;
 port[1]=ex;
 printf("%s %s wef \n",port[0],port[1]);
}
