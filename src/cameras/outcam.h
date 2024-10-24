#pragma once
#include "cameras.h"

int webcam_output_open_write(const char *dev);
void print_format_info(int fd);
int force_transfer_and_encoding(int fd);
int webcam_output_set_format(int fd, uint16_t width, uint16_t height);
int webcam_output_write_frame(int fd, const void *frame_data, size_t frame_size);
