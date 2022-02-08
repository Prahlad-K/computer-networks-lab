#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

#include <strings.h>
#include <string.h>

#include <sys/select.h>
#include <sys/types.h>
#include <signal.h>

#include <pthread.h>
#include <poll.h>
#include <sys/msg.h>
#include <fcntl.h>

struct mesg_buffer { 
	long mesg_type; 
	char mesg_text[100]; 
} message; 

int main()
{
key_t key; 
int msgid; 
key = ftok("newtable", 65); 
msgid = msgget(key, 0666 | IPC_CREAT);

char* news = malloc(100);
int portno;
int scfd = fileno(popen("./s", "w"));

int epid;
if(msgrcv(msgid, &message, sizeof(message), 2, IPC_NOWAIT)>0)
{
epid = atoi(message.mesg_text);
printf("Got editor's pid %d\n", epid);
}

while(1)
{
	while(msgrcv(msgid, &message, sizeof(message), 2, IPC_NOWAIT)>0)
	{
		strcpy(news, message.mesg_text);
		if(news[0]>='0' && news[0]<='9')
		{
			printf("Must be port number!\n");
			portno = (news[0]-'0') * 1000 + (news[1]-'0') * 100 + (news[2]-'0') * 10 + (news[3]-'0');
			printf("Port is %d\n", portno); 
			sleep(1);
			message.mesg_type = 3;
			strcpy(message.mesg_text, "stop");
			if(msgsnd(msgid, &message, sizeof(message), IPC_NOWAIT)<0)
			perror("Message not sent!");
			else
			printf("Stop message sent to News Reader 1\n");
			

			int sfd = socket(AF_INET, SOCK_STREAM, 0);
			if (sfd < 0) {
			      perror("ERROR opening socket");
			      exit(1);
			   }
			struct sockaddr_in serv_addr;
			struct hostent* server = gethostbyname("127.0.0.1"); //either the name or IP address shall do
			if (server == NULL) {
			      fprintf(stderr,"ERROR, no such host\n");
			      exit(0);
			   }
			bzero((char*)&serv_addr, sizeof(serv_addr));
			//initialized to avoid garbage values.

			serv_addr.sin_family = AF_INET;
			serv_addr.sin_port = htons(portno);
			bcopy((char*)server->h_addr, (char*)&serv_addr.sin_addr.s_addr, server->h_length);
			//server IP address copied.

			if(connect(sfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))<0)
			{
			perror("ERROR connecting");
			exit(1);
			}
			
			char* telecast = malloc(100);
			while(1)
			{
				if(fgets(telecast, 100, fdopen(sfd, "r"))>0)
				{
					printf("Received telecast, echoing onto screen.\n");
					kill(epid, SIGUSR1);
					write(scfd, telecast, strlen(telecast));
				}
				else
				{
					printf("Live telecast is concluded\n");
					message.mesg_type = 5;
					strcpy(message.mesg_text, "start");
					if(msgsnd(msgid, &message, sizeof(message), IPC_NOWAIT)<0)
					perror("Message not sent!");
					else
					printf("Start message sent to News Reader 1\n");
					break;
				}
			}
		}
		else
		{
			write(scfd, news, strlen(news));
		}
	}
	while(msgrcv(msgid, &message, sizeof(message), 4, IPC_NOWAIT)>0)
	{
		while(1)
		{
			//received the 'stop' signal
			while(msgrcv(msgid, &message, sizeof(message), 6, IPC_NOWAIT)>0)
			{
				printf("Restarted!\n");
				goto start;
			}
			printf("Sleeping!\n");
			sleep(1);
		}
	}
	start:;
}

return 0;
}














