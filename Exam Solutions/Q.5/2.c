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
#include<errno.h>
extern int errno;
pid_t p;
void function()
{
    printf("\nfunction dtyjhdfg etg\n");
}
void main()
{
  printf("\nmain 20 PID : %d\n",getpid());
    signal(SIGUSR1,function);
    kill(getpid(),SIGUSR1);
    while(1);

}
