// udp client driver program
#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include "clientserver.h"

int mainClient(const char *addr, int port)
{
    char buffer[FRAME_SIZE];

    int fd = open("/dev/video1", O_RDWR); // Open the real webcam
    if (fd == -1)
    {
        perror("Error opening webcam");
        return -1;
    }

    char *message = "Hello Server";
    int sockfd, n;
    struct sockaddr_in servaddr;

    // clear servaddr
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_addr.s_addr = inet_addr(addr);
    servaddr.sin_port = htons(port);
    servaddr.sin_family = AF_INET;

    // create datagram socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    // connect to server
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        printf("\n Error : Connect Failed \n");
        exit(0);
    }

    // waiting for response
    // recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)NULL, NULL);
    // puts(buffer);

    // request to send datagram
    // no need to specify server address in sendto
    // connect stores the peers IP and port
    while (1)
    {
        read(fd, buffer, FRAME_SIZE);
        sendto(sockfd, message, FRAME_SIZE, 0, (struct sockaddr *)NULL, sizeof(servaddr));
    }

    // close the descriptor
    close(fd);
    close(sockfd);
}