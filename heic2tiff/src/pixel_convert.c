#include "pixel_convert.h"

H2TStatus h2t_prepare_pixels_for_tiff(const H2TImage *image) {
    if (image == NULL || image->pixels == NULL) return H2T_ERR_CONVERT;
    if (image->width <= 0 || image->height <= 0) return H2T_ERR_CONVERT;
    if (image->channels != 3 && image->channels != 4) return H2T_ERR_CONVERT;
    return H2T_OK;
}
