#include <bits/stdc++.h>
#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 

#define PORT 8080

using namespace std;

int main()
{
	struct sockaddr_in serv_addr, cli_addr;

    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0) {
          perror("ERROR opening socket");
          exit(1);
       }

    bzero((char*)&serv_addr, sizeof(serv_addr));
    //initialized to avoid garbage values.

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(7227);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //localhost, loopback address
    //local IP, local port

    int opt=1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    if(bind(sfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0)
    perror("ERROR in bind\n");
    
    
    

}

