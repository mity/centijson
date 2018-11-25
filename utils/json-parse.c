/*
 * CentiJSON
 * <http://github.com/mity/centijson>
 *
 * Copyright (c) 2018 Martin Mitas
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "cmdline.h"
#include "json-dom.h"
#include "json-err.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static const char* output_path = NULL;
static const char* input_path = NULL;
static int minimize = 0;
static const char* argv0;


static void
print_usage(void)
{
    printf("Usage: %s [OPTION]... [FILE]\n", argv0);
    printf("Parse and write down JSON file.\n");
    printf("  -h, --help             %s\n", "Display this help and exit");

    printf("\n");
    printf("Disclaimer: This utility is more meant for purposes like testing, ");
    printf("benchmarking, or providing an example code rather then as a serious ");
    printf("generally useful utility.\n");

    exit(EXIT_SUCCESS);
}
static const CMDLINE_OPTION cmdline_options[] = {
    { 'o',  "output",       'o', CMDLINE_OPTFLAG_REQUIREDARG },
    { 'm',  "minimize",     'm', 0 },
    { 'h',  "help",         'h', 0 },
    { 0 }
};

static int
cmdline_callback(int id, const char* arg, void* userdata)
{
    switch(id) {
        /* Options */
        case 'o':       output_path = arg; break;
        case 'm':       minimize = 1; break;
        case 'h':       print_usage(); break;

        /* Non-option arguments */
        case 0:
            if(input_path) {
                fprintf(stderr, "Too many arguments. Only one input file can be specified.\n");
                fprintf(stderr, "Use --help for more info.\n");
                exit(1);
            }
            input_path = arg;
            break;

        /* Errors */
        case CMDLINE_OPTID_UNKNOWN:
            fprintf(stderr, "Unrecognized command line option '%s'.", arg);
            exit(EXIT_FAILURE);
        case CMDLINE_OPTID_MISSINGARG:
            fprintf(stderr, "The command line option '%s' requires an argument.", arg);
            exit(EXIT_FAILURE);
        case CMDLINE_OPTID_BOGUSARG:
            fprintf(stderr, "The command line option '%s' does not expect an argument.", arg);
            exit(EXIT_FAILURE);
    }

    return 0;
}


static int
write_callback(const char* data, size_t size, void* userdata)
{
    FILE* out = (FILE*) userdata;

    if(fwrite(data, size, 1, out) == 0) {
        fprintf(stderr, "Output error.\n");
        return -1;
    }

    return 0;
}

#define BUFFER_SIZE     4096

static int
process_file(FILE* in, FILE* out)
{
    JSON_DOM_PARSER parser;
    JSON_INPUT_POS pos;
    VALUE root;
    char* buffer;
    size_t n;
    int ret = -1;
    unsigned dom_flags;

    buffer = (char*) malloc(BUFFER_SIZE);
    if(buffer == NULL) {
        fprintf(stderr, "Out of memory.\n");
        goto err_malloc;
    }

    if(json_dom_init(&parser, NULL, 0) != 0)
        goto err_init;

    while(1) {
        n = fread(buffer, 1, BUFFER_SIZE, in);
        if(n == 0)
            break;

        if(json_dom_feed(&parser, buffer, n) != 0)
            break;
    }

    ret = json_dom_fini(&parser, &root, &pos);
    if(ret != 0) {
        json_err(ret, &pos);
        goto err_parse;
    }

    dom_flags = (minimize ? JSON_DOM_DUMP_MINIMIZE : 0);
    if(json_dom_dump(&root, write_callback, (void*) out, 0, dom_flags) != 0)
        goto err_dump;
    ret = 0;

err_dump:
    value_fini(&root);
err_parse:
err_init:
    free(buffer);
err_malloc:
    return ret;
}

int
main(int argc, char** argv)
{
    FILE* in = stdin;
    FILE* out = stdout;
    int ret = -1;

    argv0 = argv[0];

    cmdline_read(cmdline_options, argc, argv, cmdline_callback, NULL);

    if(input_path != NULL && strcmp(input_path, "-") != 0) {
        in = fopen(input_path, "rb");
        if(in == NULL) {
            fprintf(stderr, "Cannot open %s.\n", input_path);
            exit(1);
        }
    }
    if(output_path != NULL && strcmp(output_path, "-") != 0) {
        out = fopen(output_path, "wt");
        if(out == NULL) {
            fprintf(stderr, "Cannot open %s.\n", input_path);
            exit(1);
        }
    }

    ret = process_file(in, out);

    if(in != stdin)
        fclose(in);
    if(out != stdout)
        fclose(out);

    return (ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

