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

int main()
{
	int sfd;

	struct sockaddr_in address;
	address.sin_family= AF_INET;
	address.sin_addr.s_addr = inet_addr("127.0.0.1");
	address.sin_port = htons(8080);

	int addlen = sizeof(address);

	sfd = socket(AF_INET, SOCK_STREAM, 0);
	connect(sfd, (struct sockaddr*)&address, sizeof(address));

	char* buff = malloc(100);

	if(recv(sfd, buff, 100, MSG_WAITALL)<0)
		perror("Recv error");
	char* e;
	int index;
	e = strchr(buff, '#');
	index = (int)(e-buff);
	buff[index] = '\0';

	printf("Recvd: %s", buff);

}