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
#include<sys/ipc.h>
#include<sys/msg.h>

struct myipheader
 {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    unsigned int ihl:4;
    unsigned int version:4;
#elif __BYTE_ORDER == __BIG_ENDIAN
    unsigned int version:4;
    unsigned int ihl:4;
#else
# error	"Please fix <bits/endian.h>"
#endif
    uint8_t tos;
    uint16_t tot_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t check;
    uint32_t saddr;
    uint32_t daddr;
    /*The options start here. */
 };

/*
struct ether_header
{
  uint8_t  ether_dhost[ETH_ALEN];	
  uint8_t  ether_shost[ETH_ALEN];	
  uint16_t ether_type;		       
};
*/

int main(int argc, char** argv)
{

	int psfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IP));
	
	char* buf = (char*)malloc(1500);
	uint8_t src[6],dst[6];

	//usr mac address(6  bytes)
	src[0] = 0xEC;
	src[1] = 0x8E;
	src[2] = 0xB5;
	src[3] = 0x0C;
	src[4] = 0x28;
	src[5] = 0x9E;

	//destination mac address( 6 bytes )
	dst[0] = 0xEC;
	dst[1] = 0x8E;
	dst[2] = 0xB5;
	dst[3] = 0x0C;
	dst[4] = 0x28;
	dst[5] = 0x9E;

	memcpy(buf,dst,6*(sizeof (uint8_t)));
	memcpy(buf+6*(sizeof (uint8_t)),src,6*(sizeof (uint8_t)));
	
	//packet_type (2 bytes)
	buf[12] = ETH_P_ARP / 256;
	buf[13] = ETH_P_ARP % 256;

	//ethernet header complete

	struct myipheader* iph = (struct myipheader*)buf + 14;
	
	iph->version=4;
        iph->ttl=64;
        iph->id=0;
        iph->ihl=5;
        iph->protocol=100;	// value same as last argument(integer) of raw socket creation sys_call
        iph->saddr=inet_addr("172.30.103.131");
        iph->daddr=inet_addr("172.30.103.131");
        iph->tot_len=1000;
	

	pcap_t* pp;
	ppcap = pcap_open_live("enp2s0", 800, 1, 20, errbuf);

	if (ppcap == NULL) {
	printf("Could not open interface enp2s0 for packet injection: %s", errbuf);
	return 2;
	}

	/**
	* Then we send the packet and clean up after ourselves
	*/
	if (pcap_sendpacket(ppcap, buf, 100) == 0) {
	pcap_close(ppcap);
	return 0;
	}

	/**
	* If something went wrong, let's let our user know
	*/
	pcap_perror(ppcap, "Failed to inject packet");
	pcap_close(ppcap);
	return 1;


}
