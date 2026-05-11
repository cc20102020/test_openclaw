#ifndef HEIC2TIFF_PIXEL_CONVERT_H
#define HEIC2TIFF_PIXEL_CONVERT_H

#include "heic_decode.h"
#include "error.h"

H2TStatus h2t_prepare_pixels_for_tiff(const H2TImage *image);
void h2t_pixel_copy_generic(unsigned char *dst, const unsigned char *src, unsigned long n);
void h2t_pixel_copy_x86_64(unsigned char *dst, const unsigned char *src, unsigned long n);
void h2t_pixel_copy_aarch64(unsigned char *dst, const unsigned char *src, unsigned long n);

#endif
