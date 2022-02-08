#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

#include <strings.h>
#include <string.h>

#include<sys/un.h>
#include<netinet/ip.h>
#include<errno.h>

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

#define ADDRESS "server"
int main(int argc, char** argv)
{

	if(argc<3)
	{
		printf("Usage: %s Port Number Service Number\n", argv[0]);
		exit(0);
	}
	int usfd;
	struct sockaddr_un userv_addr;
  	int userv_len,ucli_len;

  	usfd = socket(AF_UNIX, SOCK_STREAM, 0);

  	if(usfd==-1)
  	perror("\nsocket  ");

  	bzero(&userv_addr,sizeof(userv_addr));
  	userv_addr.sun_family = AF_UNIX;
	char* add = malloc(100);

	printf("%s\n", argv[2]);
	strcpy(add, "server");
	strcat(add, argv[2]);
   	strcpy(userv_addr.sun_path, add);

	userv_len = sizeof(userv_addr);

	if(connect(usfd,(struct sockaddr *)&userv_addr,userv_len)==-1)
	perror("\n connect ");

	else 
	printf("Unix Connect successful\n");

	int list[100];
	int noc = 0;

	struct sockaddr_in serv_addr, cli_addr;

	int sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd < 0) {
	      perror("ERROR opening socket");
	      exit(1);
	   }

	bzero((char*)&serv_addr, sizeof(serv_addr));
	//initialized to avoid garbage values.

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[1]));
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //localhost, loopback address
	//local IP, local port

	if(bind(sfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0)
	{
	perror("ERROR bind\n");
	}
	printf("Listening now on port %s\n", argv[1]);
		
	listen(sfd, 5);
	int clilen = sizeof(cli_addr);

	int down = 0;
	while(1)
	{
		fd_set readset;
		FD_ZERO(&readset);
		
		int max=0;
		
		FD_SET(0, &readset); //the server must also listen to the input
		if(!down)		
		{
			FD_SET(sfd, &readset); //the server must listen to the sfd for adding new clients
			for(int i=0;i<noc;i++)
			{
				FD_SET(list[i], &readset);
				if(list[i]>max)
				max=list[i];		
			}
			if(sfd>max)
			max = sfd;
		}
		
		struct timeval t;
		t.tv_sec=1;
		t.tv_usec=0;
		int rv = select(max + 1, &readset, NULL, NULL, &t);

		if (rv == -1) 
		{
			perror("select");
		}
			
		//else if(rv==0)
		//{
		//	printf("Timed out!\n");
		//}	
		else if(rv>0) 
		{
			if(FD_ISSET(0, &readset))
			{
				char* buf = malloc(100);
				fgets(buf, 100, stdin);
				printf("You've entered!\n");
				if(strcmp(buf, "down\n")==0)
				{
					//printf("Server shutting down!\n");
					
					if(send(usfd, buf, strlen(buf), MSG_CONFIRM)<0)
					perror("ERROR send");
					else
					printf("Sent down successfully\n");	
					
					if(send_fd(usfd, sfd)<0)
					perror("ERROR send_fd");
					else
					printf("Sent sfd %d\n", sfd);

					for(int i=0;i<noc;i++)
					{
						memset(buf, '\0', 100);
						strcpy(buf, "okay");
						if(send(usfd, buf, strlen(buf), MSG_CONFIRM)<0)
						perror("ERROR send");
						else
						printf("Sent sending successfully\n");
	
						if(send_fd(usfd, list[i])<0)
						perror("ERROR send_fd");
						else
						printf("Sent %d\n", list[i]);
					}
					
					memset(buf, '\0', 100);
					strcpy(buf, "done");
					if(send(usfd, buf, strlen(buf), MSG_CONFIRM)<0)
					perror("ERROR send");
					else
					printf("Sent done successfully\n");
					
					//sent all the required nsfds and sfd.
					down = 1;
				}
				if(strcmp(buf, "back\n")==0)
				{
					if(send(usfd, buf, strlen(buf), MSG_CONFIRM)<0)
					perror("ERROR send");
					else
					printf("Sent back successfully\n");
					
					sfd = recv_fd(usfd);
					if(sfd<0)
					perror("ERROR recv_fd");
					else
					printf("Received sfd %d\n", sfd);
					noc = 0;
					int j = 0;
					while(1)
					{
						memset(buf, '\0', strlen(buf));
						if(recv(usfd, buf, 4, MSG_WAITALL)<0)
						perror("ERROR recv");
						else
						printf("Received %s\n", buf);
					
						if(strcmp(buf, "done")!=0)
						{
							list[j] = recv_fd(usfd);
							if(list[j]<0)
							perror("ERROR recv_fd");

							printf("nsfd %d\n", list[j]);
							noc++;
				
							char* msg = malloc(100);
							strcpy(msg, "Main server here");

							if(write(list[j], msg, strlen(msg))<0)
							perror("ERROR write");
							else
							printf("Write successful to %d\n", list[j]);
							j++;
						}
						else
							break;
					}
					printf("Number of clients returned is %d\n", noc);
					down = 0;
				}
			}
			if(!down && FD_ISSET(sfd, &readset))
			{
				int nsfd = accept(sfd, (struct sockaddr *)&cli_addr, &clilen);
				if (nsfd < 0) {
				      perror("ERROR on accept");
				      exit(1);
				   }
				printf("Client %d has been accepted!\n", noc);				
				list[noc++] = nsfd;								
			}
			// check for events 
			for(int i=0;i<noc;i++)
	    		{
				if (!down && FD_ISSET(list[i], &readset)) 
				{
					char* msg = malloc(100);
					//client i is now active, and sent this server a message
					read(list[i], msg, 100);
					printf("Client %d sent this message: %s\n", i, msg);
					
					for(int j=0;j<noc;j++)
					{
						if(i!=j) //write this message to all the clients except the one that sent the message
						{
							if(write(list[j], msg, strlen(msg))<0)
							perror("ERROR write");
							else
							printf("Write successful to %d\n", list[j]);	
						}
					}
		    		}
			}
		}
	}

	return 0;

}

