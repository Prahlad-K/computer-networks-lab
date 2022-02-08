#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

int main()
{

char* news = malloc(100);

while(1)
{
fgets(news, 100, stdin);
printf("Screen- %s", news);
}

}
