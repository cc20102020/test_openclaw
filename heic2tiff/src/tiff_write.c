#include "tiff_write.h"
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define TIFF_TYPE_SHORT 3u
#define TIFF_TYPE_LONG 4u
#define TIFF_TYPE_RATIONAL 5u

#define TIFFTAG_IMAGEWIDTH 256u
#define TIFFTAG_IMAGELENGTH 257u
#define TIFFTAG_BITSPERSAMPLE 258u
#define TIFFTAG_COMPRESSION 259u
#define TIFFTAG_PHOTOMETRIC 262u
#define TIFFTAG_STRIPOFFSETS 273u
#define TIFFTAG_SAMPLESPERPIXEL 277u
#define TIFFTAG_ROWSPERSTRIP 278u
#define TIFFTAG_STRIPBYTECOUNTS 279u
#define TIFFTAG_XRESOLUTION 282u
#define TIFFTAG_YRESOLUTION 283u
#define TIFFTAG_PLANARCONFIG 284u
#define TIFFTAG_RESOLUTIONUNIT 296u
#define TIFFTAG_EXTRASAMPLES 338u

static int h2t_valid_tiff_image(const H2TImage *image, size_t *row_bytes, size_t *image_bytes) {
    if (image == NULL || image->pixels == NULL || row_bytes == NULL || image_bytes == NULL) return 0;
    if (image->width <= 0 || image->height <= 0) return 0;
    if (image->channels != 3 && image->channels != 4) return 0;

    size_t width = (size_t)image->width;
    size_t height = (size_t)image->height;
    size_t channels = (size_t)image->channels;
    if (width > SIZE_MAX / channels) return 0;
    *row_bytes = width * channels;
    if (image->stride < *row_bytes) return 0;
    if (height > SIZE_MAX / *row_bytes) return 0;
    *image_bytes = height * *row_bytes;
    if (*image_bytes > UINT32_MAX) return 0;
    return 1;
}

static int write_u16(FILE *fp, uint16_t value) {
    unsigned char b[2] = { (unsigned char)(value & 0xffu), (unsigned char)((value >> 8) & 0xffu) };
    return fwrite(b, 1, sizeof(b), fp) == sizeof(b);
}

static int write_u32(FILE *fp, uint32_t value) {
    unsigned char b[4] = {
        (unsigned char)(value & 0xffu),
        (unsigned char)((value >> 8) & 0xffu),
        (unsigned char)((value >> 16) & 0xffu),
        (unsigned char)((value >> 24) & 0xffu)
    };
    return fwrite(b, 1, sizeof(b), fp) == sizeof(b);
}

static int write_ifd_entry(FILE *fp, uint16_t tag, uint16_t type, uint32_t count, uint32_t value_or_offset) {
    return write_u16(fp, tag) && write_u16(fp, type) && write_u32(fp, count) && write_u32(fp, value_or_offset);
}

static uint32_t pack_short(uint16_t value) {
    return (uint32_t)value;
}

static int write_zeros(FILE *fp, size_t count) {
    static const unsigned char zeros[8] = {0};
    while (count > 0) {
        size_t chunk = count < sizeof(zeros) ? count : sizeof(zeros);
        if (fwrite(zeros, 1, chunk, fp) != chunk) return 0;
        count -= chunk;
    }
    return 1;
}

H2TStatus h2t_write_tiff(const char *path, const H2TImage *image) {
    size_t row_bytes = 0;
    size_t image_bytes = 0;
    if (path == NULL || !h2t_valid_tiff_image(image, &row_bytes, &image_bytes)) return H2T_ERR_WRITE;

    const uint16_t entry_count = (uint16_t)(image->channels == 4 ? 13 : 12);
    const uint32_t ifd_offset = 8u;
    const uint32_t ifd_bytes = 2u + (uint32_t)entry_count * 12u + 4u;
    uint32_t extra_offset = ifd_offset + ifd_bytes;
    const uint32_t bits_offset = extra_offset;
    extra_offset += (uint32_t)image->channels * 2u;
    if (extra_offset & 1u) extra_offset++;
    const uint32_t xres_offset = extra_offset;
    extra_offset += 8u;
    const uint32_t yres_offset = extra_offset;
    extra_offset += 8u;
    const uint32_t pixel_offset = (extra_offset + 1u) & ~1u;

    FILE *fp = fopen(path, "wb");
    if (fp == NULL) return H2T_ERR_WRITE;

    int ok = 1;
    ok = ok && fwrite("II", 1, 2, fp) == 2;
    ok = ok && write_u16(fp, 42u);
    ok = ok && write_u32(fp, ifd_offset);

    ok = ok && write_u16(fp, entry_count);
    ok = ok && write_ifd_entry(fp, TIFFTAG_IMAGEWIDTH, TIFF_TYPE_LONG, 1u, (uint32_t)image->width);
    ok = ok && write_ifd_entry(fp, TIFFTAG_IMAGELENGTH, TIFF_TYPE_LONG, 1u, (uint32_t)image->height);
    ok = ok && write_ifd_entry(fp, TIFFTAG_BITSPERSAMPLE, TIFF_TYPE_SHORT, (uint32_t)image->channels, bits_offset);
    ok = ok && write_ifd_entry(fp, TIFFTAG_COMPRESSION, TIFF_TYPE_SHORT, 1u, pack_short(1u));
    ok = ok && write_ifd_entry(fp, TIFFTAG_PHOTOMETRIC, TIFF_TYPE_SHORT, 1u, pack_short(2u));
    ok = ok && write_ifd_entry(fp, TIFFTAG_STRIPOFFSETS, TIFF_TYPE_LONG, 1u, pixel_offset);
    ok = ok && write_ifd_entry(fp, TIFFTAG_SAMPLESPERPIXEL, TIFF_TYPE_SHORT, 1u, pack_short((uint16_t)image->channels));
    ok = ok && write_ifd_entry(fp, TIFFTAG_ROWSPERSTRIP, TIFF_TYPE_LONG, 1u, (uint32_t)image->height);
    ok = ok && write_ifd_entry(fp, TIFFTAG_STRIPBYTECOUNTS, TIFF_TYPE_LONG, 1u, (uint32_t)image_bytes);
    ok = ok && write_ifd_entry(fp, TIFFTAG_XRESOLUTION, TIFF_TYPE_RATIONAL, 1u, xres_offset);
    ok = ok && write_ifd_entry(fp, TIFFTAG_YRESOLUTION, TIFF_TYPE_RATIONAL, 1u, yres_offset);
    ok = ok && write_ifd_entry(fp, TIFFTAG_PLANARCONFIG, TIFF_TYPE_SHORT, 1u, pack_short(1u));
    if (image->channels == 4) {
        ok = ok && write_ifd_entry(fp, TIFFTAG_EXTRASAMPLES, TIFF_TYPE_SHORT, 1u, pack_short(2u));
    }
    ok = ok && write_u32(fp, 0u);

    for (int i = 0; ok && i < image->channels; ++i) ok = write_u16(fp, 8u);
    long pos = ftell(fp);
    if (ok && pos >= 0 && ((uint32_t)pos < xres_offset)) ok = write_zeros(fp, (size_t)(xres_offset - (uint32_t)pos));
    ok = ok && write_u32(fp, 72u) && write_u32(fp, 1u);
    ok = ok && write_u32(fp, 72u) && write_u32(fp, 1u);
    pos = ftell(fp);
    if (ok && pos >= 0 && ((uint32_t)pos < pixel_offset)) ok = write_zeros(fp, (size_t)(pixel_offset - (uint32_t)pos));

    for (int row = 0; ok && row < image->height; ++row) {
        const unsigned char *ptr = image->pixels + (size_t)row * image->stride;
        ok = fwrite(ptr, 1, row_bytes, fp) == row_bytes;
    }

    if (fclose(fp) != 0) ok = 0;
    return ok ? H2T_OK : H2T_ERR_WRITE;
}
