/**
 * @file cmd_data.c
 * @author Michal Vasko <mvasko@cesnet.cz>
 * @author Radek Krejci <rkrejci@cesnet.cz>
 * @brief 'data' command of the libyang's yanglint tool.
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
#include <stdio.h>
#include <string.h>

#include "libyang.h"

void
cmd_data_help(void)
{
    printf("data [-(-s)trict] [-t TYPE] [-d DEFAULTS] [-o <output-file>] [-f (xml | json | lyb)] [-r <running-file-name>]\n");
    printf("     <data-file-name> [<RPC/action-data-file-name> | <yang-data name>]\n\n");
    printf("Accepted TYPEs:\n");
    printf("\tauto       - resolve data type (one of the following) automatically (as pyang does),\n");
    printf("\t             this option is applicable only in case of XML input data.\n");
    printf("\tdata       - LYD_OPT_DATA (default value) - complete datastore including status data.\n");
    printf("\tconfig     - LYD_OPT_CONFIG - complete configuration datastore.\n");
    printf("\tget        - LYD_OPT_GET - <get> operation result.\n");
    printf("\tgetconfig  - LYD_OPT_GETCONFIG - <get-config> operation result.\n");
    printf("\tedit       - LYD_OPT_EDIT - <edit-config>'s data (content of its <config> element).\n");
    printf("\trpc        - LYD_OPT_RPC - NETCONF RPC message.\n");
    printf("\trpcreply   - LYD_OPT_RPCREPLY (last parameter mandatory in this case)\n");
    printf("\tnotif      - LYD_OPT_NOTIF - NETCONF Notification message.\n");
    printf("\tyangdata   - LYD_OPT_DATA_TEMPLATE - yang-data extension (last parameter mandatory in this case)\n\n");
    printf("Accepted DEFAULTS:\n");
    printf("\tall        - add missing default nodes\n");
    printf("\tall-tagged - add missing default nodes and mark all the default nodes with the attribute.\n");
    printf("\ttrim       - remove all nodes with a default value\n");
    printf("\timplicit-tagged    - add missing nodes and mark them with the attribute\n\n");
    printf("Option -r:\n");
    printf("\tOptional parameter for 'rpc', 'rpcreply' and 'notif' TYPEs, the file contains running\n");
    printf("\tconfiguration datastore data referenced from the RPC/Notification. Note that the file is\n");
    printf("\tvalidated as 'data' TYPE. Special value '!' can be used as argument to ignore the\n");
    printf("\texternal references.\n\n");
    printf("\tIf an XPath expression (when/must) needs access to configuration data, you can provide\n");
    printf("\tthem in a file, which will be parsed as 'data' TYPE.\n\n");
}

static int
parse_data(struct ly_ctx *ctx, char *filepath, int *options, const struct lyd_node *tree, const char *rpc_act_file,
        struct lyd_node **result)
{
    struct lyd_node *data = NULL, *rpc_act = NULL;
    int opts = *options;
    struct ly_in *in;

    if (ly_in_new_filepath(filepath, 0, &in)) {
        fprintf(stderr, "Unable to open input YANG data file \"%s\".", filepath);
        return EXIT_FAILURE;
    }

#if 0
    if ((opts & LYD_OPT_TYPEMASK) == LYD_OPT_TYPEMASK) {
        /* automatically detect data type from the data top level */
        if (informat != LYD_XML) {
            fprintf(stderr, "Only XML data can be automatically explored.\n");
            return EXIT_FAILURE;
        }

        xml = lyxml_parse_path(ctx, filepath, 0);
        if (!xml) {
            fprintf(stderr, "Failed to parse XML data for automatic type detection.\n");
            return EXIT_FAILURE;
        }

        /* NOTE: namespace is ignored to simplify usage of this feature */

        if (!strcmp(xml->name, "data")) {
            fprintf(stdout, "Parsing %s as complete datastore.\n", filepath);
            opts = (opts & ~LYD_OPT_TYPEMASK) | LYD_OPT_DATA_ADD_YANGLIB;
        } else if (!strcmp(xml->name, "config")) {
            fprintf(stdout, "Parsing %s as config data.\n", filepath);
            opts = (opts & ~LYD_OPT_TYPEMASK) | LYD_OPT_CONFIG;
        } else if (!strcmp(xml->name, "get-reply")) {
            fprintf(stdout, "Parsing %s as <get> reply data.\n", filepath);
            opts = (opts & ~LYD_OPT_TYPEMASK) | LYD_OPT_GET;
        } else if (!strcmp(xml->name, "get-config-reply")) {
            fprintf(stdout, "Parsing %s as <get-config> reply data.\n", filepath);
            opts = (opts & ~LYD_OPT_TYPEMASK) | LYD_OPT_GETCONFIG;
        } else if (!strcmp(xml->name, "edit-config")) {
            fprintf(stdout, "Parsing %s as <edit-config> data.\n", filepath);
            opts = (opts & ~LYD_OPT_TYPEMASK) | LYD_OPT_EDIT;
        } else if (!strcmp(xml->name, "rpc")) {
            fprintf(stdout, "Parsing %s as <rpc> data.\n", filepath);
            opts = (opts & ~LYD_OPT_TYPEMASK) | LYD_OPT_RPC;
        } else if (!strcmp(xml->name, "rpc-reply")) {
            if (!rpc_act_file) {
                fprintf(stderr, "RPC/action reply data require additional argument (file with the RPC/action).\n");
                lyxml_free(ctx, xml);
                return EXIT_FAILURE;
            }
            fprintf(stdout, "Parsing %s as <rpc-reply> data.\n", filepath);
            opts = (opts & ~LYD_OPT_TYPEMASK) | LYD_OPT_RPCREPLY;
            rpc_act = lyd_parse_path(ctx, rpc_act_file, informat, LYD_OPT_RPC, val_tree);
            if (!rpc_act) {
                fprintf(stderr, "Failed to parse RPC/action.\n");
                lyxml_free(ctx, xml);
                return EXIT_FAILURE;
            }
        } else if (!strcmp(xml->name, "notification")) {
            fprintf(stdout, "Parsing %s as <notification> data.\n", filepath);
            opts = (opts & ~LYD_OPT_TYPEMASK) | LYD_OPT_NOTIF;
        } else if (!strcmp(xml->name, "yang-data")) {
            fprintf(stdout, "Parsing %s as <yang-data> data.\n", filepath);
            opts = (opts & ~LYD_OPT_TYPEMASK) | LYD_OPT_DATA_TEMPLATE;
            if (!rpc_act_file) {
                fprintf(stderr, "YANG-DATA require additional argument (name instance of yang-data extension).\n");
                lyxml_free(ctx, xml);
                return EXIT_FAILURE;
            }
        } else {
            fprintf(stderr, "Invalid top-level element for automatic data type recognition.\n");
            lyxml_free(ctx, xml);
            return EXIT_FAILURE;
        }

        if (opts & LYD_OPT_RPCREPLY) {
            data = lyd_parse_xml(ctx, &xml->child, opts, rpc_act, val_tree);
        } else if (opts & (LYD_OPT_RPC | LYD_OPT_NOTIF)) {
            data = lyd_parse_xml(ctx, &xml->child, opts, val_tree);
        } else if (opts & LYD_OPT_DATA_TEMPLATE) {
            data = lyd_parse_xml(ctx, &xml->child, opts, rpc_act_file);
        } else {
            data = lyd_parse_xml(ctx, &xml->child, opts);
        }
        lyxml_free(ctx, xml);
    } else {
        if (opts & LYD_OPT_RPCREPLY) {
            if (!rpc_act_file) {
                fprintf(stderr, "RPC/action reply data require additional argument (file with the RPC/action).\n");
                return EXIT_FAILURE;
            }
            rpc_act = lyd_parse_path(ctx, rpc_act_file, informat, LYD_OPT_RPC, trees);
            if (!rpc_act) {
                fprintf(stderr, "Failed to parse RPC/action.\n");
                return EXIT_FAILURE;
            }
            if (trees) {
                const struct lyd_node **trees_new;
                unsigned int u;
                trees_new = lyd_trees_new(1, rpc_act);

                LY_ARRAY_FOR(trees, u) {
                    trees_new = lyd_trees_add(trees_new, trees[u]);
                }
                lyd_trees_free(trees, 0);
                trees = trees_new;
            } else {
                trees = lyd_trees_new(1, rpc_act);
            }
            data = lyd_parse_path(ctx, filepath, informat, opts, trees);
        } else if (opts & (LYD_OPT_RPC | LYD_OPT_NOTIF)) {
            data = lyd_parse_path(ctx, filepath, informat, opts, trees);
        } else if (opts & LYD_OPT_DATA_TEMPLATE) {
            if (!rpc_act_file) {
                fprintf(stderr, "YANG-DATA require additional argument (name instance of yang-data extension).\n");
                return EXIT_FAILURE;
            }
            data = lyd_parse_path(ctx, filepath, informat, opts, rpc_act_file);
        } else {
#endif

    lyd_parse_data(ctx, in, 0, opts, LYD_VALIDATE_PRESENT, &data);
#if 0
}

}
#endif
    ly_in_free(in, 0);

    lyd_free_all(rpc_act);

    if (ly_err_first(ctx)) {
        fprintf(stderr, "Failed to parse data.\n");
        lyd_free_all(data);
        return EXIT_FAILURE;
    }

    *result = data;
    *options = opts;
    return EXIT_SUCCESS;
}

int
cmd_data(struct ly_ctx *ctx, const char *arg)
{
    int c, argc, option_index, ret = 1;
    int options = 0, printopt = 0;
    char **argv = NULL, *ptr;
    const char *out_path = NULL;
    struct lyd_node *data = NULL;
    struct lyd_node *tree = NULL;
    LYD_FORMAT outformat = 0;
    struct ly_out *out = NULL;
    static struct option long_options[] = {
        {"defaults", required_argument, 0, 'd'},
        {"help", no_argument, 0, 'h'},
        {"format", required_argument, 0, 'f'},
        {"option", required_argument, 0, 't'},
        {"output", required_argument, 0, 'o'},
        {"running", required_argument, 0, 'r'},
        {"strict", no_argument, 0, 's'},
        {NULL, 0, 0, 0}
    };
    void *rlcd;

    argc = 1;
    argv = malloc(2 * sizeof *argv);
    *argv = strdup(arg);
    ptr = strtok(*argv, " ");
    while ((ptr = strtok(NULL, " "))) {
        rlcd = realloc(argv, (argc + 2) * sizeof *argv);
        if (!rlcd) {
            fprintf(stderr, "Memory allocation failed (%s:%d, %s)", __FILE__, __LINE__, strerror(errno));
            goto cleanup;
        }
        argv = rlcd;
        argv[argc++] = ptr;
    }
    argv[argc] = NULL;

    optind = 0;
    while (1) {
        option_index = 0;
        c = getopt_long(argc, argv, "d:hf:o:st:r:", long_options, &option_index);
        if (c == -1) {
            break;
        }

        switch (c) {
        case 'd':
            if (!strcmp(optarg, "all")) {
                printopt = (printopt & ~LYD_PRINT_WD_MASK) | LYD_PRINT_WD_ALL;
            } else if (!strcmp(optarg, "all-tagged")) {
                printopt = (printopt & ~LYD_PRINT_WD_MASK) | LYD_PRINT_WD_ALL_TAG;
            } else if (!strcmp(optarg, "trim")) {
                printopt = (printopt & ~LYD_PRINT_WD_MASK) | LYD_PRINT_WD_TRIM;
            } else if (!strcmp(optarg, "implicit-tagged")) {
                printopt = (printopt & ~LYD_PRINT_WD_MASK) | LYD_PRINT_WD_IMPL_TAG;
            }
            break;
        case 'h':
            cmd_data_help();
            ret = 0;
            goto cleanup;
        case 'f':
            if (!strcmp(optarg, "xml")) {
                outformat = LYD_XML;
            } else if (!strcmp(optarg, "json")) {
                outformat = LYD_JSON;
            } else if (!strcmp(optarg, "lyb")) {
                outformat = LYD_LYB;
            } else {
                fprintf(stderr, "Unknown output format \"%s\".\n", optarg);
                goto cleanup;
            }
            break;
        case 'o':
            if (out_path) {
                fprintf(stderr, "Output specified twice.\n");
                goto cleanup;
            }
            out_path = optarg;
            break;
#if 0
        case 'r':
            if (optarg[0] == '!') {
                /* ignore extenral dependencies to the running datastore */
                options |= LYD_OPT_NOEXTDEPS;
            } else {
                /* external file with the running datastore */
                val_tree = lyd_parse_path(ctx, optarg, LYD_XML, LYD_OPT_DATA_NO_YANGLIB, trees);
                if (!val_tree) {
                    fprintf(stderr, "Failed to parse the additional data tree for validation.\n");
                    goto cleanup;
                }
                if (!trees) {
                    trees = lyd_trees_new(1, val_tree);
                } else {
                    trees = lyd_trees_add(trees, val_tree);
                }
            }
            break;
#endif
        case 's':
            options |= LYD_PARSE_STRICT;
            break;
        case 't':
            if (!strcmp(optarg, "auto")) {
                /* no flags */
            } else if (!strcmp(optarg, "data")) {
                /* no flags */
                /*} else if (!strcmp(optarg, "config")) {
                    options |= LYD_OPT_CONFIG;
                } else if (!strcmp(optarg, "get")) {
                    options |= LYD_OPT_GET;
                } else if (!strcmp(optarg, "getconfig")) {
                    options |= LYD_OPT_GETCONFIG;
                } else if (!strcmp(optarg, "edit")) {
                    options |= LYD_OPT_EDIT;*/
            } else {
                fprintf(stderr, "Invalid parser option \"%s\".\n", optarg);
                cmd_data_help();
                goto cleanup;
            }
            break;
        case '?':
            fprintf(stderr, "Unknown option \"%d\".\n", (char)c);
            goto cleanup;
        }
    }

    /* file name */
    if (optind == argc) {
        fprintf(stderr, "Missing the data file name.\n");
        goto cleanup;
    }

    if (parse_data(ctx, argv[optind], &options, tree, argv[optind + 1], &data)) {
        goto cleanup;
    }

    if (out_path) {
        ret = ly_out_new_filepath(out_path, &out);
    } else {
        ret = ly_out_new_file(stdout, &out);
    }
    if (ret) {
        fprintf(stderr, "Could not open the output file (%s).\n", strerror(errno));
        goto cleanup;
    }

    if (outformat) {
        ret = lyd_print_all(out, data, outformat, printopt);
        ret = ret < 0 ? ret * (-1) : 0;
    }

cleanup:
    free(*argv);
    free(argv);

    ly_out_free(out, NULL, out_path ? 1 : 0);

    lyd_free_all(data);

    return ret;
}
