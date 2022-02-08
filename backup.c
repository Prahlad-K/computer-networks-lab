#define ADDRESS  "server"

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


int main(int argc, char** argv)
{
	if(argc<2)
	{
		printf("Usage: %s Number of services\n", argv[0]);
	}
		
	int tnos = atoi(argv[1]);
	int usfd[tnos];
	int nusfd[tnos];
	struct sockaddr_un userv_addr,ucli_addr;
  	int userv_len,ucli_len;

	for(int i=0;i<tnos;i++)
	{
		usfd[i] = socket(AF_UNIX , SOCK_STREAM , 0);
		perror("socket");

	  	bzero(&userv_addr,sizeof(userv_addr));

		char* no = malloc(10);
		int j = i+1;
		sprintf(no, "%d", j);
	  	
		char* add = malloc(100);
		strcpy(add, "server");
		strcat(add, no);
		userv_addr.sun_family = AF_UNIX;
		
		strcpy(userv_addr.sun_path, add);
		unlink(add);
	
		userv_len = sizeof(userv_addr);

		if(bind(usfd[i], (struct sockaddr *)&userv_addr, userv_len)==-1)
		perror("server: bind");

		listen(usfd[i], 5);
		ucli_len=sizeof(ucli_addr);
		
		nusfd[i]=accept(usfd[i], (struct sockaddr *)&ucli_addr, &ucli_len);
		memset(no, '\0', strlen(no));
		memset(add, '\0', strlen(add));
	}

	int nsfd[100][100];
	
	int sfd[100];
	int noc[100];

	for(int i=0;i<tnos;i++)
	{
		noc[i] = 0;
		sfd[i] = 0;
	}
	struct sockaddr_in serv_addr, cli_addr;

	int clilen = sizeof(cli_addr);

	int main_server_down = 1;
	while(1)
	{	
		fd_set rfds;
		int max = -1;
		
		for(int i=0;i<tnos;i++)
		{
			FD_SET(nusfd[i], &rfds);
			if(nusfd[i]>max)
			max = nusfd[i];
		}
	
		if(main_server_down)
		{
			for(int i=0;i<tnos;i++)
			{
				FD_SET(sfd[i], &rfds);
				if(sfd[i]>max)
				max = sfd[i];
				for(int j=0;j<noc[i];j++)
				{
					FD_SET(nsfd[i][j], &rfds);
					if(nsfd[i][j]>max)
					max = nsfd[i][j];
				}
			}
		}
		
		struct timeval t;
		t.tv_sec=1;
		t.tv_usec=0;
		int rv = select(max + 1, &rfds, NULL, NULL, &t);

		for(int i=0;i<tnos;i++)
		{
			if(FD_ISSET(nusfd[i], &rfds))
			{
				printf("Detected server %d!\n", i+1);
				
				char* buf = malloc(100);
				memset(buf, '\0', 100);		
		
				if(recv(nusfd[i], buf, 5, MSG_WAITALL)<0)
				perror("ERROR recv");
				else
				printf("Received %s\n", buf);
				
				if(strcmp(buf, "down\n")==0)
				{
					sfd[i] = recv_fd(nusfd[i]);
					if(sfd[i]<0)
					perror("ERROR recv_fd");
					else
					printf("Received sfd %d\n", sfd[i]);
					
					int nc = 0;
					int j = 0;
					while(1)
					{
						memset(buf, '\0', strlen(buf));
						if(recv(nusfd[i], buf, 4, MSG_WAITALL)<0)
						perror("ERROR recv");
						else
						printf("Received %s\n", buf);
					
						if(strcmp(buf, "done")!=0)
						{
							nsfd[i][j] = recv_fd(nusfd[i]);
							if(nsfd[i][j]<0)
							perror("ERROR recv_fd");

							printf("nsfd %d\n", nsfd[i][j]);
							nc++;
				
							char* msg = malloc(100);
							strcpy(msg, "Backup server here");

							if(write(nsfd[i][j], msg, strlen(msg))<0)
							perror("ERROR write");
							else
							printf("Write successful to %d\n", nsfd[i][j]);
							j++;
						}
						else
							break;
					}
					noc[i] = nc;
					printf("Number of clients %d\n", noc[i]);
				}
				if(strcmp(buf, "back\n")==0)
				{
					if(send_fd(nusfd[i], sfd[i])<0)
					perror("ERROR send_fd");
					else
					printf("Sent sfd %d\n", sfd[i]);

					for(int j=0;j<noc[i];j++)
					{
						memset(buf, '\0', 100);
						strcpy(buf, "okay");
						if(send(nusfd[i], buf, strlen(buf), MSG_CONFIRM)<0)
						perror("ERROR send");
						else
						printf("Sent sending successfully\n");
	
						if(send_fd(nusfd[i], nsfd[i][j])<0)
						perror("ERROR send_fd");
						else
						printf("Sent %d\n", nsfd[i][j]);
					}
					
					memset(buf, '\0', 100);
					strcpy(buf, "done");
					if(send(nusfd[i], buf, strlen(buf), MSG_CONFIRM)<0)
					perror("ERROR send");
					else
					printf("Sent done successfully\n");
					
					noc[i] = 0;
					sfd[i] = 0;
					
					main_server_down = 0;
				}
			}
			if(main_server_down && FD_ISSET(sfd[i], &rfds))
			{
				int nsf = accept(sfd[i], (struct sockaddr *)&cli_addr, &clilen);
				if (nsf < 0) {
				      perror("ERROR on accept");
				      exit(1);
				   }
				printf("Client %d has been accepted for Server %d!\n", noc[i], i+1);				
				nsfd[i][noc[i]++] = nsf;
			}
			if(main_server_down)
			{
				for(int j=0;j<noc[i];j++)
				{
					if(FD_ISSET(nsfd[i][j], &rfds))
					{
						char* msg = malloc(100);
						//client i is now active, and sent this server a message
						read(nsfd[i][j], msg, 100);
						printf("Client %d sent this message to Server %d: %s\n", j, i, msg);
						
						for(int k=0;k<noc[i];k++)
						{
							if(j!=k) //write this message to all the clients except the one that sent it
							{
								write(nsfd[i][k], msg, strlen(msg));	
							}
						}
					}
				}
			}
		}
	}
}


