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

int main(int argv,char* argc[])
{
  int sfd;
  char buffer[1024] = {0};
  struct sockaddr_in addr,saddr,caddr;

    saddr.sin_family=AF_INET;
    saddr.sin_port=htons(atoi(argc[1]));
    if(inet_pton(AF_INET, "127.0.0.1", &saddr.sin_addr)<=0)
      {
          printf("\nInvalid address/ Address not supported \n");
          return 0;
      }
  if((sfd=socket(AF_INET,SOCK_STREAM,0))==0)
  {
    printf("\n socket error \n)");
    return 0;
  }

  if(connect(sfd,(struct sockaddr *)&saddr,(socklen_t)sizeof(saddr))<0)
  {
    printf("\nConnection Failed \n");
    return 0;
  }

    int len=sizeof(caddr);
    getsockname(sfd,(struct sockaddr *)&caddr,(socklen_t *)&len);
    printf("%d , %d , %d ",caddr.sin_port,caddr.sin_family,caddr.sin_addr.s_addr);

    printf("Enter your string \n");

    read(0,buffer,1024);
    send(sfd , buffer , strlen(buffer) , 0 );
    read( sfd , buffer, 1024);
    printf("%s\n",buffer );

}
