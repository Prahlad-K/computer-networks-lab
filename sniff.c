#include <stdio.h>
#include <time.h>
#include <pcap.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/ip.h>

/* 4 bytes IP address */
typedef struct ip_address{
    u_char byte1;
    u_char byte2;
    u_char byte3;
    u_char byte4;
}ip_address;

/* IPv4 header */
typedef struct ip_header{
    u_char  ver_ihl;        // Version (4 bits) + Internet header length (4 bits)
    u_char  tos;            // Type of service 
    u_short tlen;           // Total length 
    u_short identification; // Identification
    u_short flags_fo;       // Flags (3 bits) + Fragment offset (13 bits)
    u_char  ttl;            // Time to live
    u_char  proto;          // Protocol
    u_short crc;            // Header checksum
    ip_address  saddr;      // Source address
    ip_address  daddr;      // Destination address
    u_int   op_pad;         // Option + Padding
}ip_header;

/* UDP header*/
typedef struct udp_header{
    u_short sport;          // Source port
    u_short dport;          // Destination port
    u_short len;            // Datagram length
    u_short crc;            // Checksum
}udp_header;

typedef struct _arp_hdr arp_hdr;
struct _arp_hdr {   //28 bytes
  uint16_t htype;
  uint16_t ptype;
  uint8_t hlen;
  uint8_t plen;
  uint16_t opcode;
  uint8_t sender_mac[6];
  uint8_t sender_ip[4];
  uint8_t target_mac[6];
  uint8_t target_ip[4];
}*arphdr;

void packet_handler(u_char *param, const struct pcap_pkthdr *header, const u_char *pkt_data)
{   printf("-------------------------------------------------------\n");
    struct tm ltime;
    char timestr[16];
    ip_header *ih;
    udp_header *uh;
    u_int ip_len;
    u_short sport,dport;

    /* print timestamp and length of the packet */
    printf("Grabbed packet of length: %d\n",header->len);
    printf("captured at ... %s",ctime((const time_t*)&header->ts.tv_sec));
    printf("Ethernet address length: %d\n",ETHER_HDR_LEN);
    
    uint8_t* ether_frame;
    // Allocate memory for various arrays.
    ether_frame = (uint8_t*) malloc(IP_MAXPACKET*sizeof(uint8_t));
    ether_frame = (uint8_t*) pkt_data;
    arphdr = (arp_hdr *) (ether_frame + 6 + 6 + 2);
  	int i;
	// Print out contents of received ethernet frame.
	  printf ("\nEthernet frame header:\n");
	  printf ("Destination MAC (this node): ");
	  for (i=0; i<5; i++) {
	    printf ("%02x:", ether_frame[i]);
	  }
	  printf ("%02x\n", ether_frame[5]);
	  printf ("Source MAC: ");
	  for (i=0; i<5; i++) {
	    printf ("%02x:", ether_frame[i+6]);
	  }
	  printf ("%02x\n", ether_frame[11]);
	  // Next is ethernet type code (ETH_P_ARP for ARP).
	  printf ("Ethernet type code (2054 = ARP): %u\n", ((ether_frame[12]) << 8) + ether_frame[13]);	
    if( ((ether_frame[12] << 8) + ether_frame[13]) == ETH_P_ARP) {
	   
	  printf ("\nEthernet data (ARP header):\n");
	  printf ("Hardware type (1 = ethernet (10 Mb)): %u\n", ntohs (arphdr->htype));
	  printf ("Protocol type (2048 for IPv4 addresses): %u\n", ntohs (arphdr->ptype));
	  printf ("Hardware (MAC) address length (bytes): %u\n", arphdr->hlen);
	  printf ("Protocol (IPv4) address length (bytes): %u\n", arphdr->plen);
	  printf ("Opcode (2 = ARP reply): %u\n", ntohs (arphdr->opcode));
	  printf ("Sender hardware (MAC) address: ");
	  for (i=0; i<5; i++) {
	    printf ("%02x:", arphdr->sender_mac[i]);
	  }
	  printf ("%02x\n", arphdr->sender_mac[5]);
	  printf ("Sender protocol (IPv4) address: %u.%u.%u.%u\n",
	    arphdr->sender_ip[0], arphdr->sender_ip[1], arphdr->sender_ip[2], arphdr->sender_ip[3]);
	  printf ("Target (this node) hardware (MAC) address: ");
	  for (i=0; i<5; i++) {
	    printf ("%02x:", arphdr->target_mac[i]);
	  }
	  printf ("%02x\n", arphdr->target_mac[5]);
	  printf ("Target (this node) protocol (IPv4) address: %u.%u.%u.%u\n",
	    arphdr->target_ip[0], arphdr->target_ip[1], arphdr->target_ip[2], arphdr->target_ip[3]);
    }
    else{	
    /* retireve the position of the ip header */
    ih = (ip_header *) (pkt_data + 14); //length of ethernet header

    /* retireve the position of the udp header */
    ip_len = (ih->ver_ihl & 0xf) * 4;
    uh = (udp_header *) ((u_char*)ih + ip_len);

    /* convert from network byte order to host byte order */
    sport = ntohs( uh->sport );
    dport = ntohs( uh->dport );

    /* print ip addresses and udp ports */
    printf("%d.%d.%d.%d/%d -> %d.%d.%d.%d/%d\n\n",
        ih->saddr.byte1,
        ih->saddr.byte2,
        ih->saddr.byte3,
        ih->saddr.byte4,
        sport,
        ih->daddr.byte1,
        ih->daddr.byte2,
        ih->daddr.byte3,
        ih->daddr.byte4,
        dport);
    }
   printf("-------------------------------------------------------\n");
}
int main(int argc,char* argv[]){
    char* device;
    char error_buffer[PCAP_ERRBUF_SIZE];
    device = pcap_lookupdev(error_buffer);

    if(!device){
        printf("Error finding device: %s\n",error_buffer);
        return 1;
    }
    pcap_t *handle;
    int packets_cnt_limit = 1;
    int timeout_limit = 100; //in ms
    handle = pcap_open_live(
        device,
        BUFSIZ,
        packets_cnt_limit,
        timeout_limit,
        error_buffer
    );
    if(!handle){
        printf("pcap_open_live err: %s\n",error_buffer);
        return 1;
    }
    pcap_loop(handle,atoi(argv[1]),packet_handler,NULL);
    printf("\nDone processing packets\n");
    return 0;
}
