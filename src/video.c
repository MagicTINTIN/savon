#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "clientserver.h"

int createVirtualCams(const int channels[], const char *names[], int channelsCount)
{

    system("sudo modprobe v4l2loopback devices=1 video_nr=3 card_label=\"DistantCam\"");
    printf("Virtual webcam 'DistantCam' created at /dev/video3\n");
    return 0;
}