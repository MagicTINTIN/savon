#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/videodev2.h>
#include <unistd.h>
#include "clientserver.h"

int mainServer(int port)
{
    char buffer[FRAME_SIZE];

    // Creating a virtual webcam
    int virtualIds[] = {10};
    const char *virtualNames[] = {"Distant"};
    createVirtualCams(virtualIds, virtualNames, 1);
    // Open the virtual webcam (created with v4l2loopback)
    int virtual_fd = open("/dev/video10", O_RDWR);
    if (virtual_fd == -1)
    {
        perror("Error opening virtual webcam");
        return -1;
    }

    char *message = "Hello Client";
    int listenfd, len;
    struct sockaddr_in servaddr, cliaddr;
    bzero(&servaddr, sizeof(servaddr));

    // create a UDP Socket
    listenfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (listenfd < 0)
    {
        perror("Socket creation failed");
        return -1;
    }

    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    servaddr.sin_family = AF_INET;

    // bind server address to socket descriptor
    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("Bind failed");
        return -1;
    }

    // buffer[n] = '\0';
    // puts(buffer);

    // // send the response
    // sendto(listenfd, message, MAXLINE, 0,
    //        (struct sockaddr *)&cliaddr, sizeof(cliaddr));

    // receive the datagram
    len = sizeof(cliaddr);
    while (1)
    {
        int n = recvfrom(listenfd, buffer, sizeof(buffer),
                         0, (struct sockaddr *)&cliaddr, &len);
        write(virtual_fd, buffer, FRAME_SIZE); // Write to virtual webcam
    }

    close(virtual_fd);
    printf("Exiting server...\n");
}