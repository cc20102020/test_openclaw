#ifndef HEIC2TIFF_CLI_H
#define HEIC2TIFF_CLI_H

#include <stdbool.h>
#include <stdio.h>
#include "error.h"

typedef struct {
    const char *input_path;
    const char *output_path;
    bool show_help;
    bool show_version;
} H2TOptions;

H2TStatus h2t_parse_args(int argc, char **argv, H2TOptions *options);
void h2t_fprint_help(FILE *stream, const char *argv0);
void h2t_print_help(const char *argv0);
void h2t_print_version(void);

#endif
