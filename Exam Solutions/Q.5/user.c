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

int main(int argv,char* argc[])
{
  char buffer[1024]={0};
  int p;
  int fd=open("inetd.conf",O_RDWR | O_APPEND,0666);
  if(fd<0)
  printf("\n fileopen failed \n");

  while(1)
  {
    printf("\nInsert a new service as Ex: 9090|./s1|\n");
    // read(0,buffer,1024);
    scanf("%d",&p);
    //write(fd,buffer,strlen(buffer));
    kill((pid_t)atoi(argc[1]),SIGUSR1);
  }
}
