#ifndef HEIC2TIFF_ERROR_H
#define HEIC2TIFF_ERROR_H

typedef enum {
    H2T_OK = 0,
    H2T_ERR_ARGS = 2,
    H2T_ERR_DECODE = 3,
    H2T_ERR_CONVERT = 4,
    H2T_ERR_WRITE = 5,
    H2T_ERR_ALLOC = 6,
    H2T_ERR_TOO_LARGE = 7
} H2TStatus;

const char *h2t_status_string(H2TStatus status);

#endif
