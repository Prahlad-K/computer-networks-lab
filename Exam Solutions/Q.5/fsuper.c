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

int nl=0,option=1;
int sfd[10];
char *port[10];
char *exec[10];

pid_t s[10];
int cnt=0;
char buffer[1024] = {0};
struct sockaddr_in addr,caddr;
int addrlen;

int thrv=0;          //thread variable to when to fork

void *fork_thr(void *arg)
{
  while(1)
  {
    if(thrv==1)
    {
    s[cnt-1]=fork();

    if(s[cnt-1]==0)
    {
      close(sfd[cnt-1]);
      char* ch[]={exec[cnt-1],NULL};
      execvp(ch[0],ch);
    }
    else
    thrv=0;
   }
  }
}
void fn1()
{
  char buffer[1024] = {0};
  char *po;
  char *ex;
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
 port[cnt-1]=po;
 exec[cnt-1]=ex;
 printf("%s %s  \n",port[cnt-1],exec[cnt-1]);
 addr.sin_family=AF_INET;
 addr.sin_port=htons(atoi(port[cnt-1]));
 addrlen=sizeof(addr);

 if(inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr)<=0)
 {
       printf("\nInvalid address/ Address not supported \n");
       return;
 }
 printf("%d , %d , %d \n",ntohs(addr.sin_port),addr.sin_family,addr.sin_addr.s_addr);

 if((sfd[cnt-1]=socket(AF_INET,SOCK_STREAM,0))==0)
 {
   printf("\n socket error \n");
   return;
 }
 if(setsockopt(sfd[cnt-1],SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR),(char*)&option,sizeof(option)) < 0)
  {
  printf("setsockopt failed\n");
  return;
  }
 if(bind(sfd[cnt-1],(struct sockaddr *)&addr,(socklen_t)addrlen)<0)
 {
   printf("\n bind error \n");
   return;
 }

 if(listen(sfd[cnt-1],1)<0)
 {
   printf("\n listen error \n");
   return;
 }
        printf("\n%s after listen\n",strerror(errno));

        thrv=1;
}


void main()
{
  pthread_t pt;
  int iret=pthread_create(&pt,NULL,fork_thr,NULL);

  printf("Process Id : %d\n",getpid());
  signal(SIGUSR1,fn1);


  int nsfd;
  fd_set rfd;
  while(1)
  {
    read(0,buffer,1024);
    struct timeval tv;
    tv.tv_sec=5;
    tv.tv_usec=0;

    FD_ZERO(&rfd);
    int maxfd=0;
    for(int i=0;i<cnt;i++)
    {
      FD_SET(sfd[i],&rfd);
      if(sfd[i]>maxfd)
      maxfd=sfd[i];
    }
    maxfd=maxfd+1;



    int val=select(maxfd,&rfd,NULL,NULL,&tv);
    if(val<0)
       {
         printf("\n %s in select %d\n",strerror(errno),val);
         return;
       }
    if(val>0)
    {
      for(int i=0;i<cnt;i++)
      {
      if(FD_ISSET(sfd[i],&rfd))
      {
        printf("\n%d %d\n",s[i],i);
        sleep(4);

       kill(s[i],SIGUSR1);
       printf("\n%s\n",strerror(errno));
       //read(0,buffer,1024);
      }
      }
    }
  }
}
