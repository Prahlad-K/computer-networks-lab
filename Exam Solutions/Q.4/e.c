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
#include <sys/shm.h> 

struct mesg_buffer { 
	long mesg_type; 
	char mesg_text[100]; 
} message; 

void reverse(char str[], int length) 
{ 
    int start = 0; 
    int end = length -1; 
    while (start < end) 
    { 
        char t = *(str+start);
	*(str+start) = *(str+end);
	*(str+end) = t;
        start++; 
        end--; 
    } 
} 
  
char* itoa(int num, char* str, int base) 
{ 
    int i = 0; 
    int isNegative = 0; 
  
    /* Handle 0 explicitely, otherwise empty string is printed for 0 */
    if (num == 0) 
    { 
        str[i++] = '0'; 
        str[i] = '\0'; 
        return str; 
    } 
  
    // In standard itoa(), negative numbers are handled only with  
    // base 10. Otherwise numbers are considered unsigned. 
    if (num < 0 && base == 10) 
    { 
        isNegative = 1; 
        num = -num; 
    } 
  
    // Process individual digits 
    while (num != 0) 
    { 
        int rem = num % base; 
        str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0'; 
        num = num/base; 
    } 
  
    // If number is negative, append '-' 
    if (isNegative) 
        str[i++] = '-'; 
  
    str[i] = '\0'; // Append string terminator 
  
    // Reverse the string 
    reverse(str, i); 
  
    return str; 
} 

int count =0;
void count_updater()
{
key_t key = ftok("shmcount", 65);
int shmid = shmget(key, sizeof(int), 0666|IPC_CREAT);
int* ptr = (int*) shmat(shmid, 0, 0);

count++;
*ptr = count;

printf("Current telecast count: %d\n", count);
signal(SIGUSR1, count_updater);
}


int main()
{

signal(SIGUSR1, count_updater);


int nor = 3;
int rfd[nor];
rfd[0] = open("rep1", O_RDONLY);
rfd[1] = open("rep2", O_RDONLY);
rfd[2] = open("rep3", O_RDONLY);

for(int i=0;i<3;i++)
{
if(rfd[i]>0)
printf("Reporter %d successfully connected\n", i+1);
}

char* pid = malloc(10);
itoa((int)getpid(), pid, 10);

key_t key; 
int msgid; 
key = ftok("newtable", 65); 
msgid = msgget(key, 0666 | IPC_CREAT);

message.mesg_type = 1;
strcpy(message.mesg_text, pid);
if(msgsnd(msgid, &message, sizeof(message), IPC_NOWAIT)<0)
perror("Message not sent!");
else
printf("Editor's Pid sent to News Reader 1\n");

message.mesg_type = 2;
strcpy(message.mesg_text, pid);
if(msgsnd(msgid, &message, sizeof(message), IPC_NOWAIT)<0)
perror("Message not sent!");
else
printf("Editor's Pid sent to News Reader 2\n");


int dfd  = fileno(popen("./d", "w"));
if(dfd>0)
printf("Document writer successfully created!\n");

int maxfd =0;
for(int i=0;i<nor;i++)
{
if(rfd[i]>maxfd)
maxfd = rfd[i];
}
maxfd++;

int n1 = 1;
fd_set rfds;
struct timeval tv;

tv.tv_sec = 1; //wait for 1s for response. if this is 0, equivalent to polling without waiting.
tv.tv_usec = 0;

FD_ZERO(&rfds);
printf("At the ready....\n");
char* rep = malloc(100);
while(1)
{
	for(int i=0;i<nor;i++)
	FD_SET(rfd[i], &rfds);

	if(select(maxfd, &rfds, NULL, NULL, &tv)>0)
	{
		for(int i=0;i<nor;i++)
		{
			if(FD_ISSET(rfd[i], &rfds))
			{
				fgets(rep, 100, fdopen(rfd[i], "r"));
				
				printf("Passing on\n");
				if(rep[0]=='#')
				{
					//write to the document writer
					printf("Report has been entered into the document writer.\n");
					for(int i=0;i<strlen(rep);i++)
					rep[i] = rep[i+1];					
					write(dfd, rep, strlen(rep));
					//removed the hash symbol
				}
				
				printf("Report from Reporter %d: %s", i+1, rep);
				if(n1==1)
				{
					message.mesg_type = 1;
					strcpy(message.mesg_text, rep);
					if(msgsnd(msgid, &message, sizeof(message), IPC_NOWAIT)<0)
					perror("Message not sent!\n");
					else
					printf("Sent to News Reader 1\n");
					n1 = 0;
				}
				else
				{	
					message.mesg_type = 2;
					strcpy(message.mesg_text, rep);
					if(msgsnd(msgid, &message, sizeof(message), IPC_NOWAIT)<0)
					perror("Message not sent!\n");
					else
					printf("Sent to News Reader 2\n");
					n1 = 1;
				}
				//alternate
			}
		}
	}
}		


}
