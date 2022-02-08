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

int main()
{

int fd = open("doc.txt", O_RDWR);
char* news = malloc(100);

while(1)
{fgets(news, 100, stdin);

write(fd, news, strlen(news));
}

}
