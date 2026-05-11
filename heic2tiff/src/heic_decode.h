#ifndef HEIC2TIFF_HEIC_DECODE_H
#define HEIC2TIFF_HEIC_DECODE_H

#include <stddef.h>
#include <stdint.h>
#include "error.h"

/* 4K-class product limit: supports common 4032x3024 iPhone stills and UHD 3840x2160. */
#define H2T_MAX_DIMENSION 4096
#define H2T_MAX_PIXELS ((size_t)H2T_MAX_DIMENSION * (size_t)H2T_MAX_DIMENSION)

typedef struct {
    uint8_t *pixels;
    int width;
    int height;
    int channels;
    size_t stride;
} H2TImage;

int h2t_dimensions_supported(int width, int height);
H2TStatus h2t_decode_heic(const char *path, H2TImage *out_image);
void h2t_image_free(H2TImage *image);

#endif
