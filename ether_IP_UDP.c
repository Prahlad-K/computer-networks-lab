#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <netinet/if_ether.h>
#include <signal.h>
#include <errno.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/if_ether.h>
#include <netinet/ip_icmp.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

unsigned short csum(unsigned short *ptr,int nbytes) 
{
	register long sum;
	unsigned short oddbyte;
	register short answer;

	sum=0;
	while(nbytes>1) {
		sum+=*ptr++;
		nbytes-=2;
	}
	if(nbytes==1) {
		oddbyte=0;
		*((u_char*)&oddbyte)=*(u_char*)ptr;
		sum+=oddbyte;
	}

	sum = (sum>>16)+(sum & 0xffff);
	sum = sum + (sum>>16);
	answer=(short)~sum;
	
	return(answer);
}


struct pseudo_header { //Needed for checksum calculation
    unsigned int source_address;
    unsigned int dest_address;
    unsigned char placeholder;
    unsigned char protocol;
    unsigned short udp_length;
};

int main(int argc,char* argv[]){
	int tx_len = 0;

    int sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL)); //This is the main socket to send the SYN packet
    if (sockfd < 0){
        printf("Error creating socket. Error number: %d. Error message: %s\n", errno, strerror(errno));
        exit(1);
    }
    char datagram[4096];
  
    //ether_header---------------------------------------------------
    struct ether_header *eh = (struct ether_header*) datagram;
  // 44:1c:a8:a6:ef:9b
    eh->ether_shost[0] = 0x44;
    eh->ether_shost[1] = 0x1c;
    eh->ether_shost[2] = 0xa8;
    eh->ether_shost[3] = 0xa6;
    eh->ether_shost[4] = 0xef;
    eh->ether_shost[5] = 0x9b;

    eh->ether_dhost[0] = 0x44;
    eh->ether_dhost[1] = 0x1c;
    eh->ether_dhost[2] = 0xa8;
    eh->ether_dhost[3] = 0xa6;
    eh->ether_dhost[4] = 0xef;
    eh->ether_dhost[5] = 0x9b;

    eh->ether_type = htons(ETH_P_IP);
    tx_len += sizeof(struct ether_header);

    struct iphdr* iph = (struct iphdr*)(datagram + sizeof(struct ether_header));
    //Fill in the IP Header
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    //iph->tot_len = sizeof(struct iphdr) + sizeof(struct udphdr) + strlen(msg_buff);
    iph->id = htonl(46156); //Id of this packet
    iph->frag_off = htons(16384);
    iph->ttl = 64;
    iph->protocol = IPPROTO_UDP;
    iph->saddr = inet_addr("192.168.43.123"); //Spoof the source ip address
    iph->daddr = inet_addr("192.168.43.123");
    //iph->check = check_sum((unsigned short*)datagram, iph->tot_len);
    tx_len += sizeof(struct iphdr);

    struct udphdr* udph = (struct udphdr*)(datagram+ tx_len);
    //udp
    udph->source = htons(46156);
    udph->dest = htons(8080);
    //udph->len = htons(sizeof(struct udphdr) + strlen(msg_buff));
    udph->check = 0;
    tx_len += sizeof(struct udphdr);

    //data-----------------------------------------------------------
    char* buff = (datagram + tx_len);
    strcpy(buff,"hello from GW");
    tx_len += strlen(buff);

    udph->len = htons(tx_len - sizeof(struct ether_header) - sizeof(struct iphdr));
    iph->tot_len = htons(tx_len - sizeof(struct ether_header));

    iph->check = csum((unsigned short*)(datagram+sizeof(struct ether_header)),sizeof(struct iphdr));

    struct pseudo_header psh;
    udph->check =0;
    psh.source_address = inet_addr("192.168.43.123");
    psh.dest_address = inet_addr("192.168.43.123");
    psh.placeholder = 0;
    psh.protocol = IPPROTO_UDP;
    psh.udp_length = htons(sizeof(struct udphdr)+strlen(buff));
	
	int psize = sizeof(struct pseudo_header) + sizeof(struct udphdr) + strlen(buff);
	char* pseudogram = malloc(psize);
	
	memcpy(pseudogram , (char*) &psh , sizeof (struct pseudo_header));
	memcpy(pseudogram + sizeof(struct pseudo_header) , udph , sizeof(struct udphdr) + strlen(buff));
	
	udph->check = csum( (unsigned short*)pseudogram , psize);

    struct sockaddr_ll dest;
    dest.sll_ifindex = if_nametoindex("wlp1s0");
    dest.sll_family = AF_PACKET;
    memcpy(dest.sll_addr, eh->ether_dhost, ETH_ALEN);
    dest.sll_halen = htons(ETH_ALEN);

    //while(1){
    if(sendto(sockfd, datagram, tx_len ,0, (struct sockaddr*)&dest, sizeof(dest)) < 0){
        printf("Error. Error number: %d. Error message: %s\n", errno, strerror(errno));
        exit(1);
        }
    //}

    return 0;
}
