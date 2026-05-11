#include "cli.h"
#include "error.h"
#include "heic_decode.h"
#include "pixel_convert.h"
#include "tiff_write.h"
#include <stdio.h>

int main(int argc, char **argv) {
    H2TOptions opts;
    H2TStatus st = h2t_parse_args(argc, argv, &opts);
    if (st != H2T_OK) {
        fprintf(stderr, "heic2tiff: %s\n", h2t_status_string(st));
        h2t_fprint_help(stderr, argv[0]);
        return st;
    }
    if (opts.show_help) {
        h2t_print_help(argv[0]);
        return H2T_OK;
    }
    if (opts.show_version) {
        h2t_print_version();
        return H2T_OK;
    }

    H2TImage image;
    st = h2t_decode_heic(opts.input_path, &image);
    if (st != H2T_OK) {
        fprintf(stderr, "heic2tiff: %s: %s\n", opts.input_path, h2t_status_string(st));
        return st;
    }

    st = h2t_prepare_pixels_for_tiff(&image);
    if (st == H2T_OK) st = h2t_write_tiff(opts.output_path, &image);
    h2t_image_free(&image);

    if (st != H2T_OK) {
        fprintf(stderr, "heic2tiff: %s\n", h2t_status_string(st));
        return st;
    }
    return H2T_OK;
}
