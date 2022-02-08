#include<stdio.h>
#include<arpa/inet.h>
#include<string.h>
#include<stdlib.h>
#include<pcap.h>
#include<time.h>
#include<unistd.h>
#include<netinet/in.h>
#include<netinet/ip.h>
#include<netinet/udp.h>
#include<netinet/tcp.h>
#include<netinet/if_ether.h>
#include<net/ethernet.h>

#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <net/if.h>

int main(int argc,char **argv)
{
	char *device;
	char error[PCAP_ERRBUF_SIZE];
	bpf_u_int32 ip_raw;
	bpf_u_int32 subnet_mask_raw;
	
	device=pcap_lookupdev(error);
		
	if(device==NULL)
	{
		perror("pcap_lookupdev");
		exit(1);
	}
	else
	{
		printf("Device found-%s\n",device);
	}
	if(pcap_lookupnet(device,&ip_raw,&subnet_mask_raw,error)<0)
	{
		perror("pcap_lookup");
		exit(1);
	}
	struct in_addr	addr;
	char ip[13],subnet[13];
	addr.s_addr=ip_raw;	
	strcpy(ip,inet_ntoa(addr));	
	addr.s_addr=subnet_mask_raw;
	strcpy(subnet,inet_ntoa(addr));	
	printf("Ip found -%s\n",ip);
	printf("Subnet found -%s\n",subnet);
	
	pcap_t *handle;
	const u_char *packet;
	struct pcap_pkthdr packet_header;
	int packet_limit_count=1;//0 for infinite
	int timeout_limit=1000;
	
	handle=pcap_open_live(device,BUFSIZ,packet_limit_count,timeout_limit,error);			
	if(handle==NULL)
	{
		perror("pcap_open_live");
		exit(1);
	}
	while(1)
	{
		packet=pcap_next(handle,&packet_header);
		char vin[INET_ADDRSTRLEN];
		struct iphdr *iph=(struct iphdr*)(packet+14);
		if(packet==NULL)
		printf("error\n");
		else
		{
//			printf("Packet capture length-%d\n",packet_header.caplen);
//			printf("Packet total length-%d\n",packet_header.len);
//			printf("Ethernet address length is %d\n",ETHER_HDR_LEN);
			
				
			char str1[1000];
			strcpy(str1,inet_ntop(AF_INET,&iph->saddr,vin,INET_ADDRSTRLEN));
			if(strcmp(str1, "192.168.192.3"))
			continue;
			
			printf("************************Packet**********************************\n");	
			printf("Captured at ::%s\n",ctime((const time_t*)&packet_header.ts.tv_sec));
			

		}
		
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
			continue;
		}			
		u_char *ptr;
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
			int len=packet_header.caplen-n1-14-(iph->ihl*4);
			while(len--)
			{
				printf("%c",*msg++);
			}
			printf("\n");
		}
		else
		{
			printf("some other protocol-%d",iph->protocol);
		}		
		printf("\n\n");
	}
	return 0;
}
