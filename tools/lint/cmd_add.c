/**
 * @file cmd_add.c
 * @author Michal Vasko <mvasko@cesnet.cz>
 * @author Radek Krejci <rkrejci@cesnet.cz>
 * @brief 'add' command of the libyang's yanglint tool.
 *
 * Copyright (c) 2015-2020 CESNET, z.s.p.o.
 *
 * This source code is licensed under BSD 3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://opensource.org/licenses/BSD-3-Clause
 */

#define _GNU_SOURCE

#include "cmd.h"

#include <errno.h>
#include <getopt.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libyang.h"

#include "common.h"

void
cmd_add_help(void)
{
    printf("Usage: add [-iD] <schema1> [<schema2> ...]\n"
            "                  Add a new module from a specific file.\n\n"
            "  -D, --disable-searchdir\n"
            "                  Do not implicitly search in current working directory for\n"
            "                  the import schema modules. If specified a second time, do not\n"
            "                  even search in the module directory (all modules must be \n"
            "                  explicitly specified).\n"
            "  -F FEATURES, --features=FEATURES\n"
            "                  Features to support, default all.\n"
            "                  <modname>:[<feature>,]*\n"
            "  -i, --makeimplemented\n"
            "                  Make the imported modules \"referenced\" from any loaded\n"
            "                  <schema> module also implemented. If specified a second time,\n"
            "                  all the modules are set implemented.\n");
}

void
cmd_add(struct ly_ctx **ctx, const char *cmdline)
{

    int argc = 0;
    char **argv = NULL;
    int opt, opt_index;
    struct option options[] = {
        {"disable-searchdir", no_argument, NULL, 'D'},
        {"features", required_argument, NULL, 'F'},
        {"help", no_argument, NULL, 'h'},
        {"makeimplemented", no_argument, NULL, 'i'},
        {NULL, 0, NULL, 0}
    };
    uint16_t options_ctx = 0;
    struct schema_features *features = NULL;
    uint32_t features_count = 0;

    if (parse_cmdline(cmdline, &argc, &argv)) {
        goto cleanup;
    }

    while ((opt = getopt_long(argc, argv, "D:F:hi", options, &opt_index)) != -1) {
        switch (opt) {
        case 'D': /* --disable--search */
            if (options_ctx & LY_CTX_DISABLE_SEARCHDIRS) {
                fprintf(stderr, "yanglint warning: -D specified too many times.\n");
            }
            if (options_ctx & LY_CTX_DISABLE_SEARCHDIR_CWD) {
                options_ctx &= ~LY_CTX_DISABLE_SEARCHDIR_CWD;
                options_ctx |= LY_CTX_DISABLE_SEARCHDIRS;
            } else {
                options_ctx |= LY_CTX_DISABLE_SEARCHDIR_CWD;
            }
            break;

        case 'F': /* --features */
            if (parse_features(optarg, &features, &features_count)) {
                goto cleanup;
            }
            break;

        case 'h':
            cmd_add_help();
            goto cleanup;

        case 'i': /* --makeimplemented */
            if (options_ctx & LY_CTX_REF_IMPLEMENTED) {
                options_ctx &= ~LY_CTX_REF_IMPLEMENTED;
                options_ctx |= LY_CTX_ALL_IMPLEMENTED;
            } else {
                options_ctx |= LY_CTX_REF_IMPLEMENTED;
            }
            break;

        default:
            fprintf(stderr, "Unknown option.\n");
            goto cleanup;
        }
    }

    if (argc == optind) {
        /* no argument */
        cmd_add_help();
        goto cleanup;
    }

    if (options_ctx) {
        ly_ctx_set_options(*ctx, options_ctx);
    }

    for (int i = 0; i < argc - optind; i++) {
        /* process the schema module files */
        LY_ERR ret;
        uint8_t path_unset = 1; /* flag to unset the path from the searchpaths list (if not already present) */
        char *dir, *module;
        const char **flist = NULL;
        struct ly_in *in = NULL;

        if (parse_schema_path(argv[optind + i], &dir, &module)) {
            goto cleanup;
        }

        /* add temporarily also the path of the module itself */
        dir = strdup(argv[optind + i]);
        if (ly_ctx_set_searchdir(*ctx, dirname(dir)) == LY_EEXIST) {
            path_unset = 0;
        }

        /* get features list for this module */
        for (uint32_t u = 0; u < features_count; ++u) {
            if (!strcmp(module, features[u].module)) {
                flist = (const char **)features[u].features;
                break;
            }
        }

        /* temporary cleanup */
        free(dir);
        free(module);

        /* prepare input handler */
        ret = ly_in_new_filepath(argv[optind + i], 0, &in);
        if (ret) {
            goto cleanup;
        }

        /* parse the file */
        ret = lys_parse(*ctx, in, LYS_IN_UNKNOWN, flist, NULL);
        ly_in_free(in, 1);
        ly_ctx_unset_searchdir_last(*ctx, path_unset);

        if (ret) {
            /* libyang printed the error messages */
            goto cleanup;
        }
    }

cleanup:
    if (options_ctx) {
        ly_ctx_unset_options(*ctx, options_ctx);
    }
    free_features(features, features_count);
    free_cmdline(argv);
}
