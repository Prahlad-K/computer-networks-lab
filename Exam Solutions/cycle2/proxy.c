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
    Aaddress.sin_addr.s_addr = inet_addr("127.0.0.2"); 
    Aaddress.sin_port = htons( PORT ); 

    sfd1 = socket(AF_INET, SOCK_STREAM, 0);

    connect(sfd1, (struct sockaddr *)&Aaddress, sizeof(Aaddress));


	int sfd2;
    struct sockaddr_in Baddress;

    Baddress.sin_family = AF_INET; 
    Baddress.sin_addr.s_addr = inet_addr("127.0.0.3"); 
    Baddress.sin_port = htons( PORT ); 

    sfd2 = socket(AF_INET, SOCK_STREAM, 0);

    connect(sfd2, (struct sockaddr *)&Baddress, sizeof(Baddress));


    mpz_t q, alpha;

    //take a large prime number q and the generator of Zq which is alpha
    mpz_init_set_str(q, "10375039676004850895153180219140825871832424447991559740017057480128913835806851959289793274985265665777122921936941938022987986088961250820311707667977663", 10);
    mpz_init_set_str(alpha, "3489494002834111451509114829193233051117747768439168837412006394274759575652051518038030330723614924696942312359936737919658157609986927981108133394482327", 10);

    gmp_randstate_t state;
    gmp_randinit_mt(state);
    unsigned long seed;
    seed = 349857389573753;
    gmp_randseed_ui(state, seed);
 
    mpz_t Xproxy, YA, YB, Yproxy;
    mpz_inits(Xproxy, YA, YB, Yproxy, NULL);

    mpz_urandomm(Xproxy, state, q);
    gmp_printf("X proxy: %Zd\n", Xproxy);
    mpz_powm(Yproxy, alpha, Xproxy, q);
    gmp_printf("Y proxy: %Zd\n", Yproxy);


    char* buff = malloc(500);

    //receive public keys of A and B  
    memset(buff, 0, strlen(buff));
    if(recv(sfd1, buff, 154, MSG_WAITALL))
	{
		buff[154]= '\0';
		printf("Received YA: %s\n", buff);
	}
    else
        printf("Error receiving YA\n");

    mpz_set_str(YA, (const char*)buff, 10);

    memset(buff, 0, strlen(buff));
    if(recv(sfd2, buff, 154, MSG_WAITALL))
        printf("Received YB: %s\n", buff);
    else
        printf("Error receiving YB\n");
    mpz_set_str(YB, (const char*)buff, 10);

    //send public key Yproxy to A and B
    mpz_get_str(buff, 10, Yproxy);
    if(send(sfd1, buff, strlen(buff), MSG_CONFIRM))
        printf("Sent Yproxy successfully to A\n");
    else
        printf("Error sending Yproxy to A!\n");

    if(send(sfd2, buff, strlen(buff), MSG_CONFIRM))
        printf("Sent Yproxy successfully to B\n");
    else
        printf("Error sending Yproxy to B!\n");

    mpz_t s2, M2, C;
    mpz_inits(s2, M2, C, NULL);

    mpz_powm(s2, YA, Xproxy, q);
    gmp_printf("s2: %Zd\n", s2);

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


    gmp_printf("n: %Zd\n", n);
    mpz_powm(M2, C, s2, n);

    mpz_get_str(buff, 10, M2);
    strcat(buff, "#");
    if(send(sfd2, buff, strlen(buff), MSG_CONFIRM))
        printf("Sent M2 successfully to B\n");
    else
        printf("Error sending M2 to B!\n");

}
