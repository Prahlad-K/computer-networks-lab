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
#include <netinet/in.h>
#include <string.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<errno.h>
extern int errno;

char buffer[1024]={0};
int nsfd,sfd;
struct sockaddr_in addr,caddr;
int addrlen;

void *serv(void *s)
{
  int sfd=*((int *)s);
  write(1,"\n inside service s1 \n",15);
  char buffer[1024] = {0};
  read(sfd,buffer,1024);
  int l=strlen(buffer);
  sprintf(buffer,"s1:The string length is %d",l-1);
  send(sfd,buffer,strlen(buffer),0);
}

void fn1(int sig)
{
  write(1,"\ninside sig hand of s1\n",15);
  if(listen(sfd,10)<0)
  {
    printf("\n listen error \n");
    return;
  }
    write(1,"\nabove accept\n",15);
  if((nsfd=accept(sfd,(struct sockaddr *)&addr,(socklen_t *)&addrlen))<0)
  {
    printf("\n accept error \n)");
    return;
  }
  write(1,"\naccepted bro\n",15);
  pthread_t pt;
  int *nsfd1;
  nsfd1=&nsfd;
  int iret;
  iret=pthread_create(&pt,NULL,serv,(void *)nsfd1);

}
void main()
{
    int option=1;
  signal(SIGUSR1,fn1);
  addr.sin_family=AF_INET;
  addr.sin_port=htons(9190);
  addrlen=sizeof(addr);
  if(inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr)<=0)
  {
        printf("\nInvalid address/ Address not supported \n");
        return;
  }

  if((sfd=socket(AF_INET,SOCK_STREAM,0))<=0)
  {
    printf("\n socket error \n");
    return;
  }
  if(setsockopt(sfd,SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR),(char*)&option,sizeof(option)) < 0)
   {
     printf("\nsetsockopt failed\n");
     return;
   }

  if(bind(sfd,(struct sockaddr *)&addr,(socklen_t)addrlen)<0)
  {
    printf("\n bind error s1 \n");
    return;
  }
  printf("\nyou're connected to s1. My pid is %d \n",getpid());

  while(1);


}
