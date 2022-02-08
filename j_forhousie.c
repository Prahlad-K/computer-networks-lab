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
#include <sys/ipc.h>
#include <sys/msg.h>
#include<netinet/ip.h>
#include<arpa/inet.h>
#include<pcap.h>
#include<errno.h>
#include<netinet/if_ether.h>
#include<net/ethernet.h>
#include<netinet/ether.h>
#include<netinet/udp.h>

struct mymsg
{
	long type;
	char msg[120];
};



int send_fd(int socket, int fd_to_send)
 {
  struct msghdr socket_message;
  struct iovec io_vector[1];
  struct cmsghdr *control_message = NULL;
  char message_buffer[1];
  /* storage space needed for an ancillary element with a paylod of length is CMSG_SPACE(sizeof(length)) */
  char ancillary_element_buffer[CMSG_SPACE(sizeof(int))];
  int available_ancillary_element_buffer_space;

  /* at least one vector of one byte must be sent */
  message_buffer[0] = 'F';
  io_vector[0].iov_base = message_buffer;
  io_vector[0].iov_len = 1;

  /* initialize socket message */
  memset(&socket_message, 0, sizeof(struct msghdr));
  socket_message.msg_iov = io_vector;
  socket_message.msg_iovlen = 1;

  /* provide space for the ancillary data */
  available_ancillary_element_buffer_space = CMSG_SPACE(sizeof(int));
  memset(ancillary_element_buffer, 0, available_ancillary_element_buffer_space);
  socket_message.msg_control = ancillary_element_buffer;
  socket_message.msg_controllen = available_ancillary_element_buffer_space;

  /* initialize a single ancillary data element for fd passing */
  control_message = CMSG_FIRSTHDR(&socket_message);
  control_message->cmsg_level = SOL_SOCKET;
  control_message->cmsg_type = SCM_RIGHTS;
  control_message->cmsg_len = CMSG_LEN(sizeof(int));
  *((int *) CMSG_DATA(control_message)) = fd_to_send;

  return sendmsg(socket, &socket_message, 0);
 }
 

 int recv_fd(int socket)
 {
  int sent_fd, available_ancillary_element_buffer_space;
  struct msghdr socket_message;
  struct iovec io_vector[1];
  struct cmsghdr *control_message = NULL;
  char message_buffer[1];
  char ancillary_element_buffer[CMSG_SPACE(sizeof(int))];

  /* start clean */
  memset(&socket_message, 0, sizeof(struct msghdr));
  memset(ancillary_element_buffer, 0, CMSG_SPACE(sizeof(int)));

  /* setup a place to fill in message contents */
  io_vector[0].iov_base = message_buffer;
  io_vector[0].iov_len = 1;
  socket_message.msg_iov = io_vector;
  socket_message.msg_iovlen = 1;

  /* provide space for the ancillary data */
  socket_message.msg_control = ancillary_element_buffer;
  socket_message.msg_controllen = CMSG_SPACE(sizeof(int));

  if(recvmsg(socket, &socket_message, MSG_CMSG_CLOEXEC) < 0)
   return -1;

  if(message_buffer[0] != 'F')
  {
   /* this did not originate from the above function */
   return -1;
  }

  if((socket_message.msg_flags & MSG_CTRUNC) == MSG_CTRUNC)
  {
   /* we did not provide enough space for the ancillary element array */
   return -1;
  }

  /* iterate ancillary elements */
   for(control_message = CMSG_FIRSTHDR(&socket_message);
       control_message != NULL;
       control_message = CMSG_NXTHDR(&socket_message, control_message))
  {
   if( (control_message->cmsg_level == SOL_SOCKET) &&
       (control_message->cmsg_type == SCM_RIGHTS) )
   {
    sent_fd = *((int *) CMSG_DATA(control_message));
    return sent_fd;
   }
  }

  return -1;
 }

int main()
{	
	struct mymsg msg1;
	key_t  key;
	int    mqpid;
	int    ret;
	int    len;
	system("touch f1.txt");
	if((key=ftok("f1.txt",'B')) == -1)
	{
		perror("key");
		exit(1);
	}
	if((mqpid=msgget(key,0644|IPC_CREAT))==-1)
	{
		perror("Key");
		exit(1);
	}
	
	
	msg1.type = 1;

	memset(msg1.msg,'\0',sizeof(msg1.msg));
	if(msgrcv( mqpid , &msg1 , sizeof(msg1.msg),0 ,0) == -1)
	{
		perror("msgrcv");
		exit(1);
	}
	else
	{
		printf("Client IP is %s\n", msg1.msg);
	}

	int rsfd = socket(AF_INET, SOCK_RAW, 10);

	struct sockaddr_in myaddr;
	myaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 	
	
	bind(rsfd, (struct sockaddr*)&myaddr, sizeof(myaddr));
	
	int optval=1;
	setsockopt(rsfd, IPPROTO_IP, SO_BROADCAST, &optval, sizeof(int));//IP_HDRINCL

	struct sockaddr_in send_addr;
	send_addr.sin_family = AF_INET;
	send_addr.sin_addr.s_addr = inet_addr("127.0.0.2"); 
	//IP2 is the IP of all clients
	srand(time(0)); 
	char* result = malloc(100);
	memset(result, '\0', 100);
	while(1)
	{
		int r = rand()%30;
		char* rand = malloc(100);
		memset(rand, '\0', 100);
		sprintf(rand, "%d", r);
		sendto(rsfd, rand, sizeof(rand), MSG_CONFIRM, (struct sockaddr*)&send_addr, sizeof(send_addr));	
		perror("Sendto");
		printf("Sent %s\n", rand);
		sleep(1);
	}
	//generated 5 random numbers
	
	return 0;
}
