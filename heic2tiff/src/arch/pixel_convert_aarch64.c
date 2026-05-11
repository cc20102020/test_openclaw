#include "pixel_convert.h"

void h2t_pixel_copy_aarch64(unsigned char *dst, const unsigned char *src, unsigned long n) {
#if defined(__aarch64__) && defined(__GNUC__)
    /* Placeholder for safe future NEON/asm helpers; generic C is used today. */
    h2t_pixel_copy_generic(dst, src, n);
#else
    h2t_pixel_copy_generic(dst, src, n);
#endif
}
