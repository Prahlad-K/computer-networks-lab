#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

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

	int sfd[argc-1];
	struct sockaddr_in serv_addr;
	struct hostent* server;
	if(argc<2)
	{
		printf("Format- %s Port Numbers\n", argv[0]);
		return 1;
	}
	for(int i=0;i<argc-1;i++)
	{
		sfd[i] = socket(AF_INET, SOCK_STREAM, 0);
		if (sfd[i] < 0) {
		      perror("ERROR opening socket");
		      exit(1);
		   }
		server = gethostbyname("127.0.0.1"); //either the name or IP address shall do
		if (server == NULL) {
		      fprintf(stderr,"ERROR, no such host\n");
		      exit(0);
		   }
		bzero((char*)&serv_addr, sizeof(serv_addr));
		//initialized to avoid garbage values.

		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(atoi(argv[i+1]));
		//server Port address.
		bcopy((char*)server->h_addr, (char*)&serv_addr.sin_addr.s_addr, server->h_length);
		//server IP address copied.

		if(connect(sfd[i], (struct sockaddr*)&serv_addr, sizeof(serv_addr))<0)
		{
		perror("ERROR connecting");
		exit(1);
		}
	}
	printf("Client is successfully connected!\n");

	char mes[100];
				
	while(1)
	{
		fd_set rfds;
		FD_SET(0, &rfds);
		int max = 1;
				
		for(int i=0;i<argc-1;i++)
		{
			FD_SET(sfd[i], &rfds);		
			if(sfd[i]>max)
			max = sfd[i];
		}

		struct timeval t;
		t.tv_sec=1;
		t.tv_usec=0;
	
		if(select(max+1, &rfds, NULL, NULL, &t)>0)		
		{	
			if(FD_ISSET(0, &rfds))
			{
				bzero((char*)mes, sizeof(*mes));

				fgets(mes, 99, stdin);
				
				int sno = 0;
				if(argc>2)
				{printf("Which server? ");
				scanf("%d", &sno);
				}
				
				int n = write(sfd[sno], mes, 99);
				if (n < 0) {
				      perror("ERROR writing from socket");
				      exit(1);
				   }

			}
			for(int i=0;i<argc-1;i++)
			{
				if(FD_ISSET(sfd[i], &rfds))
				{
					bzero((char*)mes, 99);
					int n = read(sfd[i], mes, 99);
					if (n < 0) {
					      perror("ERROR reading from socket");
					      exit(1);
					}
					if(n==0)
					{
						printf("Server %d closed!\n", i+1);
						exit(0);
					}
					printf("Server: %s\n", mes);
				}
			}
		}
	}

return 0;
}

