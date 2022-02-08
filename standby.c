#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

#include <strings.h>
#include <string.h>

#include<sys/un.h>
#include<netinet/ip.h>
#include<errno.h>
#include <pthread.h>

void do_thread_service(void* args)
{
	int sfd = *(int*)args;

	char mes[100];

	while(1)
	{
		bzero(mes, 99);
		int n = read(sfd, mes, 99);
		if (n < 0) {
		      perror("ERROR reading from socket");
		      exit(1);
		   }
		if(n==0)
		{
			printf("Thread Closing!\n");
			pthread_exit(0);
		}
		printf("Client: %s\n", mes);

		bzero(mes, 99);
		strcpy(mes, "Got your message, Client!");
		n = write(sfd, mes, 99);
		if (n < 0) {
		      perror("ERROR writing from socket");
		      exit(1);
		}

	}
}

int main(int argc, char** argv)
{

	struct sockaddr_in serv_addr, cli_addr;

	int ssfd = socket(AF_INET, SOCK_STREAM, 0);
	if (ssfd < 0) {
	      perror("ERROR opening socket");
	      exit(1);
	   }

	bzero((char*)&serv_addr, sizeof(serv_addr));
	//initialized to avoid garbage values.

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(7228);
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //localhost, loopback address
	//local IP, local port

	int opt=1;
	setsockopt(ssfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

	if(bind(ssfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0)
	{
	perror("ERROR in bind\n");
	}
	
	listen(ssfd, 5);
	
	printf("Listening now on port 7228\n");
	int clilen = sizeof(cli_addr);
	int nsfd = accept(ssfd, (struct sockaddr *)&cli_addr, &clilen);
	if (nsfd < 0) {
	      perror("ERROR on accept");
	      exit(1);
	}
	printf("Connected to the server!\n");
	
	int ports[100];
	int noc = 0;

	int sfd[100];
	pthread_t tid[100];
	int tno = 0;
	char* buf = malloc(100);
		
	while(1)
	{
		memset(buf, '\0', 100);
		if(recv(nsfd, buf, 5, MSG_WAITALL)<0)
		perror("ERROR in recv");
		else
		printf("Received %s\n", buf);
		
		if(strcmp(buf, "busy\n")==0)
		{
			printf("Received a message stating that the server is busy!\n");
			while(1)
			{
				char* port = malloc(10);
				if(recv(nsfd, port, 4, MSG_WAITALL)<0)
				perror("ERROR in recv");
					
				if(strcmp(port, "done")==0)
				break;
				else
				{
					ports[noc++] = atoi(port);
					if(ports[noc-1]==0)
					break;
					printf("Received port %d\n", ports[noc-1]);
				}
			}
			for(int i=0;i<noc;i++)
			{
				sfd[i] = socket(AF_INET, SOCK_STREAM, 0);
				if (sfd[i] < 0) {
				      perror("ERROR opening socket");
				      exit(1);
				   }
				struct hostent* server = gethostbyname("127.0.0.1");
				if (server == NULL) {
				      fprintf(stderr,"ERROR, no such host\n");
				      exit(0);
				   }
				bzero((char*)&serv_addr, sizeof(serv_addr));
				//initialized to avoid garbage values.

				serv_addr.sin_family = AF_INET;
				serv_addr.sin_port = htons(ports[i]);

				//server Port address.
				bcopy((char*)server->h_addr, (char*)&serv_addr.sin_addr.s_addr, server->h_length);
				//server IP address copied.

				if(connect(sfd[i], (struct sockaddr*)&serv_addr, sizeof(serv_addr))<0)
				{
				perror("ERROR connecting");
				exit(1);
				}
				printf("Connected to port %d\n", ports[i]);	

				if(pthread_create(&tid[tno++],NULL,(void*)&do_thread_service, (void*)&sfd[i])!=0)
				perror("ERROR in pthread_create\n");
			}		
		}
		if(strcmp(buf, "back\n")==0)
		{
			for(int i=0;i<noc;i++)
			{
				if(send(sfd[i], buf, strlen(buf), MSG_CONFIRM)<0)
				perror("ERROR in send");	
				else
				printf("Sent %s to Client %d\n", buf, i);
			}
			noc = 0;
			//it's restored
		}
	}
	return 0;
}

