
#include<pcap.h>
#include<stdio.h>
#include<stdlib.h> // for exit()
#include<string.h> //for memset
#include<sys/socket.h>
#include<arpa/inet.h> // for inet_ntoa()
#include<net/ethernet.h>
#include<netinet/ip_icmp.h>   //Provides declarations for icmp header
#include<netinet/udp.h>   //Provides declarations for udp header
#include<netinet/tcp.h>   //Provides declarations for tcp header
#include<netinet/ip.h>    //Provides declarations for ip header
#include <unistd.h> 
#include <errno.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/ioctl.h>
#include <net/if.h>

 
void process_packet(u_char *, const struct pcap_pkthdr *, const u_char *);
void process_ip_packet(const u_char * , int);
void print_ip_packet(const u_char * , int);
void print_tcp_packet(const u_char *  , int );
void print_udp_packet(const u_char * , int);
void print_icmp_packet(const u_char * , int );
void PrintData (const u_char * , int);
 
FILE *logfile; //to store the output
struct sockaddr_in source,dest;
char *MYip;
int tcp=0,udp=0,icmp=0,others=0,igmp=0,total=0,i,j; 


 unsigned char ip_address[15];
   

int main()
{
    pcap_if_t *alldevsp , *device;
    pcap_t *handle; //Handle of the device that shall be sniffed
 
    char errbuf[100] , *devname , devs[100][100];
    int count = 1 , n;


//get the ip address of the system


    int fd;
    struct ifreq ifr;
    
    /*AF_INET - to define network interface IPv4*/
    /*Creating soket for it.*/
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    
    /*AF_INET - to define IPv4 Address type.*/
    ifr.ifr_addr.sa_family = AF_INET;
    
    /*eth0 - define the ifr_name - port name
    where network attached.*/
    memcpy(ifr.ifr_name, "wlp2s0", IFNAMSIZ-1);
    
    /*Accessing network interface information by
    passing address using ioctl.*/
    ioctl(fd, SIOCGIFADDR, &ifr);
    /*closing fd*/
    close(fd);
    
    /*Extract IP Address*/
    strcpy(ip_address,inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
    
    printf("System IP Address is: %s\n",ip_address);

//open network device to find all the packet transfers
    
     devname = malloc(100);
    strcpy(devname,"wlp2s0");

     
    //Open the device for sniffing
    printf("Opening device %s for sniffing ... " , devname);
    handle = pcap_open_live(devname , 65535 , 1 , 0 , errbuf);
     
    if (handle == NULL) 
    {
        fprintf(stderr, "Couldn't open device %s : %s\n" , devname , errbuf);
        exit(1);
    }
    printf("Done\n");
     
    logfile=fopen("log.txt","w");
    if(logfile==NULL) 
    {
        printf("Unable to create file.");
    }
     
    //Put the device in sniff loop
    pcap_loop(handle , 0 , process_packet , NULL); 
    return 0;   
}
 

void print_ip_header(const u_char * Buffer, int Size)
{
   
    unsigned short iphdrlen;
         
    struct iphdr *iph = (struct iphdr *)(Buffer  + sizeof(struct ethhdr) );
    iphdrlen =iph->ihl*4;
     
    memset(&source, 0, sizeof(source));
    source.sin_addr.s_addr = iph->saddr;
     
    memset(&dest, 0, sizeof(dest));
    dest.sin_addr.s_addr = iph->daddr;


    fprintf(logfile , "   |-Source IP        : %s\n" , inet_ntoa(source.sin_addr) );
    fprintf(logfile , "   |-Destination IP   : %s\n" , inet_ntoa(dest.sin_addr) );


    if(strcmp(ip_address,inet_ntoa(source.sin_addr))==0) //if IP is same as source IP -- diplay destination
        {
            printf("%s\n" , inet_ntoa(dest.sin_addr) );
        }
    if(strcmp(ip_address,inet_ntoa(dest.sin_addr))==0) //if IP is same as destination IP -- diplay source
        {
            printf("%s\n" , inet_ntoa(source.sin_addr) );
        }

}
 
 void process_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *buffer)
{
    int size = header->len;
     
    print_ip_header(buffer,size);
    
}
 
 