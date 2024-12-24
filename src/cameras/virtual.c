#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../network/clientserver.h"

int createVirtualCams(const int channels[], const char *names[], int channelsCount)
{
    if (channelsCount > MAX_CAMERAS_COUNT)
    {
        fprintf(stderr, "Can not create more than %d cameras\n", MAX_CAMERAS_COUNT);
        return 1;
    }

    if (channelsCount <= 0)
    {
        system("sudo modprobe -r v4l2loopback");
        printf("No camera created\n");

        return 0;
    }

    // sizeof("sudo modprobe v4l2loopback video_nr= card_label=") == 48 + nbcam×(2×',' + 2×'\"' + '1'..'99' + margin 2) + margin 20
    char cmd[48 + (MAX_CAMERAS_COUNT * (8 + MAX_CAM_NAME_LENGTH)) + 20] = "sudo modprobe v4l2loopback video_nr=";

    for (int chNb = 0; chNb < channelsCount; chNb++)
    {
        if (channels[chNb] > 99 || channels[chNb] < 0)
        {
            fprintf(stderr, "%d is not a valid camera ID (should be between 0 and 99)\n", channels[chNb]);
            return 1;
        }
        if (chNb > 0)
            strcat(cmd, ",");
        sprintf(cmd, "%s%d", cmd, channels[chNb]);
    }
    strcat(cmd, " card_label=");
    for (int chNb = 0; chNb < channelsCount; chNb++)
    {
        if (strlen(names[chNb]) > MAX_CAM_NAME_LENGTH)
            fprintf(stderr, "Warning! \"%s\" is too long for a name (max length: %d). Name cropped...\n", names[chNb], MAX_CAM_NAME_LENGTH);
        if (chNb > 0)
            strcat(cmd, ",");
        strcat(cmd, "\"");
        strncat(cmd, names[chNb], MAX_CAM_NAME_LENGTH);
        strcat(cmd, "\"");
    }

    // load the v4l2loopback module
    system("sudo modprobe -r v4l2loopback");
    system(cmd);
    printf("Updating cams: \"%s\"\n", cmd);
    return 0;
}