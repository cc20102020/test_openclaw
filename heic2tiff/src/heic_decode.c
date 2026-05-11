#include "heic_decode.h"
#include <dlfcn.h>
#include <limits.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* Minimal libheif ABI declarations used through dlopen/dlsym. This removes
 * compile/link-time dependency on libheif headers and libraries while still
 * using libheif at runtime when HEIC decoding is requested. */
struct heif_context;
struct heif_image_handle;
struct heif_image;
struct heif_error {
    int code;
    int subcode;
    const char *message;
};

typedef struct heif_context *(*fn_context_alloc)(void);
typedef void (*fn_context_free)(struct heif_context *);
typedef struct heif_error (*fn_context_read_from_file)(struct heif_context *, const char *, const void *);
typedef struct heif_error (*fn_context_get_primary_image_handle)(struct heif_context *, struct heif_image_handle **);
typedef int (*fn_image_handle_get_width)(const struct heif_image_handle *);
typedef int (*fn_image_handle_get_height)(const struct heif_image_handle *);
typedef int (*fn_image_handle_has_alpha_channel)(const struct heif_image_handle *);
typedef void (*fn_image_handle_release)(const struct heif_image_handle *);
typedef struct heif_error (*fn_decode_image)(const struct heif_image_handle *, struct heif_image **, int, int, const void *);
typedef int (*fn_image_get_width)(const struct heif_image *, int);
typedef int (*fn_image_get_height)(const struct heif_image *, int);
typedef const uint8_t *(*fn_image_get_plane_readonly)(const struct heif_image *, int, int *);
typedef void (*fn_image_release)(const struct heif_image *);
typedef void (*fn_deinit)(void);

typedef struct {
    void *handle;
    fn_context_alloc context_alloc;
    fn_context_free context_free;
    fn_context_read_from_file context_read_from_file;
    fn_context_get_primary_image_handle context_get_primary_image_handle;
    fn_image_handle_get_width image_handle_get_width;
    fn_image_handle_get_height image_handle_get_height;
    fn_image_handle_has_alpha_channel image_handle_has_alpha_channel;
    fn_image_handle_release image_handle_release;
    fn_decode_image decode_image;
    fn_image_get_width image_get_width;
    fn_image_get_height image_get_height;
    fn_image_get_plane_readonly image_get_plane_readonly;
    fn_image_release image_release;
    fn_deinit deinit;
} H2THeifApi;

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

static int load_symbol(void *handle, const char *name, void **out) {
    *out = dlsym(handle, name);
    return *out != NULL;
}

static int load_heif_api(H2THeifApi *api) {
    static const char *candidates[] = { "libheif.so.1", "libheif.so", NULL };
    memset(api, 0, sizeof(*api));
    for (int i = 0; candidates[i] != NULL && api->handle == NULL; ++i) {
        api->handle = dlopen(candidates[i], RTLD_NOW | RTLD_LOCAL);
    }
    if (api->handle == NULL) return 0;

#define LOAD(field, symbol) load_symbol(api->handle, symbol, (void **)&api->field)
    int ok = LOAD(context_alloc, "heif_context_alloc") &&
             LOAD(context_free, "heif_context_free") &&
             LOAD(context_read_from_file, "heif_context_read_from_file") &&
             LOAD(context_get_primary_image_handle, "heif_context_get_primary_image_handle") &&
             LOAD(image_handle_get_width, "heif_image_handle_get_width") &&
             LOAD(image_handle_get_height, "heif_image_handle_get_height") &&
             LOAD(image_handle_has_alpha_channel, "heif_image_handle_has_alpha_channel") &&
             LOAD(image_handle_release, "heif_image_handle_release") &&
             LOAD(decode_image, "heif_decode_image") &&
             LOAD(image_get_width, "heif_image_get_width") &&
             LOAD(image_get_height, "heif_image_get_height") &&
             LOAD(image_get_plane_readonly, "heif_image_get_plane_readonly") &&
             LOAD(image_release, "heif_image_release") &&
             LOAD(deinit, "heif_deinit");
#undef LOAD
    if (!ok) {
        dlclose(api->handle);
        memset(api, 0, sizeof(*api));
    }
    return ok;
}

static void close_heif_api(H2THeifApi *api) {
    if (api->handle != NULL) dlclose(api->handle);
    memset(api, 0, sizeof(*api));
}

H2TStatus h2t_decode_heic(const char *path, H2TImage *out_image) {
    if (path == NULL || out_image == NULL) return H2T_ERR_DECODE;
    memset(out_image, 0, sizeof(*out_image));

    H2THeifApi api;
    if (!load_heif_api(&api)) return H2T_ERR_DECODE;

    H2TStatus status = H2T_ERR_DECODE;
    struct heif_context *ctx = NULL;
    struct heif_image_handle *handle = NULL;
    struct heif_image *img = NULL;

    ctx = api.context_alloc();
    if (ctx == NULL) {
        status = H2T_ERR_ALLOC;
        goto cleanup;
    }

    struct heif_error err = api.context_read_from_file(ctx, path, NULL);
    if (err.code != HEIF_ERROR_OK) goto cleanup;

    err = api.context_get_primary_image_handle(ctx, &handle);
    if (err.code != HEIF_ERROR_OK) goto cleanup;

    int handle_width = api.image_handle_get_width(handle);
    int handle_height = api.image_handle_get_height(handle);
    if (!h2t_dimensions_supported(handle_width, handle_height)) {
        status = H2T_ERR_TOO_LARGE;
        goto cleanup;
    }

    int has_alpha = api.image_handle_has_alpha_channel(handle);
    int chroma = has_alpha ? HEIF_CHROMA_INTERLEAVED_RGBA : HEIF_CHROMA_INTERLEAVED_RGB;
    int channels = has_alpha ? 4 : 3;

    err = api.decode_image(handle, &img, HEIF_COLORSPACE_RGB, chroma, NULL);
    if (err.code != HEIF_ERROR_OK || img == NULL) goto cleanup;

    int width = api.image_get_width(img, HEIF_CHANNEL_INTERLEAVED);
    int height = api.image_get_height(img, HEIF_CHANNEL_INTERLEAVED);
    size_t total = 0;
    if (!h2t_dimensions_supported(width, height)) {
        status = H2T_ERR_TOO_LARGE;
        goto cleanup;
    }
    if (!checked_size(width, height, channels, &total)) goto cleanup;

    size_t row_bytes = (size_t)width * (size_t)channels;
    if (row_bytes > (size_t)INT_MAX) goto cleanup;

    int src_stride = 0;
    const uint8_t *src = api.image_get_plane_readonly(img, HEIF_CHANNEL_INTERLEAVED, &src_stride);
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
    if (img != NULL) api.image_release(img);
    if (handle != NULL) api.image_handle_release(handle);
    if (ctx != NULL) api.context_free(ctx);
    if (api.deinit != NULL) api.deinit();
    close_heif_api(&api);
    return status;
}

void h2t_image_free(H2TImage *image) {
    if (image == NULL) return;
    free(image->pixels);
    image->pixels = NULL;
    image->width = image->height = image->channels = 0;
    image->stride = 0;
}
