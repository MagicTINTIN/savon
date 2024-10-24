#pragma once
#include "cameras.h"

/**
 * Buffer structure
 */
typedef struct buffer {
    uint8_t *start;
    size_t  length;
} buffer_t;

/**
 * Webcam structure
 */
typedef struct webcam {
    char            *name;
    int             fd;
    buffer_t        *buffers;
    uint8_t         nbuffers;


    // struct v4l2_buffer     v4l2buf;
    buffer_t        frame;
    pthread_t       thread;
    pthread_mutex_t mtx_frame;

    uint16_t        width;
    uint16_t        height;
    uint8_t         colorspace;

    char            formats[16][5];
    bool            streaming;
} webcam_t;

webcam_t *webcam_open(const char *dev);
void webcam_close(webcam_t *w);
void webcam_resize(webcam_t *w, uint16_t width, uint16_t height);
void webcam_stream(webcam_t *w, bool flag);
void webcam_grab(webcam_t *w, buffer_t *frame);
void webcam_read(struct webcam *w);
// void convertToRGB(struct buffer buf, struct buffer *frame);