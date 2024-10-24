#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <errno.h>
#include "webcam.h"
#include <bits/socket.h>
#define SA struct sockaddr

#define SERVER_PORT 12345
#define HEADER 4
#define BUFFER_SIZE 32768 // 65507/2
#define FRAME_BUFFER_SIZE 1280 * 720 * 2

int webcam_output_open_write(const char *dev)
{
    int fd = open(dev, O_WRONLY);
    if (fd == -1)
    {
        perror("Cannot open video device for output");
        return -1;
    }
    return fd;
}

void print_format_info(int fd)
{
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));

    fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

    if (ioctl(fd, VIDIOC_G_FMT, &fmt) == -1)
    {
        perror("Failed to get video format");
        return;
    }

    printf("Width/Height      : %d/%d\n", fmt.fmt.pix.width, fmt.fmt.pix.height);
    printf("Pixel Format      : '%c%c%c%c'\n",
           (fmt.fmt.pix.pixelformat >> 0) & 0xFF,
           (fmt.fmt.pix.pixelformat >> 8) & 0xFF,
           (fmt.fmt.pix.pixelformat >> 16) & 0xFF,
           (fmt.fmt.pix.pixelformat >> 24) & 0xFF);
    printf("Colorspace        : %d\n", fmt.fmt.pix.colorspace);
    printf("Transfer Function : %d\n", fmt.fmt.pix.xfer_func);
    printf("YCbCr/HSV Encoding: %d\n", fmt.fmt.pix.ycbcr_enc);
    printf("Quantization      : %d\n", fmt.fmt.pix.quantization);
}

int force_transfer_and_encoding(int fd)
{
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));

    fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

    // Retrieve the current format
    if (ioctl(fd, VIDIOC_G_FMT, &fmt) == -1)
    {
        perror("Failed to get video format");
        return -1;
    }

    fmt.fmt.pix.ycbcr_enc = V4L2_YCBCR_ENC_601;
    fmt.fmt.pix.xfer_func = V4L2_XFER_FUNC_709;
    fmt.fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;

    // Apply the new format with explicit settings
    if (ioctl(fd, VIDIOC_S_FMT, &fmt) == -1)
    {
        perror("Failed to set video format");
        return -1;
    }

    return 0;
}

int webcam_output_set_format(int fd, uint16_t width, uint16_t height)
{
    struct v4l2_format fmt;
    CLEAR(fmt);

    fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV; // V4L2_PIX_FMT_SRGGB8;// V4L2_PIX_FMT_YUYV; // Choose your pixel format
    // fmt.fmt.pix.colorspace = V4L2_COLORSPACE_REC709;
    // Set colorspace to sRGB to avoid mapping both to Rec. 709
    fmt.fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;

    // Set the desired transfer function to Rec. 709
    fmt.fmt.pix.xfer_func = V4L2_XFER_FUNC_709; // Transfer function: Rec. 709

    // Set YCbCr encoding to ITU-R 601
    fmt.fmt.pix.ycbcr_enc = V4L2_YCBCR_ENC_601; // ITU-R 601
    // fmt.fmt.pix.field = V4L2_FIELD_NONE;

    if (-1 == ioctl(fd, VIDIOC_S_FMT, &fmt))
    {
        perror("Failed to set video format");
        return -1;
    }

    return 0;
}

int webcam_output_write_frame(int fd, const void *frame_data, size_t frame_size)
{
    ssize_t bytes_written = write(fd, frame_data, frame_size);
    fflush(stderr);
    if (bytes_written == -1)
    {
        perror("Failed to write frame data to the device");
        return -1;
    }
    return 0;
}

int main(int argc, char **argv)
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