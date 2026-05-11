#include "cli.h"
#include <stdio.h>
#include <string.h>

H2TStatus h2t_parse_args(int argc, char **argv, H2TOptions *options) {
    if (options == NULL) return H2T_ERR_ARGS;
    options->input_path = NULL;
    options->output_path = NULL;
    options->show_help = false;
    options->show_version = false;

    if (argc == 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        options->show_help = true;
        return H2T_OK;
    }
    if (argc == 2 && (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-V") == 0)) {
        options->show_version = true;
        return H2T_OK;
    }
    if (argc != 3) return H2T_ERR_ARGS;
    options->input_path = argv[1];
    options->output_path = argv[2];
    if (options->input_path[0] == '\0' || options->output_path[0] == '\0') return H2T_ERR_ARGS;
    if (options->input_path[0] == '-' || options->output_path[0] == '-') return H2T_ERR_ARGS;
    if (strcmp(options->input_path, options->output_path) == 0) return H2T_ERR_ARGS;
    return H2T_OK;
}

void h2t_fprint_help(FILE *stream, const char *argv0) {
    if (stream == NULL) stream = stdout;
    if (argv0 == NULL || argv0[0] == '\0') argv0 = "heic2tiff";
    fprintf(stream, "Usage:\n");
    fprintf(stream, "  %s input.heic output.tiff\n", argv0);
    fprintf(stream, "  %s --help | -h\n", argv0);
    fprintf(stream, "  %s --version | -V\n", argv0);
}

void h2t_print_help(const char *argv0) {
    h2t_fprint_help(stdout, argv0);
}

void h2t_print_version(void) {
    printf("heic2tiff 0.1.0\n");
}
