#include "incam.h"
#include <signal.h>
#include <sys/ioctl.h>

/*
 * Modified code to grab frames from webcam by wthielen
 * https://github.com/wthielen/Webcam
 */

/**
 * Keeping tabs on opened webcam devices
 */
static webcam_t *_w[16] = {
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL};

/**
 * Private sigaction to catch segmentation fault
 */
static struct sigaction *sa;

/**
 * Private function for successfully ioctl-ing the v4l2 device
 */
static int _ioctl(int fh, int request, void *arg)
{
    int r;

    do
    {
        r = ioctl(fh, request, arg);
    } while (-1 == r && EINTR == errno);

    return r;
}

/**
 * Handler for segmentation faults
 * This should go through all the opened webcams in _w and
 * clean them up.
 */
static void handler(int sig, siginfo_t *si, void *unused)
{
    int i = 0;
    fprintf(stderr, "A segmentation fault occured. Cleaning up...\n");

    for (i = 0; i < 16; i++)
    {
        if (_w[i] == NULL)
            continue;

        // If webcam is streaming, unlock the mutex, and stop streaming
        if (_w[i]->streaming)
        {
            pthread_mutex_unlock(&_w[i]->mtx_frame);
            webcam_stream(_w[i], false);
        }
        webcam_close(_w[i]);
    }

    exit(EXIT_FAILURE);
}

/**
 * Create frame buffer and store buf data in it
 * @param buf from
 * @param frame to
 */
static void storeFrameBuffer(struct buffer buf, struct buffer *frame)
{
    size_t i;
    uint8_t y, u, v;

    int uOffset = 0;
    int vOffset = 0;

    if (frame->start == NULL)
    {
        frame->length = buf.length;
        frame->start = malloc(buf.length);
        printf("Buffer size: %d\n", buf.length);
    }
    memcpy(frame->start, buf.start, buf.length);
}

/**
 * Open the webcam on the given device and return a webcam
 * structure.
 * @param dev name (eg. /dev/video0)
 * @returns webcam pointer
 */
struct webcam *webcam_open(const char *dev)
{
    struct stat st;

    struct v4l2_capability cap;
    struct v4l2_format fmt;

    uint16_t min;

    int fd;
    struct webcam *w;

    // Prepare signal handler if not yet
    if (sa == NULL)
    {
        sa = calloc(1, sizeof(struct sigaction));
        sa->sa_flags = SA_SIGINFO;
        sigemptyset(&sa->sa_mask);
        sa->sa_sigaction = handler;
        sigaction(SIGSEGV, sa, NULL);
    }

    // Check if the device path exists
    if (-1 == stat(dev, &st))
    {
        fprintf(stderr, "Cannot identify '%s': %d, %s\n",
                dev, errno, strerror(errno));
        return NULL;
    }

    // Should be a character device
    if (!S_ISCHR(st.st_mode))
    {
        fprintf(stderr, "%s is no device\n", dev);
        return NULL;
    }

    // Create a file descriptor
    fd = open(dev, O_RDWR | O_NONBLOCK, 0);
    if (-1 == fd)
    {
        fprintf(stderr, "Cannot open'%s': %d, %s\n",
                dev, errno, strerror(errno));
        return NULL;
    }

    // Query the webcam capabilities
    if (-1 == _ioctl(fd, VIDIOC_QUERYCAP, &cap))
    {
        if (EINVAL == errno)
        {
            fprintf(stderr, "%s is no V4L2 device\n", dev);
            return NULL;
        }
        else
        {
            fprintf(stderr, "%s: could not fetch video capabilities\n", dev);
            return NULL;
        }
    }

    // Needs to be a capturing device
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        fprintf(stderr, "%s is no video capture device\n", dev);
        return NULL;
    }

    // Prepare webcam structure
    w = calloc(1, sizeof(struct webcam));
    w->fd = fd;
    w->name = strdup(dev);
    w->frame.start = NULL;
    w->frame.length = 0;
    pthread_mutex_init(&w->mtx_frame, NULL);

    // Initialize buffers
    w->nbuffers = 0;
    w->buffers = NULL;

    // Store webcam in _w
    int i = 0;
    for (; i < 16; i++)
    {
        if (_w[i] == NULL)
        {
            _w[i] = w;
            break;
        }
    }

    // Request supported formats
    struct v4l2_fmtdesc fmtdesc;
    uint32_t idx = 0;
    char *pixelformat = calloc(5, sizeof(char));
    for (;;)
    {
        fmtdesc.index = idx;
        fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (-1 == _ioctl(w->fd, VIDIOC_ENUM_FMT, &fmtdesc))
            break;

        memset(w->formats[idx], 0, 5);
        memcpy(&w->formats[idx][0], &fmtdesc.pixelformat, 4);
        fprintf(stderr, "%s: Found format: %s - %s\n", w->name, w->formats[idx], fmtdesc.description);
        idx++;
    }

    return w;
}

/**
 * Closes the webcam
 * (releases the buffers, and frees up memory)
 * @param w webcam pointer
 */
void webcam_close(webcam_t *w)
{
    uint16_t i;

    // Clear frame
    free(w->frame.start);
    w->frame.length = 0;

    // Release memory-mapped buffers
    for (i = 0; i < w->nbuffers; i++)
    {
        munmap(w->buffers[i].start, w->buffers[i].length);
    }

    // Free allocated resources
    free(w->buffers);
    free(w->name);

    // Close the webcam file descriptor, and free the memory
    close(w->fd);
    free(w);
}

// TODO: ADD webcam g fmt

/**
 * Sets the webcam to capture at the given width and height
 * @param w webcam pointer
 * @param width
 * @param heigh
 */
void webcam_resize(webcam_t *w, uint16_t width, uint16_t height)
{
    uint32_t i;
    struct v4l2_format fmt;
    struct v4l2_buffer buf;

    // Use YUYV as default for now
    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    // TODO: configuratble pix format & colorspace
    // fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.colorspace = V4L2_COLORSPACE_REC709;
    fprintf(stderr, "%s: requesting image format %ux%u\n", w->name, width, height);
    _ioctl(w->fd, VIDIOC_S_FMT, &fmt);

    // Storing result
    w->width = fmt.fmt.pix.width;
    w->height = fmt.fmt.pix.height;
    w->colorspace = fmt.fmt.pix.colorspace;

    char *pixelformat = calloc(5, sizeof(char));
    memcpy(pixelformat, &fmt.fmt.pix.pixelformat, 4);
    fprintf(stderr, "%s: set image format to %ux%u using %s\n", w->name, w->width, w->height, pixelformat);

    // Buffers have been created before, so clear them
    if (NULL != w->buffers)
    {
        for (i = 0; i < w->nbuffers; i++)
        {
            munmap(w->buffers[i].start, w->buffers[i].length);
        }

        free(w->buffers);
    }

    // Request the webcam's buffers for memory-mapping
    struct v4l2_requestbuffers req;
    CLEAR(req);

    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == _ioctl(w->fd, VIDIOC_REQBUFS, &req))
    {
        if (EINVAL == errno)
        {
            fprintf(stderr, "%s does not support memory mapping\n", w->name);
            return;
        }
        else
        {
            fprintf(stderr, "Unknown error with VIDIOC_REQBUFS: %d\n", errno);
            return;
        }
    }

    // Needs at least 2 buffers
    if (req.count < 2)
    {
        fprintf(stderr, "Insufficient buffer memory on %s\n", w->name);
        return;
    }

    // Storing buffers in webcam structure
    fprintf(stderr, "Preparing %d buffers for %s\n", req.count, w->name);
    w->nbuffers = req.count;
    w->buffers = calloc(w->nbuffers, sizeof(struct buffer));

    if (!w->buffers)
    {
        fprintf(stderr, "Out of memory\n");
        return;
    }

    // Prepare buffers to be memory-mapped
    for (i = 0; i < w->nbuffers; ++i)
    {
        CLEAR(buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (-1 == _ioctl(w->fd, VIDIOC_QUERYBUF, &buf))
        {
            fprintf(stderr, "Could not query buffers on %s\n", w->name);
            return;
        }

        w->buffers[i].length = buf.length;
        w->buffers[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, w->fd, buf.m.offset);

        if (MAP_FAILED == w->buffers[i].start)
        {
            fprintf(stderr, "Mmap failed\n");
            return;
        }
    }
}

/**
 * Reads a frame from the webcam and stores it in the webcam structure
 * @param w webcam pointer
 */
void webcam_read(struct webcam *w)
{
    struct v4l2_buffer buf;

    // Try getting an image from the device
    for (;;)
    {
        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        // Dequeue a (filled) buffer from the video device
        if (-1 == _ioctl(w->fd, VIDIOC_DQBUF, &buf))
        {
            switch (errno)
            {
            case EAGAIN:
                continue;

            case EIO:
            default:
                fprintf(stderr, "%d: Could not read from device %s\n", errno, w->name);
                break;
            }
        }

        // Make sure we are not out of bounds
        assert(buf.index < w->nbuffers);

        // Lock frame mutex, and store RGB
        pthread_mutex_lock(&w->mtx_frame);
        // memcpy(&w->v4l2buf, &buf, sizeof(buf));
        // convertToRGB(w->buffers[buf.index], &w->frame);
        storeFrameBuffer(w->buffers[buf.index], &w->frame);
        pthread_mutex_unlock(&w->mtx_frame);
        break;
    }

    // Queue buffer back into the video device
    if (-1 == _ioctl(w->fd, VIDIOC_QBUF, &buf))
    {
        fprintf(stderr, "Error while swapping buffers on %s\n", w->name);
        return;
    }
}

/**
 * The loop function for the webcam thread
 */
static void *webcam_streaming(void *ptr)
{
    webcam_t *w = (webcam_t *)ptr;

    while (w->streaming)
        webcam_read(w);
}

/**
 * Tells the webcam to go into streaming mode, or to
 * stop streaming.
 * When going into streaming mode, it also creates
 * a thread running the webcam_streaming function.
 * When exiting the streaming mode, it sets the streaming
 * bit to false, and waits for the thread to finish.
 * @param w webcam pointer
 * @param flag true: streaming | false: not sreaming
 */
void webcam_stream(webcam_t *w, bool flag)
{
    uint8_t i;

    struct v4l2_buffer buf;
    enum v4l2_buf_type type;

    if (flag)
    {
        // Clear buffers
        for (i = 0; i < w->nbuffers; i++)
        {
            CLEAR(buf);
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;

            if (-1 == _ioctl(w->fd, VIDIOC_QBUF, &buf))
            {
                fprintf(stderr, "Error clearing buffers on %s\n", w->name);
                return;
            }
        }

        // Turn on streaming
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == _ioctl(w->fd, VIDIOC_STREAMON, &type))
        {
            fprintf(stderr, "Could not turn on streaming on %s\n", w->name);
            return;
        }

        // Set streaming to true and start thread
        w->streaming = true;
        pthread_create(&w->thread, NULL, webcam_streaming, (void *)w);
    }
    else
    {
        // Set streaming to false and wait for thread to finish
        w->streaming = false;
        pthread_join(w->thread, NULL);

        // Turn off streaming
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == _ioctl(w->fd, VIDIOC_STREAMOFF, &type))
        {
            fprintf(stderr, "Could not turn streaming off on %s\n", w->name);
            return;
        }
    }
}

/**
 * Grab last frame from webcam and copy it to your frame buffer (it allocs it if not already done)
 * @param w webcam pointer
 * @param frame buffer pointer
 */
void webcam_grab(webcam_t *w, buffer_t *frame)
{
    // Locks the frame mutex so the grabber can copy
    // the frame in its own return buffer.
    pthread_mutex_lock(&w->mtx_frame);

    // Only copy frame if there is something in the webcam's frame buffer
    if (w->frame.length > 0)
    {
        // Initialize frame
        if ((*frame).start == NULL)
        {
            (*frame).start = calloc(w->frame.length, sizeof(char));
            (*frame).length = w->frame.length;
        }

        memcpy((*frame).start, w->frame.start, w->frame.length);
    }

    pthread_mutex_unlock(&w->mtx_frame);
}