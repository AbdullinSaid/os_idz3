#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */

#define RCVBUFSIZE 64   /* Size of receive buffer */


void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
}

int main(int argc, char *argv[])
{
    int sock;                        /* Socket descriptor */
    struct sockaddr_in servAddr; /* Echo server address */
    unsigned short servPort;     /* Echo server port */
    char *servIP;               
    char messageBuffer[RCVBUFSIZE];     /* Buffer for echo string */
    unsigned int sendMsgSize;      /* Length of string to echo */
    int bytesRcvd, totalBytesRcvd;   /* Bytes read in single recv() and total bytes read */

    if (argc != 3)    /* Test for correct number of arguments */
    {
       fprintf(stderr, "Usage: %s <Server IP> <Port>\n",
               argv[0]);
       exit(1);
    }

    servIP = argv[1];
    servPort = atoi(argv[2]);

    /* Create a reliable, stream socket using TCP */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");

    /* Construct the server address structure */
    memset(&servAddr, 0, sizeof(servAddr));     /* Zero out structure */
    servAddr.sin_family      = AF_INET;             /* Internet address family */
    servAddr.sin_addr.s_addr = inet_addr(servIP);   /* Server IP address */
    servAddr.sin_port        = htons(servPort); /* Server port */

    /* Establish the connection to the echo server */
    if (connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
        DieWithError("connect() failed");

    sendMsgSize = snprintf(messageBuffer, RCVBUFSIZE, "%d", 0);
    if (send(sock, messageBuffer, sendMsgSize, 0) != sendMsgSize) {
        DieWithError("send() failed");
    }

    if ((bytesRcvd = recv(sock, messageBuffer, RCVBUFSIZE, 0)) <= 0) {
        DieWithError("recv() failed or connection closed prematurely");
    }
    messageBuffer[bytesRcvd] = '\0';
    int n = atoi(messageBuffer);
    printf("Number of books: %d\n", n);
    srand(sock);

    for(;;) {
        /* Send the string to the server */
        int bookNumber = rand() % n;
        printf("Trying to get book number %d\n", bookNumber);
        sendMsgSize = snprintf(messageBuffer, RCVBUFSIZE, "%d", bookNumber);
        if (send(sock, messageBuffer, sendMsgSize, 0) != sendMsgSize) {
            DieWithError("send() failed");
        }
        if ((bytesRcvd = recv(sock, messageBuffer, RCVBUFSIZE, 0)) <= 0)
            DieWithError("recv() failed or connection closed prematurely");
        printf("Book %d read.\n", bookNumber);
        sleep(3);
    }

    close(sock);
    exit(0);
}

