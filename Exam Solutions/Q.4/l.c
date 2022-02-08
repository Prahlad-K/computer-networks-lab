#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

#include <strings.h>
#include <string.h>

#include <sys/select.h>
#include <sys/types.h>
#include <signal.h>

#include <pthread.h>
#include <poll.h>
#include <sys/msg.h>
#include <fcntl.h>

int main()
{

struct sockaddr_in serv_addr, cli_addr;
int sfd = socket(AF_INET, SOCK_STREAM, 0);
if (sfd < 0) 
{
      	perror("ERROR opening socket");
      	exit(1);
}

bzero((char*)&serv_addr, sizeof(serv_addr));
serv_addr.sin_family = AF_INET;
serv_addr.sin_port = htons(7001);
serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

if(bind(sfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0)
{
perror("ERROR binding\n");
}

listen(sfd, 5);

int clilen;
int nsfd = accept(sfd, (struct sockaddr *)&cli_addr, &clilen);

char* telecast = malloc(100);
while(1)
{
printf("Enter live telecast: ");
fgets(telecast, 100, stdin);
write(nsfd, telecast, strlen(telecast));
}

}
