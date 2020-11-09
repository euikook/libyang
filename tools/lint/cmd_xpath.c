/**
 * @file cmd_xpath.c
 * @author Michal Vasko <mvasko@cesnet.cz>
 * @author Radek Krejci <rkrejci@cesnet.cz>
 * @brief 'xpath' command of the libyang's yanglint tool.
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
cmd_xpath_help(void)
{
    printf("xpath [-t TYPE] [-x <additional-tree-file-name>] -e <XPath-expression>\n"
            "      <XML-data-file-name> [<JSON-rpc/action-schema-nodeid>]\n");
    printf("Accepted TYPEs:\n");
    printf("\tauto       - resolve data type (one of the following) automatically (as pyang does),\n");
    printf("\t             this option is applicable only in case of XML input data.\n");
    printf("\tconfig     - LYD_OPT_CONFIG\n");
    printf("\tget        - LYD_OPT_GET\n");
    printf("\tgetconfig  - LYD_OPT_GETCONFIG\n");
    printf("\tedit       - LYD_OPT_EDIT\n");
    printf("\trpc        - LYD_OPT_RPC\n");
    printf("\trpcreply   - LYD_OPT_RPCREPLY (last parameter mandatory in this case)\n");
    printf("\tnotif      - LYD_OPT_NOTIF\n\n");
    printf("Option -x:\n");
    printf("\tIf RPC/action/notification/RPC reply (for TYPEs 'rpc', 'rpcreply', and 'notif') includes\n");
    printf("\tan XPath expression (when/must) that needs access to the configuration data, you can provide\n");
    printf("\tthem in a file, which will be parsed as 'config'.\n");
}

int
cmd_xpath(struct ly_ctx *ctx, const char *arg)
{
    int ret = 1;

#if 0
    int c, argc, option_index, ret = 1, long_str;
    char **argv = NULL, *ptr, *expr = NULL;
    unsigned int i, j;
    int options = 0;
    struct lyd_node *data = NULL, *node, *val_tree = NULL;
    struct lyd_node_leaf_list *key;
    struct ly_set *set;
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"expr", required_argument, 0, 'e'},
        {NULL, 0, 0, 0}
    };
    void *rlcd;

    long_str = 0;
    argc = 1;
    argv = malloc(2 * sizeof *argv);
    *argv = strdup(arg);
    ptr = strtok(*argv, " ");
    while ((ptr = strtok(NULL, " "))) {
        if (long_str) {
            ptr[-1] = ' ';
            if (ptr[strlen(ptr) - 1] == long_str) {
                long_str = 0;
                ptr[strlen(ptr) - 1] = '\0';
            }
        } else {
            rlcd = realloc(argv, (argc + 2) * sizeof *argv);
            if (!rlcd) {
                fprintf(stderr, "Memory allocation failed (%s:%d, %s)", __FILE__, __LINE__, strerror(errno));
                goto cleanup;
            }
            argv = rlcd;
            argv[argc] = ptr;
            if (ptr[0] == '"') {
                long_str = '"';
                ++argv[argc];
            }
            if (ptr[0] == '\'') {
                long_str = '\'';
                ++argv[argc];
            }
            if (ptr[strlen(ptr) - 1] == long_str) {
                long_str = 0;
                ptr[strlen(ptr) - 1] = '\0';
            }
            ++argc;
        }
    }
    argv[argc] = NULL;

    optind = 0;
    while (1) {
        option_index = 0;
        c = getopt_long(argc, argv, "he:t:x:", long_options, &option_index);
        if (c == -1) {
            break;
        }

        switch (c) {
        case 'h':
            cmd_xpath_help();
            ret = 0;
            goto cleanup;
        case 'e':
            expr = optarg;
            break;
        case 't':
            if (!strcmp(optarg, "auto")) {
                options = (options & ~LYD_OPT_TYPEMASK) | LYD_OPT_TYPEMASK;
            } else if (!strcmp(optarg, "config")) {
                options = (options & ~LYD_OPT_TYPEMASK) | LYD_OPT_CONFIG;
            } else if (!strcmp(optarg, "get")) {
                options = (options & ~LYD_OPT_TYPEMASK) | LYD_OPT_GET;
            } else if (!strcmp(optarg, "getconfig")) {
                options = (options & ~LYD_OPT_TYPEMASK) | LYD_OPT_GETCONFIG;
            } else if (!strcmp(optarg, "edit")) {
                options = (options & ~LYD_OPT_TYPEMASK) | LYD_OPT_EDIT;
            } else if (!strcmp(optarg, "rpc")) {
                options = (options & ~LYD_OPT_TYPEMASK) | LYD_OPT_RPC;
            } else if (!strcmp(optarg, "rpcreply")) {
                options = (options & ~LYD_OPT_TYPEMASK) | LYD_OPT_RPCREPLY;
            } else if (!strcmp(optarg, "notif")) {
                options = (options & ~LYD_OPT_TYPEMASK) | LYD_OPT_NOTIF;
            } else if (!strcmp(optarg, "yangdata")) {
                options = (options & ~LYD_OPT_TYPEMASK) | LYD_OPT_DATA_TEMPLATE;
            } else {
                fprintf(stderr, "Invalid parser option \"%s\".\n", optarg);
                cmd_data_help();
                goto cleanup;
            }
            break;
        case 'x':
            val_tree = lyd_parse_path(ctx, optarg, LYD_XML, LYD_OPT_CONFIG);
            if (!val_tree) {
                fprintf(stderr, "Failed to parse the additional data tree for validation.\n");
                goto cleanup;
            }
            break;
        case '?':
            fprintf(stderr, "Unknown option \"%d\".\n", (char)c);
            goto cleanup;
        }
    }

    if (optind == argc) {
        fprintf(stderr, "Missing the file with data.\n");
        goto cleanup;
    }

    if (!expr) {
        fprintf(stderr, "Missing the XPath expression.\n");
        goto cleanup;
    }

    if (parse_data(argv[optind], &options, val_tree, argv[optind + 1], &data)) {
        goto cleanup;
    }

    if (!(set = lyd_find_path(data, expr))) {
        goto cleanup;
    }

    /* print result */
    printf("Result:\n");
    if (!set->number) {
        printf("\tEmpty\n");
    } else {
        for (i = 0; i < set->number; ++i) {
            node = set->set.d[i];
            switch (node->schema->nodetype) {
            case LYS_CONTAINER:
                printf("\tContainer ");
                break;
            case LYS_LEAF:
                printf("\tLeaf ");
                break;
            case LYS_LEAFLIST:
                printf("\tLeaflist ");
                break;
            case LYS_LIST:
                printf("\tList ");
                break;
            case LYS_ANYXML:
                printf("\tAnyxml ");
                break;
            case LYS_ANYDATA:
                printf("\tAnydata ");
                break;
            default:
                printf("\tUnknown ");
                break;
            }
            printf("\"%s\"", node->schema->name);
            if (node->schema->nodetype & (LYS_LEAF | LYS_LEAFLIST)) {
                printf(" (val: %s)", ((struct lyd_node_leaf_list *)node)->value_str);
            } else if (node->schema->nodetype == LYS_LIST) {
                key = (struct lyd_node_leaf_list *)node->child;
                printf(" (");
                for (j = 0; j < ((struct lys_node_list *)node->schema)->keys_size; ++j) {
                    if (j) {
                        printf(" ");
                    }
                    printf("\"%s\": %s", key->schema->name, key->value_str);
                    key = (struct lyd_node_leaf_list *)key->next;
                }
                printf(")");
            }
            printf("\n");
        }
    }
    printf("\n");

    ly_set_free(set);
    ret = 0;

cleanup:
    free(*argv);
    free(argv);

    lyd_free_withsiblings(data);
#endif
    return ret;
}
