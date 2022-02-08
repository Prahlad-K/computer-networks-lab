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
	int nsfd = *(int*)args;

	char mes[100];

	while(1)
	{
		bzero(mes, 99);
		int n = read(nsfd, mes, 99);
		if (n < 0) {
		      perror("ERROR reading from socket");
		      exit(1);
		   }
		if(n==0)
		{
			printf("Thread closing!\n");
			pthread_exit(0);
		}
		printf("Client: %s\n", mes);

		bzero(mes, 99);
		strcpy(mes, "Got your message, Client!");
		n = write(nsfd, mes, 99);
		if (n < 0) {
		      perror("ERROR writing from socket");
		      exit(1);
		}
		
	}
}

int main(int argc, char** argv)
{

	struct sockaddr_in serv_addr, cli_addr;

	int sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd < 0) {
	      perror("ERROR opening socket");
	      exit(1);
	   }

	bzero((char*)&serv_addr, sizeof(serv_addr));
	//initialized to avoid garbage values.

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(7227);
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //localhost, loopback address
	//local IP, local port

	int opt=1;
	setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

	if(bind(sfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0)
	{
	perror("ERROR in bind\n");
	}

	int ssfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd < 0) {
	      perror("ERROR opening socket");
	      exit(1);
	   }

	bzero((char*)&serv_addr, sizeof(serv_addr));
	//initialized to avoid garbage values.

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(7228); //want to connect to the alternate server
	serv_addr.sin_addr.s_addr= inet_addr("127.0.0.1");
	
	if(connect(ssfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))<0)
	{
	perror("ERROR connecting");
	exit(1);
	}

	pthread_t tid[100];
	int tno = 0;
	
	printf("Listening now on port 7227\n");
	listen(sfd, 5);
		
	int clilen = sizeof(cli_addr);

	char* buf = malloc(100);

	int ports[100];
	int nsfds[100];
	int noc = 0;

	int down = 0;

	while(1)
	{
		fd_set readset;
		FD_ZERO(&readset);
		
		int max=0;
		FD_SET(0, &readset);

		if(!down)
		{
			FD_SET(sfd, &readset);
			
			if(sfd>max)
			max = sfd;
		}
		struct timeval t;
		t.tv_sec=1;
		t.tv_usec=0;
		int rv = select(max + 1, &readset, NULL, NULL, &t);

		if(rv<0)
		perror("ERROR in select");
		if(rv>0)
		{		
			if(!down && FD_ISSET(sfd, &readset))
			{		
				int nsfd = accept(sfd, (struct sockaddr *)&cli_addr, &clilen);
				if (nsfd < 0) {
				      perror("ERROR on accept");
				      exit(1);
				   }
				
				ports[noc] = ntohs(cli_addr.sin_port);
				nsfds[noc++] = nsfd;
				printf("Port of client %d identified as %d\n", noc-1, ports[noc-1]);

		 		if(pthread_create(&tid[tno++],NULL,(void*)&do_thread_service, (void*)&nsfd)!=0)
				perror("ERROR in pthread_create\n");
			}
			
			if(FD_ISSET(0, &readset))
			{
				fgets(buf, 100, stdin);
				if(strcmp(buf, "busy\n")==0)
				{
					printf("Server going to be busy!\n");
					down = 1;
					char* port = malloc(10);					
					for(int i=0;i<noc;i++)
					{
						sprintf(port, "%d", ports[i]);
						printf("Port %s\n", port);
						if(send(nsfds[i], buf, strlen(buf), MSG_CONFIRM)<0)
						perror("ERROR in send");
					}
					
					printf("Waiting for client to prepare...\n");
					sleep(1);

					if(send(ssfd, buf, strlen(buf), MSG_CONFIRM)<0)
					printf("ERROR in send");
					else
					printf("Sent %s", buf);

					for(int i=0;i<noc;i++)
					{
						memset(port, '\0', strlen(port));
						sprintf(port, "%d", ports[i]);
						if(send(ssfd, port, strlen(port), MSG_CONFIRM)<0)
						perror("ERROR in send");	
						else
						printf("Sent %s\n", port);
					}
					memset(buf, '\0', strlen(buf));
					strcpy(buf, "done");
					if(send(ssfd, buf, strlen(buf), MSG_CONFIRM)<0)
					perror("ERROR in send");
					else
					printf("Sent %s\n", buf);
				}
				if(strcmp(buf, "back\n")==0)
				{
					printf("Server back!\n");
					down = 0;
					noc = 0; //a fresh start
					if(send(ssfd, buf, strlen(buf), MSG_CONFIRM)<0)
					printf("ERROR in send");
					else
					printf("Sent %s", buf);
				}
			}
		}
	}
	return 0;
}
