#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

#include <string.h>
#include <strings.h>

/*
struct sockaddr_in {
   short int            sin_family; //usually AF_INET
   unsigned short int   sin_port; //port number, must use htons to convert from host byte ordering to network byte ordering.
   struct in_addr       sin_addr; //different structure to hold the IP address
   unsigned char        sin_zero[8];//not used
};

struct in_addr {
   unsigned long s_addr;
};

struct hostent { //host entries
   char *h_name; //like google.com	
   char **h_aliases; //other aliases for this IP
   int h_addrtype; // usually AF_INET 
   int h_length; //4 for internet addresses (I guess it means 4 bytes = 4* 8 = 32 bits.)
   char **h_addr_list;
	
#define h_addr  h_addr_list[0]
};
*/

int main(int argc, char** argv)
{

	if(argc<2)
	{
	printf("Format- %s Port Number\n", argv[0]);
	return 1;
	}
	int sfd;

	struct sockaddr_in serv_addr;
	struct sockaddr_in cli_addr;
	int clilen = sizeof(cli_addr);
	restart:
	sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd < 0) {
	      perror("ERROR opening socket");
	      exit(1);
	   }

	bzero((char*)&serv_addr, sizeof(serv_addr));
	//initialized to avoid garbage values.

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[1]));
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
	//local IP, local port

	int opt=1;
	if(setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))<0)
	perror("ERROR in setsockopt");

	if(bind(sfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0)
	{
	perror("ERROR in bind\n");
	exit(0);
	}
	//binding the client on the port number provided
	
	bzero((char*)&serv_addr, sizeof(serv_addr));
	//initialized to avoid garbage values.

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(7227); //connect to server
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if(connect(sfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))<0)
	{
	perror("ERROR connecting");
	exit(1);
	}

//	if(getsockname(sfd, (struct sockaddr*)&cli_addr, &clilen)<0)
//	perror("ERROR in getsockname");

//	printf("Client's port number: %d\n", ntohs(cli_addr.sin_port));

	char mes[100];
		
	int down = 0;
	int nsfd;
	while(1)
	{
		if(!down)
		{
			fd_set readset;
			int max = 0;
			FD_ZERO(&readset);
			FD_SET(0, &readset);
			FD_SET(sfd, &readset);
			
			if(sfd>max)
			max = sfd;

			struct timeval t;
			t.tv_sec=1;
			t.tv_usec=0;
			int rv = select(max + 1, &readset, NULL, NULL, &t);

			if(rv<0)
			perror("ERROR in select");
		
			if(rv>0)
			{
				if(FD_ISSET(0, &readset))
				{
					bzero((char*)mes, sizeof(*mes));

					fgets(mes, 99, stdin);

					int n = write(sfd, mes, 99);
					if (n < 0) {
					      perror("ERROR writing from socket");
					      exit(1);
					   }
				}
				
				if(FD_ISSET(sfd, &readset))
				{
					bzero((char*)mes, 99);
					int n = read(sfd, mes, 99);
					if (n < 0) {
					      perror("ERROR reading from socket");
					      exit(1);
					   }
					else if(n==0)
					{
						printf("Server has closed!\n");
						exit(0);
					}
					else
					{
						if(strcmp(mes, "busy\n")==0)
						{
							down = 1;
							printf("Main server is busy with maintenence!\n");
							close(sfd);
							sfd = socket(AF_INET, SOCK_STREAM, 0);
							if (sfd < 0) {
							      perror("ERROR opening socket");
							      exit(1);
							   }

							bzero((char*)&serv_addr, sizeof(serv_addr));
							//initialized to avoid garbage values.

							serv_addr.sin_family = AF_INET;
							serv_addr.sin_port = htons(atoi(argv[1]));
							serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //localhost, loopback address
							//local IP, local port

							int opt=1;
							if(setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))<0)
							perror("ERROR in setsockopt");

							if(bind(sfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0)
							{
							perror("ERROR in bind\n");
							exit(0);
							}
							
							if(listen(sfd, 5)<0)
							perror("ERROR in listen");

							printf("Listening now on port %d\n", atoi(argv[1]));

							nsfd = accept(sfd, (struct sockaddr *)&cli_addr, &clilen);
							if (nsfd < 0) {
							      perror("ERROR on accept");
							      exit(1);
							}
							else
							printf("Connected to alternate server!\n");
						}
						else
							printf("Server: %s\n", mes);
					}
				}
			}
		}
		if(down)
		{

			fd_set readset;
			int max = 0;
			FD_ZERO(&readset);
			FD_SET(0, &readset);
			FD_SET(nsfd, &readset);
			
			if(nsfd>max)
			max = nsfd;

			struct timeval t;
			t.tv_sec=1;
			t.tv_usec=0;
			int rv = select(max + 1, &readset, NULL, NULL, &t);

			if(rv<0)
			perror("ERROR in select");
		
			if(rv>0)
			{
				if(FD_ISSET(0, &readset))
				{
					bzero((char*)mes, sizeof(*mes));

					fgets(mes, 99, stdin);

					int n = write(nsfd, mes, 99);
					if (n < 0) {
					      perror("ERROR writing from socket");
					      exit(1);
					   }
				}
				
				if(FD_ISSET(nsfd, &readset))
				{
					bzero((char*)mes, 99);
					int n = read(nsfd, mes, 99);
					if (n < 0) {
					      perror("ERROR reading from socket");
					      exit(1);
					   }
					else if(n==0)
					{
						printf("Server has closed!\n");
						exit(0);
					}
					else
					{
						if(strcmp(mes, "back\n")==0)
						{
							down = 0;
							printf("Main server is back and ready!\n");
							close(nsfd);
							close(sfd);
							goto restart;
						}
						else
							printf("Server: %s\n", mes);
					}
				}
			}
		}
	}
	return 0;
}
