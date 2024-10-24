#include "outcam.h"

/**
 * Open device to write video
 */
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
