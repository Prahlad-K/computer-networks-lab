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
void *serv(void *s)
{
    p = fork();
    if(p == 0)
    {
      char* ch[]={"./2",NULL};
      execvp(ch[0],ch);
    }

}

void fn1()
{
  pthread_t pt;
  int *nsfd;
  int iret;
  iret=pthread_create(&pt,NULL,serv,(void *)nsfd);
}
void main()
{
    signal(SIGUSR1,fn1);
        kill(getpid(),SIGUSR1);

    sleep(2);
      printf("\n %d \n",p );
    //kill(p,SIGUSR1);
    printf("%s\n",strerror(errno) );
    printf("sdfghjkl\n" );

}
