#ifndef HEIC2TIFF_TIFF_WRITE_H
#define HEIC2TIFF_TIFF_WRITE_H

#include "heic_decode.h"
#include "error.h"

H2TStatus h2t_write_tiff(const char *path, const H2TImage *image);

#endif
