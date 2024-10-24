#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <linux/videodev2.h>

int createVirtualCams(const int channels[], const char* names[], int channelsCount);



#define CLEAR(x) memset(&(x), 0, sizeof(x))