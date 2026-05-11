#ifndef HEIC2TIFF_HEIC_DECODE_H
#define HEIC2TIFF_HEIC_DECODE_H

#include <stddef.h>
#include <stdint.h>
#include "error.h"

typedef struct {
    uint8_t *pixels;
    int width;
    int height;
    int channels;
    size_t stride;
} H2TImage;

H2TStatus h2t_decode_heic(const char *path, H2TImage *out_image);
void h2t_image_free(H2TImage *image);

#endif
