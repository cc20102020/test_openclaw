#include "pixel_convert.h"
#include <string.h>

void h2t_pixel_copy_generic(unsigned char *dst, const unsigned char *src, unsigned long n) {
    if (dst == NULL || src == NULL || n == 0) return;
    memcpy(dst, src, (size_t)n);
}
