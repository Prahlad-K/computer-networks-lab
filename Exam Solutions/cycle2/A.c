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
void gcd(mpz_t a, mpz_t b, mpz_t c, mpz_t x, mpz_t y) 
{ 
    if (mpz_cmp_si(a, 0) == 0) 
    {
        mpz_set_si(x, 0);
        mpz_set_si(y, 1);
        mpz_set(c, b);
    }
    else
    {
        mpz_t modvalue;
        mpz_init(modvalue);

        mpz_t x1, y1;
        mpz_init(x1);
        mpz_init(y1);
        
        mpz_mod(modvalue, b, a); 
        gcd(modvalue, a, c, x1, y1); 
        
        mpz_t bdiva;
        mpz_init(bdiva);
        mpz_fdiv_q(bdiva, b, a);

        mpz_t mulx1;
        mpz_init(mulx1);
        mpz_mul(mulx1, x1, bdiva);

        mpz_sub(x, y1, mulx1);
        mpz_set(y, x1);
    }
} 

int main()
{
	int sfd1;
    struct sockaddr_in address1;
    int addrlen1 = sizeof(address1); 

    address1.sin_family = AF_INET; 
    address1.sin_addr.s_addr = inet_addr("127.0.0.1"); 
    address1.sin_port = htons( PORT ); 

    sfd1 = socket(AF_INET, SOCK_STREAM, 0);
    bind(sfd1, (struct sockaddr *)&address1, sizeof(address1));

    listen(sfd1, 3);

    int nsfd1 = accept(sfd1, (struct sockaddr *)&address1, (socklen_t*)&addrlen1);

    int sfd2;
    struct sockaddr_in address2;
    int addrlen2 = sizeof(address2); 

    address2.sin_family = AF_INET; 
    address2.sin_addr.s_addr = inet_addr("127.0.0.2"); 
    address2.sin_port = htons( PORT ); 

    sfd2 = socket(AF_INET, SOCK_STREAM, 0);
    bind(sfd2, (struct sockaddr *)&address2, sizeof(address2));

    listen(sfd2, 3);

    int nsfd2 = accept(sfd2, (struct sockaddr *)&address2, (socklen_t*)&addrlen2);

    //nsfd1 is for B, nsfd2 is for proxy
    //A receives a message that is encrypted with it's public key

    //Calculate A's public and private keys
    //Must choose p and alpha

    mpz_t q, alpha;

    //take a large prime number q and the generator of Zq which is alpha
    mpz_init_set_str(q, "10375039676004850895153180219140825871832424447991559740017057480128913835806851959289793274985265665777122921936941938022987986088961250820311707667977663", 10);
    mpz_init_set_str(alpha, "3489494002834111451509114829193233051117747768439168837412006394274759575652051518038030330723614924696942312359936737919658157609986927981108133394482327", 10);
    gmp_randstate_t state;
    gmp_randinit_mt(state);
    unsigned long seed;
    seed = 96743238742356;
    gmp_randseed_ui(state, seed);
 
    mpz_t XA, YA, YB, Yproxy;
    mpz_inits(XA, YA, YB, Yproxy, NULL);

    mpz_urandomm(XA, state, q);
    gmp_printf("XA: %Zd\n", XA);
    mpz_powm(YA, alpha, XA, q);
    gmp_printf("YA: %Zd\n", YA);
    //first perform diffie hellman key exhange for XA1 and XA2

    char* buff = malloc(500);
    memset(buff, 0, strlen(buff));
    mpz_get_str(buff, 10, YA);

    //send public key YA to B and proxy
    if(send(nsfd1, buff, strlen(buff), MSG_CONFIRM))
        printf("Sent YA successfully to B\n");
    else
        printf("Error sending YA to B!\n");

    if(send(nsfd2, buff, strlen(buff), MSG_CONFIRM))
        printf("Sent YA successfully to proxy\n");
    else
        printf("Error sending YA to proxy!\n");


    //receive public keys of B and proxy YB  proxy
    memset(buff, 0, strlen(buff));
    if(recv(nsfd1, buff, 154, MSG_WAITALL))
        printf("Received YB: %s\n", buff);
    else
        printf("Error receiving YB\n");
    mpz_set_str(YB, (const char*)buff, 10);

    memset(buff, 0, strlen(buff));
    if(recv(nsfd2, buff, 154, MSG_WAITALL))
        printf("Received Yproxy: %s\n", buff);
    else
        printf("Error receiving Yproxy\n");
    mpz_set_str(Yproxy, (const char*)buff, 10);

    mpz_t s1, s2;
    mpz_inits(s1, s2, NULL);

    mpz_powm(s1, YB, XA, q);
    mpz_powm(s2, Yproxy, XA, q);

    gmp_printf("s1: %Zd\n", s1);
    gmp_printf("s2: %Zd\n", s2);

    //perform RSA!    
    mpz_t d, e;
    mpz_inits(d, e, NULL);

    mpz_add(d, s1, s2);

    mpz_t p, pminus1, qminus1;
    mpz_inits(p, pminus1, qminus1, NULL);
    mpz_nextprime(p, q);
    mpz_sub_ui(pminus1, p, 1);
    mpz_sub_ui(qminus1, q, 1);

    mpz_t phi, n;
    mpz_inits(phi, n, NULL);
    mpz_mul(n, p, q);
    mpz_mul(phi, pminus1, qminus1);

    mpz_t x, y, c;
    mpz_inits(x, y, c, NULL);
    mpz_set_ui(c, 0);

    mpz_mod(d, d, phi);
    mpz_invert(e, d, phi);

    gmp_printf("d: %Zd\n", d);
    gmp_printf("e: %Zd\n", e);


    mpz_t M, C;
    mpz_inits(M, C, NULL);
    char* message = malloc(200);
    printf("Enter a message: ");
    scanf("%s", message);

    mpz_set_str(M, message, 10);

    gmp_printf("n: %Zd\n", n);
    mpz_powm(C, M, e, n);

    memset(buff, 0, strlen(buff));
    mpz_get_str(buff, 10, C);

    gmp_printf("C: %Zd\n", C);

    strcat(buff, "#");
    //send Ciphertext C to B and proxy
    if(send(nsfd1, buff, strlen(buff), MSG_CONFIRM))
        printf("Sent C successfully to B\n");
    else
        printf("Error sending C to B!\n");

    memset(buff, 0, strlen(buff));
    mpz_get_str(buff, 10, C);
    strcat(buff, "#");
    if(send(nsfd2, buff, strlen(buff), MSG_CONFIRM))
        printf("Sent C successfully to proxy\n");
    else
        printf("Error sending C to proxy!\n");

    mpz_t Mrec;
    mpz_init(Mrec);

    mpz_powm(Mrec, C, d, n);

    gmp_printf("Recovered plaintext: %Zd\n", Mrec);

}

