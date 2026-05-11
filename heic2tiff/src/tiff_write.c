#include "tiff_write.h"
#include <stddef.h>
#include <stdint.h>
#include <tiffio.h>

static int h2t_valid_tiff_image(const H2TImage *image) {
    if (image == NULL || image->pixels == NULL) return 0;
    if (image->width <= 0 || image->height <= 0) return 0;
    if (image->channels != 3 && image->channels != 4) return 0;

    size_t width = (size_t)image->width;
    size_t channels = (size_t)image->channels;
    if (width > SIZE_MAX / channels) return 0;
    size_t row_bytes = width * channels;
    if (image->stride < row_bytes) return 0;
    return 1;
}

static int h2t_set_required_tiff_fields(TIFF *tif, const H2TImage *image) {
    if (!TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, (uint32_t)image->width)) return 0;
    if (!TIFFSetField(tif, TIFFTAG_IMAGELENGTH, (uint32_t)image->height)) return 0;
    if (!TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, (uint16_t)image->channels)) return 0;
    if (!TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8)) return 0;
    if (!TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT)) return 0;
    if (!TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG)) return 0;
    if (!TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB)) return 0;
    if (!TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_LZW)) return 0;
    if (image->channels == 4) {
        uint16_t extra_samples[] = { EXTRASAMPLE_UNASSALPHA };
        if (!TIFFSetField(tif, TIFFTAG_EXTRASAMPLES, 1, extra_samples)) return 0;
    }
    return 1;
}

H2TStatus h2t_write_tiff(const char *path, const H2TImage *image) {
    if (path == NULL || !h2t_valid_tiff_image(image)) return H2T_ERR_WRITE;

    TIFF *tif = TIFFOpen(path, "w");
    if (tif == NULL) return H2T_ERR_WRITE;

    if (!h2t_set_required_tiff_fields(tif, image)) {
        TIFFClose(tif);
        return H2T_ERR_WRITE;
    }

    for (int row = 0; row < image->height; ++row) {
        const unsigned char *ptr = image->pixels + (size_t)row * image->stride;
        if (TIFFWriteScanline(tif, (void *)ptr, (uint32_t)row, 0) < 0) {
            TIFFClose(tif);
            return H2T_ERR_WRITE;
        }
    }
    TIFFClose(tif);
    return H2T_OK;
}
