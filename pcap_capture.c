#include<stdio.h>	//for printf
#include<string.h> //memset
#include<sys/socket.h>	//for socket ofcourse
#include<stdlib.h> //for exit(0);
#include<errno.h> //For errno - the error number
#include<netinet/tcp.h>
#include<netinet/udp.h>	//Provides declarations for tcp header
#include<netinet/ip.h>	//Provides declarations for ip header
#include<arpa/inet.h>
#include<pthread.h>
#include<pcap.h>
#include<unistd.h>
#include <netinet/if_ether.h>



void raw_packet_receiver(u_char *udata, const struct pcap_pkthdr *pkthdr, const u_char *packet)
{	int Size = pkthdr->len;
	struct iphdr *ip;
	struct udphdr *udp;
	struct ether_header *eth;
	u_char *ptr;
	int l1_len = (int)udata;
	int s_seq;
/*
	ip = (struct iphdr *)(packet + l1_len);
	udp = (struct udphdr *)(packet + l1_len + (ip->ihl)*4);

	printf("%d\n", l1_len);
    printf("%d\n",(int)sizeof(struct udphdr));
	int header_size = l1_len +  (ip->ihl)*4 + sizeof(struct udphdr);
    u_char* payload = (u_char*)(packet + l1_len +  (ip->ihl)*4 + sizeof(struct udphdr));
	int i;
    for(i=0;i<Size-header_size;i++){
		printf("%c",(char)payload[i]);
	}
	printf("\n");

	eth = (struct ether_header*)packet;
*/
	char vin[INET_ADDRSTRLEN];
	struct iphdr *iph=(struct iphdr*)(packet+14);
	
int header_size = l1_len +  (ip->ihl)*4 + sizeof(struct udphdr);
	printf("\t\t[Ethernet Header]\n");
	printf("________________________________________________\n");
	struct ether_header *eptr;	
	eptr=(struct ether_header*)packet;
	if(ntohs(eptr->ether_type)==ETHERTYPE_IP)
	{
		printf("Ethernet type hex:%x decm:%d is an IP packet\n",ntohs(eptr->ether_type),ntohs(eptr->ether_type));
	}
	else if(ntohs(eptr->ether_type)==ETHERTYPE_ARP)
	{
		printf("Ethernet type hex:%x decm:%d is an ARP packet\n",ntohs(eptr->ether_type),ntohs(eptr->ether_type));	
		struct arphdr *arph=(struct arphdr*)(packet+14);
		printf("\n\noperation code-%x\n\n",ntohs(arph->ar_op));
	}
	else
	{
		printf("Ethernet type hex:%x is not IP",ntohs(eptr->ether_type));
		
	}			
	//u_char *ptr;
	int i=ETHER_ADDR_LEN;
	ptr=eptr->ether_dhost;	
	printf("Destination Address:");
	do
	{
		printf("%s%x",(i==ETHER_ADDR_LEN)?" ":":",*ptr++);
	}while(--i>0);
	printf("\n");
	
	i=ETHER_ADDR_LEN;
	ptr=eptr->ether_shost;
	printf("Source Address:");		
	
	do
	{
		printf("%s%x",(i==ETHER_ADDR_LEN)?" ":":",*ptr++);
	}while(--i>0);
	printf("\n");
	
	printf("\t\t[IP Header]\n");
	printf("________________________________________________\n");
//6+6+2
	
	printf("version:%d\n",iph->version);
	printf("ip-header length:%d\n",iph->ihl);
	printf("service-no:%d\n",iph->tos);
	printf("total-len:%d\n",iph->tot_len);
	printf("packet-id:%d\n",iph->id);
	printf("fragment-offset:%d\n",iph->frag_off);
	printf("time-to-live:%d\n",iph->ttl);
	printf("protocol:%d\n",iph->protocol);
	printf("Source Ip:%s\n",inet_ntop(AF_INET,&iph->saddr,vin,INET_ADDRSTRLEN));
	printf("Destination Ip:%s\n",inet_ntop(AF_INET,&iph->daddr,vin,INET_ADDRSTRLEN));

	if(iph->protocol==17)
	{
		struct 	udphdr *udp=(struct udphdr*)(packet+14+iph->ihl*4);
		printf("\t\t[UDP Header]\n");
		printf("________________________________________________\n");
		printf("Source port:%d\n",udp->source);
		printf("Destination port:%d\n",udp->dest);
		printf("Lenght:%d\n",udp->len);
		printf("Checksum:%d\n",udp->check);
		
		printf("\t\t[Data]\n");			
		printf("________________________________________________\n\n");
		int n=udp->len-8;
		const u_char *msg=(packet+14+iph->ihl*4+8);
		while(n--)
		{
			printf("%c",*msg++);
		}
		printf("\n");
	}
	else if(iph->protocol==6)
	{
		struct tcphdr *tcp=(struct tcphdr*)(packet+14+iph->ihl*4);
		printf("\t\t[TCP PACKET]\n");
		printf("________________________________________________\n");
		printf("Source port:%d\n",tcp->source);
		printf("Destination port:%d\n",tcp->dest);
		printf("Sequence NUmber:%d\n",tcp->seq);
		printf("Ack_Number:%d\n",tcp->ack_seq);
		printf("Data offest:%d\n",tcp->doff);
		printf("Window:%d\n",tcp->window);
		printf("Checksum:%d\n",tcp->check);
		
		printf("\t\t[Data]\n");
		printf("________________________________________________\n\n");
		int n1=tcp->doff;
		const u_char *msg=(packet+14+iph->ihl*4+n1);
		int len=Size-header_size;
		while(len--)
		{
			printf("%c",*msg++);
		}
		printf("\n");
	}
	else if(iph->protocol==100)
	{
		printf("Received a packet from client!\n");
		char* ip = malloc(100);
		ip = (char*)(packet+14+iph->ihl*4);	
	}		
	printf("\n\n");
	/*
	char* interface = malloc(10);
	struct sockaddr_ll device;
	strcpy(interface, "wlp3s0");
        memset (&device, 0, sizeof (device));
	if ((device.sll_ifindex = if_nametoindex (interface)) == 0)
	{
		perror ("if_nametoindex() failed to obtain interface index ");
		exit (EXIT_FAILURE);
	}
	printf ("Index for interface %s is %i\n", interface, device.sll_ifindex);
        device.sll_family = AF_PACKET;
 	memcpy (device.sll_addr, eth->ether_dhost, 6 * sizeof (uint8_t));
	device.sll_halen = 6;

	//eth->ether_shost[0] = 0x
	eth->ether_shost[0] = 0x44;
	eth->ether_shost[1] = 0x1C;
	eth->ether_shost[2] = 0xA8;
	eth->ether_shost[3] = 0xA6;
	eth->ether_shost[4] = 0xEF;
	eth->ether_shost[5] = 0x9B;


/*
	pcap_t* pd1;
	char *dev = "wlp3s0";
	char errbuf[PCAP_ERRBUF_SIZE];
	if ((pd1 = pcap_open_live(dev, 1514, 1, 500, errbuf)) == NULL) {
		fprintf(stderr, "cannot open device %s: %s\n", dev, errbuf);
		exit(1);
	}
	

	if(pcap_inject(pd1,packet,Size)<=0)
	{
		perror("Could not send");
	}
*/

	//int psfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	//sendto(psfd, packet, Size, 0, (struct sockaddr*)&device, sizeof(device));

	
    sleep(1);
}

void *capture_run(void *arg)
{
	pcap_t *pd;
	char *filter = "dst host 192.168.43.16 and ip";
	char *dev = "wlp3s0";
	char errbuf[PCAP_ERRBUF_SIZE];
	bpf_u_int32	netp;
	bpf_u_int32	maskp;
	struct bpf_program	fprog;					/* Filter Program	*/
	int dl = 0, dl_len = 0;

	if ((pd = pcap_open_live(dev, 1514, 1, 500, errbuf)) == NULL) {
		fprintf(stderr, "cannot open device %s: %s\n", dev, errbuf);
		exit(1);
	}

	pcap_lookupnet(dev, &netp, &maskp, errbuf);
	pcap_compile(pd, &fprog, filter, 0, netp);
	if (pcap_setfilter(pd, &fprog) == -1) {
		fprintf(stderr, "cannot set pcap filter %s: %s\n", filter, errbuf);
		exit(1);
	}
	pcap_freecode(&fprog);
	dl = pcap_datalink(pd);
	
	switch(dl){
		case 1:
			dl_len = 14;
			break;
		default:
			dl_len = 14;
			break;
	}

	if (pcap_loop(pd, -1, raw_packet_receiver, (u_char *)dl_len) < 0) {
		fprintf(stderr, "cannot get raw packet: %s\n", pcap_geterr(pd));
		exit(1);
	}
}


int main(){
    capture_run(NULL);
    return 0;
}
