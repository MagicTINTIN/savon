#pragma once

#define DEFAULT_PORT 5000 
#define MAXLINE 1000

#define MAX_CAMERAS_COUNT 10
#define MAX_CAM_NAME_LENGTH 20

#define FRAME_SIZE 4096

int mainClient(const char *addr, int port);
int mainServer(int port);

int createVirtualCams(const int channels[], const char* names[], int channelsCount);