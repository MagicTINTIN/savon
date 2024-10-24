#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <errno.h>
#include <bits/socket.h>
#include <netinet/tcp.h>
#include "../cameras/incam.h"
#define SA struct sockaddr

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 12345
#define BUFFER_SIZE_W_END_BYTE 32768 // 65507 // UDP max size
#define HEADER 4
#define BUFFER_SIZE (BUFFER_SIZE_W_END_BYTE - HEADER) // UDP max size - header size

int mainClient(const char *addr, int port)
{
    int i = 0;
    webcam_t *w = webcam_open("/dev/video0");

    // Prepare frame, and filename, and file to store frame in
    buffer_t frame;
    frame.start = NULL;
    frame.length = 0;

    int sockfd, connfd;
    struct sockaddr_in servaddr, cli;

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    /* Don't delay send to coalesce packets (NAGLING) */
    int flag = 1;
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
    if (sockfd == -1)
    {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    servaddr.sin_port = htons(SERVER_PORT);

    // connect the client socket to server socket
    if (connect(sockfd, (SA *)&servaddr, sizeof(servaddr)) != 0)
    {
        printf("connection with the server failed...\n");
        exit(0);
    }
    else
        printf("connected to the server..\n");

    webcam_resize(w, 1280, 720);
    webcam_stream(w, true);

    if (strncmp(addr, "stop", 4) == 0)
    {
        webcam_stream(w, false);
        webcam_close(w);
        printf("Closed\n");
        exit(0);
    }

    char endOfFrame[] = "END of FRAME: Code 0123456789\0";

    while (true)
    {
        webcam_grab(w, &frame);

        if (frame.length > 0)
        {
            size_t offset = 0;
            unsigned int loops = frame.length / BUFFER_SIZE + (frame.length % BUFFER_SIZE == 0 ? 0 : 1);
            while (offset < frame.length)
            {
                loops--;
                size_t chunk_size = (frame.length - offset > BUFFER_SIZE) ? BUFFER_SIZE : frame.length - offset;
                unsigned char buf[BUFFER_SIZE_W_END_BYTE] = {0};
                unsigned int *headerLocation = (unsigned int *)buf;
                *headerLocation = loops;
                if (offset + chunk_size > frame.length)
                {
                    printf("SHOULD NOT APPEAR!\n");
                    *headerLocation = 0;
                } // and it never appear, so from this side it seems fine
                memcpy(buf + HEADER, frame.start + offset, chunk_size);
                // printf("%d=%x:%x|", loops, *headerLocation, (unsigned char)buf[HEADER]);
                ssize_t sent_bytes = write(sockfd, buf, chunk_size + HEADER);

                if (sent_bytes == -1)
                {
                    perror("Failed to send frame data");
                    break;
                }
                offset += chunk_size;
            }
            usleep(1000000 / 25);

            i++;
            // printf(" - frame: %d\n", i);
            // fflush(stdout);
        }
    }
    webcam_stream(w, false);
    webcam_close(w);

    if (frame.start != NULL)
        free(frame.start);

    return 0;
}