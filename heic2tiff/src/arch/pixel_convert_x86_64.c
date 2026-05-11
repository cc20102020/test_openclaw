#include "pixel_convert.h"

void h2t_pixel_copy_x86_64(unsigned char *dst, const unsigned char *src, unsigned long n) {
#if defined(__x86_64__) && defined(__GNUC__)
    /* Deliberately tiny optional boundary. Generic C remains correctness path. */
    h2t_pixel_copy_generic(dst, src, n);
#else
    h2t_pixel_copy_generic(dst, src, n);
#endif
}
