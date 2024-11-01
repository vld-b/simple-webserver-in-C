#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <string.h>

#define PORT 8080
#define BUFFER_SIZE 2048

typedef union {
    unsigned int addr;
    unsigned char bytes[4];
} IPv4;

int main()
{
    // Creating the buffer for the socket read
    char buffer[BUFFER_SIZE];

    // Creating the response buffer and reading index.html
    FILE *index;
    index = fopen("index.html", "r");
    if (index == NULL) {
        perror("Failed to open index.html\n");
        return 4;
    }
    fseek(index, 0, SEEK_END);
    int indexLength = ftell(index);
    rewind(index);
    char indexHTML[indexLength];
    fread(&indexHTML, 1, indexLength, index);
    // Close the file
    fclose(index);

    char responseHeader[] = "HTTP/1.0 200 OK\r\nServer: vld-backend\r\nContent-Type: text/html\r\n\r\n";
    char responseBuffer[strlen(responseHeader) + indexLength];
    strcpy(( char* restrict )&responseBuffer, ( const char* restrict )&responseHeader);
    strcat(( char* restrict )&responseBuffer, ( const char* restrict )&indexHTML);

    // Create a TCP socket 
    // AF_INET sets the IPv4 protocol
    // SOCK_STREAM sets the TCP protocol
    int tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpSocket == -1) {
        perror("Socket failed to initialize\n");
        return 1;
    }
    printf("Socket created successfully\n");

    // Create the address to bind the socket to
    struct sockaddr_in hostAddress;
    int hostAddressLen = sizeof(hostAddress);

    // Set the address values
    hostAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    hostAddress.sin_port = htons(PORT);
    hostAddress.sin_family = AF_INET;
    
    struct sockaddr* convertedAddr = ( struct sockaddr * ) &hostAddress;

    // bind the socket to the address
    if (bind(tcpSocket, convertedAddr, hostAddressLen) != 0) {
        perror("Socket failed to bind\n");
        return 2;
    }
    printf("Socket bound successfully\n");
    
    struct sockaddr_in initialPeerAddress;
    int peerAddressLen = sizeof(initialPeerAddress);
    IPv4 peerAddress = {0};

    // listen to incoming connections
    if (listen(tcpSocket, 64) != 0) {
        perror("Socket failed to listen\n");
        return 3;
    }
    printf("Server listening for connections\n");

    IPv4 ip = {0};

    while (1) {
        int acceptedSock = accept(tcpSocket, convertedAddr, ( unsigned int * restrict) &hostAddressLen);
        if (acceptedSock < 0) {
            perror("Socket failed to accept\n");
            goto closeSocket;
        }
        printf("Socket accepted successfully: %d\n", acceptedSock);
        ip.addr = hostAddress.sin_addr.s_addr;
        printf("IP: %d\n", htonl(ip.addr));
        printf("%d.%d.%d.%d\n", ip.bytes[0], ip.bytes[1], ip.bytes[2], ip.bytes[3]);

        int gottenPeerName = getpeername(acceptedSock, ( struct sockaddr* )&initialPeerAddress, ( socklen_t* restrict )&peerAddressLen);
        if (gottenPeerName < 0) {
            perror("Failed to get the peer address\n");
            goto closeSocket;
        }
        peerAddress.addr = initialPeerAddress.sin_addr.s_addr;
        printf("Peer address: \n%d.%d.%d.%d\n", peerAddress.bytes[0], peerAddress.bytes[1], peerAddress.bytes[2], peerAddress.bytes[3]);

        int bytesRead = read(acceptedSock, &buffer, BUFFER_SIZE);
        if (bytesRead < 0) {
            perror("Failed to read from the socket\n");
            goto closeSocket;
        }
        printf("BUFFER: \n%s\n", buffer);
        char temp[] = {buffer[0], buffer[1], buffer[2]};
        printf("temp: \n%s \n", temp);

        int successfullyWritten = write(acceptedSock, ( const void* )responseBuffer, strlen(responseBuffer));
        if (successfullyWritten < 0) {
            perror("Failed to write to the socket\n");
            goto closeSocket;
        }
    closeSocket:
        close(acceptedSock);
        printf("Closed socket connection\n");
    }

    return 0;
}