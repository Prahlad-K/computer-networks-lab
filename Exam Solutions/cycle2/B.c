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
#include <gmp.h>
#include <time.h>

#define PORT 8080

int main()
{
	int sfd1;
    struct sockaddr_in Aaddress;

    Aaddress.sin_family = AF_INET; 
    Aaddress.sin_addr.s_addr = inet_addr("127.0.0.1"); 
    Aaddress.sin_port = htons( PORT ); 

    sfd1 = socket(AF_INET, SOCK_STREAM, 0);

 	connect(sfd1, (struct sockaddr *)&Aaddress, sizeof(Aaddress));


 	int sfd2;
    struct sockaddr_in address2;
    int addrlen2 = sizeof(address2); 

    address2.sin_family = AF_INET; 
    address2.sin_addr.s_addr = inet_addr("127.0.0.3"); 
    address2.sin_port = htons( PORT ); 

    sfd2 = socket(AF_INET, SOCK_STREAM, 0);
    bind(sfd2, (struct sockaddr *)&address2, sizeof(address2));

    listen(sfd2, 3);

    int nsfd2 = accept(sfd2, (struct sockaddr *)&address2, (socklen_t*)&addrlen2);

   
  	mpz_t q, alpha;

    //take a large prime number q and the generator of Zq which is alpha
    mpz_init_set_str(q, "10375039676004850895153180219140825871832424447991559740017057480128913835806851959289793274985265665777122921936941938022987986088961250820311707667977663", 10);
    mpz_init_set_str(alpha, "3489494002834111451509114829193233051117747768439168837412006394274759575652051518038030330723614924696942312359936737919658157609986927981108133394482327", 10);

    gmp_randstate_t state;
    gmp_randinit_mt(state);
    unsigned long seed;
    seed = 234212846524345;
    gmp_randseed_ui(state, seed);
 
    mpz_t XB, YA, YB, Yproxy;
    mpz_inits(XB, YA, YB, Yproxy, NULL);

    mpz_urandomm(XB, state, q);
    gmp_printf("XB: %Zd\n", XB);
    mpz_powm(YB, alpha, XB, q);
    gmp_printf("YB: %Zd\n", YB);

    char* buff = malloc(500);

    //receive public key of A
    memset(buff, 0, strlen(buff));
    if(recv(sfd1, buff, 154, MSG_WAITALL))
	{
		buff[154]= '\0';
		printf("Received YA: %s\n", buff);
	}
    else
        printf("Error receiving YA\n");

    mpz_set_str(YA, (const char*)buff, 10);

    //send public key YB to A and proxy
    mpz_get_str(buff, 10, YB);
    if(send(sfd1, buff, strlen(buff), MSG_CONFIRM))
        printf("Sent YB successfully to A\n");
    else
        printf("Error sending YB to A!\n");

    if(send(nsfd2, buff, strlen(buff), MSG_CONFIRM))
        printf("Sent YB successfully to proxy\n");
    else
        printf("Error sending YB to proxy!\n");
    
    //receive public key of proxy
    memset(buff, 0, strlen(buff));
    if(recv(nsfd2, buff, 154, MSG_WAITALL))
        printf("Received Yproxy: %s\n", buff);
    else
        printf("Error receiving Yproxy\n");
    mpz_set_str(Yproxy, (const char*)buff, 10);

    mpz_t s1, C, M1, M2, M;
    mpz_inits(s1, C, M1, M2, M, NULL);

    mpz_powm(s1, YA, XB, q);
    gmp_printf("s1: %Zd\n", s1);

    memset(buff, 0, strlen(buff));
    if(recv(sfd1, buff, 500, MSG_WAITALL))
    {
	    char *e;
		int index;
		e = strchr(buff, '#');
		index = (int)(e - buff);
		buff[index] = '\0';
        printf("Received C: %s\n", buff);
    }
    else
        printf("Error receiving C\n");
	    
	

    mpz_set_str(C, (const char*)buff, 10);

    mpz_t p;
    mpz_init(p);
    mpz_nextprime(p, q);

    mpz_t n;
    mpz_init(n);
    mpz_mul(n, p, q);

    mpz_powm(M1, C, s1, n);

    memset(buff, 0, strlen(buff));
    if(recv(nsfd2, buff, 500, MSG_WAITALL))
    {
	    char *e;
		int index;
		e = strchr(buff, '#');
		index = (int)(e - buff);
		buff[index] = '\0';
        printf("Received M2: %s\n", buff);
    }
    else
        printf("Error receiving M2\n");
    mpz_set_str(M2, (const char*)buff, 10);

    mpz_mul(M, M1, M2);
    mpz_mod(M, M, n);

    gmp_printf("Message decrypted M is: %Zd\n", M);

}

