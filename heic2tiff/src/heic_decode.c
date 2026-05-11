#include "heic_decode.h"
#include <limits.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* Minimal libheif ABI declarations. The static build links libheif/libde265
 * into the final executable, so the command line tool does not load codec
 * libraries dynamically at runtime. */
struct heif_context;
struct heif_image_handle;
struct heif_image;
struct heif_error {
    int code;
    int subcode;
    const char *message;
};

extern struct heif_context *heif_context_alloc(void);
extern void heif_context_free(struct heif_context *);
extern struct heif_error heif_context_read_from_file(struct heif_context *, const char *, const void *);
extern struct heif_error heif_context_get_primary_image_handle(struct heif_context *, struct heif_image_handle **);
extern int heif_image_handle_get_width(const struct heif_image_handle *);
extern int heif_image_handle_get_height(const struct heif_image_handle *);
extern int heif_image_handle_has_alpha_channel(const struct heif_image_handle *);
extern void heif_image_handle_release(const struct heif_image_handle *);
extern struct heif_error heif_decode_image(const struct heif_image_handle *, struct heif_image **, int, int, const void *);
extern int heif_image_get_width(const struct heif_image *, int);
extern int heif_image_get_height(const struct heif_image *, int);
extern const uint8_t *heif_image_get_plane_readonly(const struct heif_image *, int, int *);
extern void heif_image_release(const struct heif_image *);
extern void heif_deinit(void);

#define HEIF_ERROR_OK 0
#define HEIF_COLORSPACE_RGB 1
#define HEIF_CHROMA_INTERLEAVED_RGB 10
#define HEIF_CHROMA_INTERLEAVED_RGBA 11
#define HEIF_CHANNEL_INTERLEAVED 10

int h2t_dimensions_supported(int width, int height) {
    if (width <= 0 || height <= 0) return 0;
    if (width > H2T_MAX_DIMENSION || height > H2T_MAX_DIMENSION) return 0;
    return (size_t)width <= H2T_MAX_PIXELS / (size_t)height;
}

static int checked_size(int width, int height, int channels, size_t *out) {
    if (width <= 0 || height <= 0 || channels <= 0 || out == NULL) return 0;
    size_t w = (size_t)width, h = (size_t)height, c = (size_t)channels;
    if (w > SIZE_MAX / h || w * h > SIZE_MAX / c) return 0;
    *out = w * h * c;
    return 1;
}

H2TStatus h2t_decode_heic(const char *path, H2TImage *out_image) {
    if (path == NULL || out_image == NULL) return H2T_ERR_DECODE;
    memset(out_image, 0, sizeof(*out_image));

    H2TStatus status = H2T_ERR_DECODE;
    struct heif_context *ctx = NULL;
    struct heif_image_handle *handle = NULL;
    struct heif_image *img = NULL;

    ctx = heif_context_alloc();
    if (ctx == NULL) return H2T_ERR_ALLOC;

    struct heif_error err = heif_context_read_from_file(ctx, path, NULL);
    if (err.code != HEIF_ERROR_OK) goto cleanup;

    err = heif_context_get_primary_image_handle(ctx, &handle);
    if (err.code != HEIF_ERROR_OK) goto cleanup;

    int handle_width = heif_image_handle_get_width(handle);
    int handle_height = heif_image_handle_get_height(handle);
    if (!h2t_dimensions_supported(handle_width, handle_height)) {
        status = H2T_ERR_TOO_LARGE;
        goto cleanup;
    }

    int has_alpha = heif_image_handle_has_alpha_channel(handle);
    int chroma = has_alpha ? HEIF_CHROMA_INTERLEAVED_RGBA : HEIF_CHROMA_INTERLEAVED_RGB;
    int channels = has_alpha ? 4 : 3;

    err = heif_decode_image(handle, &img, HEIF_COLORSPACE_RGB, chroma, NULL);
    if (err.code != HEIF_ERROR_OK || img == NULL) goto cleanup;

    int width = heif_image_get_width(img, HEIF_CHANNEL_INTERLEAVED);
    int height = heif_image_get_height(img, HEIF_CHANNEL_INTERLEAVED);
    size_t total = 0;
    if (!h2t_dimensions_supported(width, height)) {
        status = H2T_ERR_TOO_LARGE;
        goto cleanup;
    }
    if (!checked_size(width, height, channels, &total)) goto cleanup;

    size_t row_bytes = (size_t)width * (size_t)channels;
    if (row_bytes > (size_t)INT_MAX) goto cleanup;

    int src_stride = 0;
    const uint8_t *src = heif_image_get_plane_readonly(img, HEIF_CHANNEL_INTERLEAVED, &src_stride);
    if (src == NULL || src_stride <= 0 || (size_t)src_stride < row_bytes) goto cleanup;

    uint8_t *dst = malloc(total);
    if (dst == NULL) {
        status = H2T_ERR_ALLOC;
        goto cleanup;
    }

    for (int y = 0; y < height; ++y) {
        memcpy(dst + (size_t)y * row_bytes, src + (size_t)y * (size_t)src_stride, row_bytes);
    }

    out_image->pixels = dst;
    out_image->width = width;
    out_image->height = height;
    out_image->channels = channels;
    out_image->stride = row_bytes;
    status = H2T_OK;

cleanup:
    if (img != NULL) heif_image_release(img);
    if (handle != NULL) heif_image_handle_release(handle);
    if (ctx != NULL) heif_context_free(ctx);
    heif_deinit();
    return status;
}

void h2t_image_free(H2TImage *image) {
    if (image == NULL) return;
    free(image->pixels);
    image->pixels = NULL;
    image->width = image->height = image->channels = 0;
    image->stride = 0;
}
