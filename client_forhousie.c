#include<time.h>
#include<stdio.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<sys/select.h>
#include<pthread.h>
#include<signal.h>
#include<stdlib.h>
#include<fcntl.h>
#include<sys/shm.h>
#include<unistd.h>
#include<sys/un.h>
#include<netinet/ip.h>
#include<arpa/inet.h>
#include<pcap.h>
#include<errno.h>
#include<netinet/if_ether.h>
#include<net/ethernet.h>
#include<netinet/ether.h>
#include<netinet/udp.h>


int main(int argc, char** argv)
{
	
	if(argc!=2)
	printf("\n usage ./a.out Server IP Server port_no Your protocol number");

	int sfd;
	struct sockaddr_in serv_addr;
	int port_no=atoi(argv[2]);

	bzero(&serv_addr,sizeof(serv_addr));

	if((sfd = socket(AF_INET , SOCK_STREAM , 0))==-1)
	perror("\n socket");
	else printf("\n socket created successfully\n");
	
	struct sockaddr_in myaddr;
	myaddr.sin_addr.s_addr = inet_addr("127.0.0.2");
	myaddr.sin_port = htons(8227);
		
	bind(sfd, (struct sockaddr*)&myaddr, sizeof(myaddr));	//bound on IP2

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port_no);
	//serv_addr.sin_addr.s_addr = INADDR_ANY;
	inet_pton(AF_INET,argv[1], &serv_addr.sin_addr);

	if(connect(sfd , (struct sockaddr *)&serv_addr , sizeof(serv_addr))==-1)
	perror("\n connect : ");
	else printf("\nconnect successful");


	char* msg = malloc(100);
	if(recv(sfd, msg, 100, 0)<0)
	perror("ERROR in recv");
	else
	printf("Received tokens %s\n", msg);
	
	char* nums[5];
	for(int i=0;i<5;i++)
	nums[i] = malloc(100);

	int j =0;
	int ctr = 0;
	for(int i=0;i<strlen(msg);i++)
	{
		if(msg[i]!='-')
		{
			nums[j][ctr] = msg[i];
			ctr++;
		}
		else
		{
			nums[j][ctr] = '\0';
			printf("Number %d is %s\n", j+1, nums[j]);
			j++;
			ctr = 0;
		}	
		
	}	
	
	int rsfd = socket(AF_INET, SOCK_RAW, 10); //protocol number

	int optval=1;
	setsockopt(rsfd, IPPROTO_IP, SO_BROADCAST, &optval, sizeof(int));//IP_HDRINCL

	myaddr.sin_addr.s_addr = inet_addr("127.0.0.2");
	
	bind(rsfd, (struct sockaddr*)&myaddr, sizeof(myaddr));
	//now bound to 127.0.0.2

	struct sockaddr_in recv_addr;
	recv_addr.sin_family = AF_INET;
	recv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	
	
	int matched =0;
	while(1)
	{
		memset(msg, '\0', strlen(msg));
		socklen_t len = sizeof(recv_addr);
		recvfrom(rsfd, msg, 100, 0, (struct sockaddr*)&recv_addr, &len);
		perror("Recvfrom: ");

		struct iphdr* iph = (struct iphdr*)msg;

		char* number = malloc(10);
		strcpy(number, (char*)(msg+(iph->ihl*4)));
		printf("Received number: %s\n", number);

		for(int i=0;i<5;i++)
		{
			if(strcmp(number, nums[i])==0)
			{
				matched++;
				printf("Matched!!\n");
			}
		}
	
		if(matched==5)
		break;
				
		sleep(1);
	}

	
	return 0;
}
