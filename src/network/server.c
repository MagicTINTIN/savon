#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <errno.h>
#include "../cameras/outcam.h"
#include <bits/socket.h>
#define SA struct sockaddr

#define SERVER_PORT 12345
#define HEADER 4
#define BUFFER_SIZE 32768 // 65507/2
#define FRAME_BUFFER_SIZE 1280 * 720 * 2

int mainServer(int port)
{
    int i = 0;

    int fdOut = webcam_output_open_write("/dev/video10");
    if (fdOut == -1)
    {
        fprintf(stderr, "Failed to open virtual webcam output\n");
        return -1;
    }

    if (webcam_output_set_format(fdOut, 1280, 720) == -1)
    {
        fprintf(stderr, "Failed to set output format\n");
        close(fdOut);
        return -1;
    }
    // buffers
    unsigned char buffer[BUFFER_SIZE];
    unsigned char frameBuffer[FRAME_BUFFER_SIZE];

    int sockfd, connfd, len;
    struct sockaddr_in servaddr, cli;

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
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
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERVER_PORT);

    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA *)&servaddr, sizeof(servaddr))) != 0)
    {
        printf("socket bind failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully binded..\n");

    // Now server is ready to listen and verification
    if ((listen(sockfd, 5)) != 0)
    {
        printf("Listen failed...\n");
        exit(0);
    }
    else
        printf("Server listening..\n");
    len = sizeof(cli);

    // Accept the data packet from client and verification
    connfd = accept(sockfd, (SA *)&cli, &len);
    if (connfd < 0)
    {
        printf("server accept failed...\n");
        exit(0);
    }
    else
        printf("server accept the client...\n");

    ssize_t n;

    bool serverListening = true;

    while (serverListening)
    {
        bool isEndOfFrame = false;
        unsigned offset = 0;
        unsigned int previous = INT32_MAX;
        while (!isEndOfFrame)
        {
            // memset(buffer, 0, BUFFER_SIZE);
            n = read(connfd, buffer, BUFFER_SIZE);

            if (n == 0) {
                serverListening = false;
                break;
            }

            unsigned int header = *(unsigned int *)buffer;
            if (header == 0)
            {
                // si 0, il ne reste plus de morceaux associé à cette frame
                isEndOfFrame = true;
            }
            // si un paquet d'entête supérieure arrive c'est qu'on a commencé une nouvelle image, ne devrait pas arriver en tcp car reprise totale des pertes et ordre d'arrivé normalement conservé (peut être manuellement désactivé avec && false mais pratique pour visualiser nos erreurs)
            else if (header > previous)
            {
                if (webcam_output_write_frame(fdOut, frameBuffer, FRAME_BUFFER_SIZE) == -1)
                {
                    fprintf(stderr, "Error writing video frame\n");
                    break;
                }

                i++;
                // printf(" - received frame: %d\n", i);
                // fflush(stdout);

                isEndOfFrame = false;
                offset = 0;
                previous = INT32_MAX;
            }
            // printf("%u=%x:%x|", header, header, (unsigned char)buffer[HEADER]);
            // fflush(stdout);
            n -= HEADER;
            if (offset + n > FRAME_BUFFER_SIZE)
            {
                printf(" (safing copy) "); // ne devrait pas arriver car contrôlé côté serveur pour ne pas déborder... Or cela arrive systématiquement avant ce phénomène de "dépassement"...
                memcpy(frameBuffer + offset, buffer + HEADER, FRAME_BUFFER_SIZE - offset);
                isEndOfFrame = true;
            }
            else
            {
                memcpy(frameBuffer + offset, buffer + HEADER, n);
                offset += n;
            }
        }

        if (webcam_output_write_frame(fdOut, frameBuffer, FRAME_BUFFER_SIZE) == -1)
        {
            fprintf(stderr, "Error writing video frame\n");
            break;
        }

        i++;
        // printf(" - received frame: %d\n", i);
        // fflush(stdout);
    }
    printf("Closing connection...\n");
    close(sockfd);

    return 0;
}