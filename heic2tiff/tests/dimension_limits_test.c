#include "heic_decode.h"
#include <stdio.h>

static int expect_supported(int width, int height, int expected) {
    int actual = h2t_dimensions_supported(width, height);
    if (actual != expected) {
        fprintf(stderr, "expected %dx%d support=%d, got %d\n", width, height, expected, actual);
        return 1;
    }
    return 0;
}

int main(void) {
    int failed = 0;
    failed |= expect_supported(4032, 3024, 1); /* common iPhone 12MP HEIC */
    failed |= expect_supported(3840, 2160, 1); /* UHD 4K */
    failed |= expect_supported(4096, 4096, 1); /* maximum supported square */
    failed |= expect_supported(4097, 2160, 0);
    failed |= expect_supported(3840, 4097, 0);
    failed |= expect_supported(0, 2160, 0);
    failed |= expect_supported(-1, 2160, 0);
    return failed ? 1 : 0;
}
