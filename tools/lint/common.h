/**
 * @file main.c
 * @author Radek Krejci <rkrejci@cesnet.cz>
 * @brief libyang's yanglint tool - common functions and definitions for both interactive and non-interactive mode.
 *
 * Copyright (c) 2020 CESNET, z.s.p.o.
 *
 * This source code is licensed under BSD 3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef COMMON_H_
#define COMMON_H_

#include "libyang.h"

#ifdef __GNUC__
#  define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
#else
#  define UNUSED(x) UNUSED_ ## x
#endif

#define PROMPT "> "

struct schema_features {
    char *module;
    char **features;
} *schemas_features;

void free_features(struct schema_features *flist, uint32_t fcounter);

int parse_features(const char *fstring, struct schema_features **flist_p, uint32_t *fcounter_p);

int parse_schema_path(const char *path, char **dir, char **module);

/**
 * @brief Helper function to prepare argc, argv pair from a command line string.
 *
 * @param[in] cmdline Complete command line string.
 * @param[out] argc_p Pointer to store argc value.
 * @param[out] argv_p Pointer to store argv vector.
 * @return 0 on success, non-zero on failure.
 */
int parse_cmdline(const char *cmdline, int *argc_p, char **argv_p[]);

/**
 * @brief Destructor for the argument vector prepared by ::parse_cmdline().
 *
 * @param[in,out] argv Argument vector to destroy.
 */
void free_cmdline(char *argv[]);

/**
 * @brief Get expected format of the @p filename's content according to the @p filename's suffix.
 * @param[in] filename Name of the file to examine.
 * @param[out] schema Pointer to a variable to store the expected input schema format. Do not provide the pointer in case a
 * schema format is not expected.
 * @param[out] data Pointer to a variable to store the expected input data format. Do not provide the pointer in case a data
 * format is not expected.
 * @return zero in case a format was successfully detected.
 * @return nonzero in case it is not possible to get valid format from the @p filename.
 */
int get_format(const char *filename, LYS_INFORMAT *schema, LYD_FORMAT *data);

/**
 * @brief Print list of schemas in the context.
 *
 * @param[in] out Output handler where to print.
 * @param[in] ctx Context to print.
 * @param[in] outformat Optional output format. If not specified (:LYD_UNKNOWN), a simple list with single module per line
 * is printed. Otherwise, the ietf-yang-library data are printed in the specified format.
 * @return zero in case the data successfully printed.
 * @return nonzero in case of error.
 */
int print_list(struct ly_out *out, struct ly_ctx *ctx, LYD_FORMAT outformat);

#endif /* COMMON_H_ */
