#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>  /* TCP echo server includes */
#include <pthread.h>        /* for POSIX threads */
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/wait.h>

void *ThreadMain(void *arg);            /* Main program of a thread */
 
#define MAXPENDING 5    /* Maximum outstanding connection requests */
 
void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
}
 
int CreateTCPServerSocket(unsigned short port)
{
    int sock;                        /* socket to create */
    struct sockaddr_in servAddr; /* Local address */
 
    /* Create socket for incoming connections */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");
 
    /* Construct local address structure */
    memset(&servAddr, 0, sizeof(servAddr));   /* Zero out structure */
    servAddr.sin_family = AF_INET;                /* Internet address family */
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    servAddr.sin_port = htons(port);              /* Local port */
 
    /* Bind to the local address */
    if (bind(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
        DieWithError("bind() failed");
 
    /* Mark the socket so it will listen for incoming connections */
    if (listen(sock, MAXPENDING) < 0)
        DieWithError("listen() failed");
 
    return sock;
}

/* Structure of arguments to pass to client thread */
struct ThreadArgs
{
    int clntSock;                      /* Socket descriptor for client */
};

#define MAX_BOOKS 50
#define MAX_BUFFER_SIZE 100000
#define RCVBUFSIZE 64

static int semBooks[MAX_BOOKS];
static int semMonitorBuffer;

static char monitorBuffer[MAX_BUFFER_SIZE];
static int monitorBufferSize = 0;
static int booksNum;

struct sembuf sem_wait = {0, -1, SEM_UNDO};
struct sembuf sem_signal = {0, 1, SEM_UNDO};

void clean_all() {
    for (int i = 0; i < booksNum; ++i) {
         semctl(semBooks[i], 0, IPC_RMID, 0);
    }
}

void sigint_handler(int signum) {
    clean_all();
    exit(-1);
}

int AcceptTCPConnection(int servSock)
{
    int clntSock;                    /* Socket descriptor for client */
    struct sockaddr_in echoClntAddr; /* Client address */
    unsigned int clntLen;            /* Length of client address data structure */

    /* Set the size of the in-out parameter */
    clntLen = sizeof(echoClntAddr);
    
    /* Wait for a client to connect */
    if ((clntSock = accept(servSock, (struct sockaddr *) &echoClntAddr, 
           &clntLen)) < 0)
        DieWithError("accept() failed");
    
    /* clntSock is connected to a client! */
    
    printf("Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));

    return clntSock;
}


void HandleTCPClient(int clntSocket)
{
    char messageBuffer[RCVBUFSIZE];        /* Buffer for echo string */
    int recvMsgSize;                    /* Size of received message */
    int sendMsgSize;
    /* Receive message from client */
    if ((recvMsgSize = recv(clntSocket, messageBuffer, RCVBUFSIZE, 0)) < 0) {
        DieWithError("recv() failed");
    }

    int startMonitorIndex = monitorBufferSize;

    semop(semMonitorBuffer, &sem_wait, 1);
    monitorBufferSize += snprintf(monitorBuffer + monitorBufferSize, RCVBUFSIZE, "Client %d connected\n", clntSocket);
    semop(semMonitorBuffer, &sem_signal, 1);

    if (recvMsgSize != 1) {
        DieWithError("incorrect client type");
    }
    char clientType = messageBuffer[0];
    
    sendMsgSize = snprintf(messageBuffer, RCVBUFSIZE, "%d", booksNum);
    if (send(clntSocket, messageBuffer, sendMsgSize, 0) != sendMsgSize) {
        DieWithError("send() failed");
    }
    semop(semMonitorBuffer, &sem_wait, 1);
    monitorBufferSize += snprintf(monitorBuffer + monitorBufferSize, RCVBUFSIZE, "Server sent to Client %d number of books %d\n", clntSocket, booksNum);
    semop(semMonitorBuffer, &sem_signal, 1);
    if (clientType == '0') { /* library reader client */
        while (recvMsgSize > 0) { /* zero indicates end of transmission */
            if ((recvMsgSize = recv(clntSocket, messageBuffer, RCVBUFSIZE, 0)) < 0) {
                DieWithError("recv() failed");
            }
            int requiredBook = atoi(messageBuffer);
            semop(semMonitorBuffer, &sem_wait, 1);
            printf("Client %d required book number %d\n", clntSocket, requiredBook);
            monitorBufferSize += snprintf(monitorBuffer + monitorBufferSize, RCVBUFSIZE, "Client %d required book number %d\n", clntSocket, requiredBook);
            semop(semMonitorBuffer, &sem_signal, 1);

            semop(semBooks[requiredBook], &sem_wait, 1);
            semop(semMonitorBuffer, &sem_wait, 1);
            printf("Client %d took book number %d\n", clntSocket, requiredBook);
            monitorBufferSize += snprintf(monitorBuffer + monitorBufferSize, RCVBUFSIZE, "Client %d took book number %d\n", clntSocket, requiredBook);
            semop(semMonitorBuffer, &sem_signal, 1);
            sleep(10);
            semop(semMonitorBuffer, &sem_wait, 1);
            printf("Client %d returned book number %d\n", clntSocket, requiredBook);
            monitorBufferSize += snprintf(monitorBuffer + monitorBufferSize, RCVBUFSIZE, "Client %d returned book number %d\n", clntSocket, requiredBook);
            semop(semMonitorBuffer, &sem_signal, 1);
            semop(semBooks[requiredBook], &sem_signal, 1);

            if (send(clntSocket, "home", 4, 0) != 4) {
                DieWithError("send() failed");
            }
            semop(semMonitorBuffer, &sem_wait, 1);
            printf("Client %d walked home\n", clntSocket);
            monitorBufferSize += snprintf(monitorBuffer + monitorBufferSize, RCVBUFSIZE, "Client %d walked home\n", clntSocket);
            semop(semMonitorBuffer, &sem_signal, 1);
        }
    } else if (clientType == '1') { /* monitor client */
        for(;;) {
            sleep(10);
            semop(semMonitorBuffer, &sem_wait, 1);
            while (startMonitorIndex < monitorBufferSize) {
                sleep(1);
                printf("%d %d\n", startMonitorIndex, monitorBufferSize);
                if (monitorBufferSize - startMonitorIndex <= RCVBUFSIZE) {
                    startMonitorIndex += send(clntSocket, monitorBuffer + startMonitorIndex, monitorBufferSize - startMonitorIndex, 0);
                } else {
                    startMonitorIndex += send(clntSocket, monitorBuffer + startMonitorIndex, RCVBUFSIZE - 1, 0);
                }
            }
            semop(semMonitorBuffer, &sem_signal, 1);
        }
    } else {
        DieWithError("incorrect client type");
    }

    close(clntSocket); /* Close client socket */
}



int main(int argc, char *argv[])
{
    int servSock;                    /* Socket descriptor for server */
    int clntSock;                    /* Socket descriptor for client */
    unsigned short servPort;     /* Server port */
    pthread_t threadID;              /* Thread ID from pthread_create() */
    struct ThreadArgs *threadArgs;   /* Pointer to argument structure for thread */

    if (argc != 3)     /* Test for correct number of arguments */
    {
        fprintf(stderr,"Usage: %s <SERVER PORT> <NUMBER OF BOOKS>\n", argv[0]);
        exit(1);
    }

    signal(SIGINT, sigint_handler);
    signal(SIGTERM, sigint_handler);
    
    servPort = atoi(argv[1]);  /* First arg:  local port */
    booksNum = atoi(argv[2]);

    if ((semMonitorBuffer = semget(123, 1, IPC_CREAT | 0666)) == -1) {
        perror("semget");
        exit(-1);
    }
    if (semctl(semMonitorBuffer, 0, SETVAL, 1) == -1) {
        perror("semctl");
        exit(-1);
    }
    for (int i = 0; i < booksNum; ++i) {
        if ((semBooks[i] = semget(1234 * (i + 1), 1, IPC_CREAT | 0666)) == -1) {
            perror("semget");
            exit(-1);
        }
        if (semctl(semBooks[i], 0, SETVAL, 1) == -1) {
            perror("semctl");
            exit(-1);
        }
    }
    monitorBufferSize = 0;
    servSock = CreateTCPServerSocket(servPort);

    for (;;) /* run forever */
    {
        clntSock = AcceptTCPConnection(servSock);

        /* Create separate memory for client argument */
        if ((threadArgs = (struct ThreadArgs *) malloc(sizeof(struct ThreadArgs)))
               == NULL)
            DieWithError("malloc() failed");
        threadArgs -> clntSock = clntSock;

        /* Create client thread */
        if (pthread_create(&threadID, NULL, ThreadMain, (void *) threadArgs) != 0)
            DieWithError("pthread_create() failed");
        printf("with thread %ld\n", (long int) threadID);
    }
    /* NOT REACHED */
}

void *ThreadMain(void *threadArgs)
{
    int clntSock;                   /* Socket descriptor for client connection */

    /* Guarantees that thread resources are deallocated upon return */
    pthread_detach(pthread_self());

    /* Extract socket file descriptor from argument */
    clntSock = ((struct ThreadArgs *) threadArgs) -> clntSock;
    free(threadArgs);              /* Deallocate memory for argument */

    HandleTCPClient(clntSock);

    return (NULL);
}

