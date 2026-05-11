#include "error.h"

const char *h2t_status_string(H2TStatus status) {
    switch (status) {
    case H2T_OK: return "ok";
    case H2T_ERR_ARGS: return "invalid arguments";
    case H2T_ERR_DECODE: return "HEIC decode failed";
    case H2T_ERR_CONVERT: return "pixel conversion failed";
    case H2T_ERR_WRITE: return "TIFF write failed";
    case H2T_ERR_ALLOC: return "memory allocation failed";
    case H2T_ERR_TOO_LARGE: return "image exceeds supported 4K limit";
    default: return "unknown error";
    }
}
