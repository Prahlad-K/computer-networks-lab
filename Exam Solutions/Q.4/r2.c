#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

#include <strings.h>
#include <string.h>

#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>

#include <pthread.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/shm.h> 

int main()
{
mkfifo("rep2", S_IWUSR| S_IRUSR);
char* report = malloc(100);

int fd = open("rep2", O_WRONLY);

while(1)
{
fgets(report, 100, stdin);
if(strcmp(report, "getcount\n")==0)
{
	key_t key = ftok("shmcount", 65);
	int shmid = shmget(key, sizeof(int), 0666|IPC_CREAT);
	int* ptr = (int*) shmat(shmid, 0, 0);
	printf("Current count is %d\n", *ptr);

}
else
{
	if(write(fd, report, 100)>0)
	printf("Sent report %s", report); //confirmation
}
}

}
