/**
 * @file main_ni.c
 * @author Radek Krejci <rkrejci@cesnet.cz>
 * @brief libyang's yanglint tool - noninteractive code
 *
 * Copyright (c) 2020 CESNET, z.s.p.o.
 *
 * This source code is licensed under BSD 3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://opensource.org/licenses/BSD-3-Clause
 */

#define _GNU_SOURCE

#include <errno.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#include "libyang.h"

#include "cmd.h"
#include "common.h"
#include "tools/config.h"

struct cmdline_file {
    struct ly_in *in;
    const char *path;
    LYD_FORMAT format;
};

/**
 * @brief Context structure to hold and pass variables in a structured form.
 */
struct context {
    /* libyang context for the run */
    struct ly_ctx *ctx;

    /* prepared output (--output option or stdout by default) */
    struct ly_out *out;

    struct ly_set searchpaths;

    /* options flags */
    uint8_t list;        /* -l option to print list of schemas */

    /*
     * schema
     */
    /* set schema modules' features via --feature option */
    struct schema_features *schemas_features;
    uint32_t schemas_features_count;

    /* set of loaded schema modules (struct lys_module *) */
    struct ly_set schema_modules;

    /* options to parse and print schema modules */
    uint32_t schema_parse_options;
    uint32_t schema_print_options;

    /* specification of printing schema node subtree, option --schema-node */
    const char *schema_node_path;
    const struct lysc_node *schema_node;

    /* value of --format in case of schema format */
    LYS_OUTFORMAT schema_out_format;

    /*
     * data
     */
    /* various options based on --type option */
    uint8_t data_type; /* values taken from LYD_VALIDATE_OP and extended by 0 for standard data tree */
    uint32_t data_parse_options;
    uint32_t data_validate_options;
    uint32_t data_print_options;

    /* flag for --merge option */
    uint8_t data_merge;

    /* value of --format in case of data format */
    LYD_FORMAT data_out_format;

    /* input files with the data suffixes */
    struct data_input {
        struct cmdline_file file;

        /*
         * In case the data_type is PARSE_REPLY, the parsing function requires information about the request for this reply.
         * One way to provide the request is a data file containing the full RPC/Action request which will be parsed.
         * Alternatively, it can be set as the Path of the requested RPC/Action and in that case it is stored in
         * data_request_paths.
         */
        struct cmdline_file request;
    } *data_inputs;
    uint32_t data_inputs_count;

    /*
     * an alternative way of providing requests for parsing data replies, instead of providing full
     * request in a data file, only the Path of the requested RPC/Action is provided and stored as
     * const char *. Note that the number of items in the set must be 1 (1 applies to all) or equal to
     * data_inputs_count (1 to 1 mapping).
     */
    struct ly_set data_request_paths;

    /* storage for --operational */
    struct cmdline_file data_operational;
};

static void
erase_context(struct context *c)
{
    /* data */
    for (uint32_t u = 0; u < c->data_inputs_count; ++u) {
        ly_in_free(c->data_inputs[u].file.in, 1);
        ly_in_free(c->data_inputs[u].request.in, 1);
    }
    free(c->data_inputs);
    ly_set_erase(&c->data_request_paths, NULL);
    ly_in_free(c->data_operational.in, 1);

    /* schema */
    free_features(c->schemas_features, c->schemas_features_count);
    ly_set_erase(&c->schema_modules, NULL);

    /* context */
    ly_set_erase(&c->searchpaths, NULL);

    ly_out_free(c->out, NULL,  0);
    ly_ctx_destroy(c->ctx, NULL);
}

static void
version(void)
{
    fprintf(stdout, "yanglint %s\n", PROJECT_VERSION);
}

static void
help(int shortout)
{

    fprintf(stdout, "Usage:\n");
    fprintf(stdout, "    yanglint [options] [-f { yang | yin | info}] <schema>...\n");
    fprintf(stdout, "        Validates the YANG module in <schema>, and all its dependencies.\n\n");
    fprintf(stdout, "    yanglint [options] [-f { xml | json }] <schema>... <file>...\n");
    fprintf(stdout, "        Validates the YANG modeled data in <file> according to the <schema>.\n\n");
    fprintf(stdout, "    yanglint\n");
    fprintf(stdout, "        Starts interactive mode with more features.\n\n");

    if (shortout) {
        return;
    }
    fprintf(stdout, "Options:\n"
            "  -h, --help      Show this help message and exit.\n"
            "  -v, --version   Show version number and exit.\n"
            "  -V, --verbose   Show verbose messages, can be used multiple times to\n"
            "                  increase verbosity.\n"
#ifndef NDEBUG
            "  -G GROUPS, --debug=GROUPS\n"
            "                  Enable printing of specific debugging message group\n"
            "                  (nothing will be printed unless verbosity is set to debug):\n"
            "                  <group>[,<group>]* (dict, yang, yin, xpath, diff)\n\n"
#endif

            "  -d MODE, --default=MODE\n"
            "                  Print data with default values, according to the MODE\n"
            "                  (to print attributes, ietf-netconf-with-defaults model\n"
            "                  must be loaded):\n"
            "      all             - Add missing default nodes.\n"
            "      all-tagged      - Add missing default nodes and mark all the default\n"
            "                        nodes with the attribute.\n"
            "      trim            - Remove all nodes with a default value.\n"
            "      implicit-tagged - Add missing nodes and mark them with the attribute.\n\n"

            "  -D, --disable-searchdir\n"
            "                  Do not implicitly search in current working directory for\n"
            "                  schema modules. If specified a second time, do not even\n"
            "                  search in the module directory (all modules must be \n"
            "                  explicitly specified).\n\n"

            "  -p PATH, --path=PATH\n"
            "                  Search path for schema (YANG/YIN) modules. The option can be\n"
            "                  used multiple times. The current working directory and the\n"
            "                  path of the module being added is used implicitly.\n\n"

            "  -F FEATURES, --features=FEATURES\n"
            "                  Features to support, default all.\n"
            "                  <modname>:[<feature>,]*\n\n"

            "  -i, --makeimplemented\n"
            "                  Make the imported modules \"referenced\" from any loaded\n"
            "                  module also implemented. If specified a second time, all the\n"
            "                  modules are set implemented.\n\n"

            "  -l, --list      Print info about the loaded schemas.\n"
            "                  (i - imported module, I - implemented module)\n"
            "                  In case the -f option with data encoding is specified,\n"
            "                  the list is printed as ietf-yang-library data.\n\n"

            "  -o OUTFILE, --output=OUTFILE\n"
            "                  Write the output to OUTFILE instead of stdout.\n\n"

            "  -f FORMAT, --format=FORMAT\n"
            "                  Convert input into FORMAT. Supported formats: \n"
            "                  yang, yin, tree and info for schemas,\n"
            "                  xml, json for data.\n\n"

            "  -P PATH, --schema-node=PATH\n"
            "                 Print only the specified subtree of the schema.\n"
            "                 The PATH is the XPath subset mentioned in documentation as\n"
            "                 the Path format. The option can be combined with --single-node\n"
            "                 option to print information only about the specified node.\n"
            "  -q, --single-node\n"
            "                 Supplement to the --schema-node option to print information\n"
            "                 only about a single node specified as PATH argument.\n\n"

            "  -s, --strict   Strict data parsing (do not skip unknown data), has no effect\n"
            "                 for schemas.\n\n"

            "  -e, --present  Validate only with the schema modules whose data actually\n"
            "                 exist in the provided input data files. Takes effect only\n"
            "                 with the 'data' or 'config' TYPEs. Used to avoid requiring\n"
            "                 mandatory nodes from modules which data are not present in the\n"
            "                 provided input data files.\n\n"

            "  -t TYPE, --type=TYPE\n"
            "                 Specify data tree type in the input data file(s):\n"
            "        data          - Complete datastore with status data (default type).\n"
            "        config        - Configuration datastore (without status data).\n"
            "        get           - Result of the NETCONF <get> operation.\n"
            "        getconfig     - Result of the NETCONF <get-config> operation.\n"
            "        edit          - Content of the NETCONF <edit-config> operation.\n"
            "        rpc           - Content of the NETCONF <rpc> message, defined as YANG's\n"
            "                        RPC/Action input statement.\n"
            "        reply         - Reply to the RPC/Action. Besides the reply itself,\n"
            "                        yanglint(1) requires information about the request for\n"
            "                        the reply. The request (RPC/Action) can be provide as\n"
            "                        the --request option or as another input data <file>\n"
            "                        provided right after the reply data <file> and\n"
            "                        containing complete RPC/Action for the reply.\n"
            "        notif         - Notification instance (content of the <notification>\n"
            "                        element without <eventTime>).\n"
            "  -r PATH, --request=PATH\n"
            "                 The alternative way of providing request information for the\n"
            "                 '--type=reply'. The PATH is the XPath subset described in\n"
            "                 documentation as Path format. It is required to point to the\n"
            "                 RPC or Action in the schema which is supposed to be a request\n"
            "                 for the reply(ies) being parsed from the input data files.\n"
            "                 In case of multiple input data files, the 'request' option can\n"
            "                 be set once for all the replies or multiple times each for the\n"
            "                 respective input data file.\n\n"

            "  -O FILE, --operational=FILE\n"
            "                Provide optional data to extend validation of the 'rpc',\n"
            "                'reply' or 'notif' TYPEs. The FILE is supposed to contain\n"
            "                the :running configuration datastore and state data\n"
            "                (operational datastore) referenced from the RPC/Notification.\n\n"

            "  -m, --merge    Merge input data files into a single tree and validate at\n"
            "                 once.The option has effect only for 'data' and 'config' TYPEs.\n\n"

#if 0
            "  -y YANGLIB_PATH       - Path to a yang-library data describing the initial context.\n\n"
            "Tree output specific options:\n"
            "  --tree-help           - Print help on tree symbols and exit.\n"
            "  --tree-print-groupings\n"
            "                        Print top-level groupings in a separate section.\n"
            "  --tree-print-uses     - Print uses nodes instead the resolved grouping nodes.\n"
            "  --tree-no-leafref-target\n"
            "                        Do not print target nodes of leafrefs.\n"
            "  --tree-path=SCHEMA_PATH\n"
            "                        Print only the specified subtree.\n"
            "  --tree-line-length=LINE_LENGTH\n"
            "                        Wrap lines if longer than the specified length (it is not a strict limit, longer lines\n"
            "                        can often appear).\n\n"
#endif
            "\n");
}

static void
libyang_verbclb(LY_LOG_LEVEL level, const char *msg, const char *path)
{
    char *levstr;

    if (level <= ly_log_level(LY_LLCUR)) {
        switch (level) {
        case LY_LLERR:
            levstr = "err :";
            break;
        case LY_LLWRN:
            levstr = "warn:";
            break;
        case LY_LLVRB:
            levstr = "verb:";
            break;
        default:
            levstr = "dbg :";
            break;
        }
        if (path) {
            fprintf(stderr, "%s %s (%s)\n", levstr, msg, path);
        } else {
            fprintf(stderr, "%s %s\n", levstr, msg);
        }
    }
}

static int
get_input(const char *filepath, LYS_INFORMAT *format_schema, LYD_FORMAT *format_data, struct ly_in **in)
{
    struct stat st;

    /* check that the filepath exists and is a regular file */
    if (stat(filepath, &st) == -1) {
        fprintf(stderr, "yanglint error: Unable to use input filepath (%s) - %s.\n", filepath, strerror(errno));
        return -1;
    }
    if (!S_ISREG(st.st_mode)) {
        fprintf(stderr, "yanglint error: Provided input file (%s) is not a regular file.\n", filepath);
        return -1;
    }

    /* get the file format */
    if (get_format(filepath, format_schema, format_data)) {
        return -1;
    }

    if (ly_in_new_filepath(filepath, 0, in)) {
        fprintf(stderr, "yanglint error: Unable to process input file.\n");
        return -1;
    }

    return 0;
}

static int
fill_context_inputs(int argc, char *argv[], struct context *c)
{
    struct ly_in *in;
    uint8_t request_expected = 0;

    /* process the operational content if any */
    if (c->data_operational.path) {
        if (get_input(c->data_operational.path, NULL, &c->data_operational.format, &c->data_operational.in)) {
            return -1;
        }
    }

    for (int i = 0; i < argc - optind; i++) {
        LYS_INFORMAT format_schema = LYS_IN_UNKNOWN;
        LYD_FORMAT format_data = LYD_UNKNOWN;

        if (get_input(argv[optind + i], &format_schema, &format_data, &in)) {
            return -1;
        }

        if (format_schema) {
            LY_ERR ret;
            uint8_t path_unset = 1; /* flag to unset the path from the searchpaths list (if not already present) */
            char *dir, *module;
            const char *fall = "*";
            const char **features = &fall;
            const struct lys_module *mod;

            if (parse_schema_path(argv[optind + i], &dir, &module)) {
                return -1;
            }

            /* add temporarily also the path of the module itself */
            if (ly_ctx_set_searchdir(c->ctx, dir) == LY_EEXIST) {
                path_unset = 0;
            }

            /* get features list for this module */
            for (uint32_t u = 0; u < c->schemas_features_count; ++u) {
                if (!strcmp(module, c->schemas_features[u].module)) {
                    features = (const char **)c->schemas_features[u].features;
                    break;
                }
            }

            /* temporary cleanup */
            free(dir);
            free(module);

            ret = lys_parse(c->ctx, in, format_schema, features, &mod);
            ly_ctx_unset_searchdir_last(c->ctx, path_unset);
            ly_in_free(in, 1);
            if (ret) {
                fprintf(stderr, "yanglint error: Processing schema module from %s failed.\n", argv[optind + i]);
                return -1;
            }

            if (c->schema_out_format) {
                /* modules will be printed */
                if (ly_set_add(&c->schema_modules, (void *)mod, 1, NULL)) {
                    fprintf(stderr, "yanglint error: Storing parsed schema module (%s) for print failed.\n",
                            argv[optind + i]);
                    return -1;
                }
            }
        } else if (request_expected) {
            c->data_inputs[c->data_inputs_count - 1].request.in = in;
            c->data_inputs[c->data_inputs_count - 1].request.path = argv[optind + i];
            c->data_inputs[c->data_inputs_count - 1].request.format = format_data;

            request_expected = 0;
        } else if (format_data) {
            struct data_input *rec;

            rec = realloc(c->data_inputs, (c->data_inputs_count + 1) * sizeof *c->data_inputs);
            if (!rec) {
                fprintf(stderr, "yanglint error: Unable to store input file information (%s).\n", strerror(errno));
                ly_in_free(in, 1);
                return -1;
            }
            c->data_inputs_count++;
            c->data_inputs = rec;
            rec = &c->data_inputs[c->data_inputs_count - 1];

            /* fill the record */
            memset(rec, 0, sizeof *rec);
            rec->file.path = argv[optind + i];
            rec->file.format = format_data;
            rec->file.in = in;
            in = NULL;

            if ((c->data_type == LYD_VALIDATE_OP_REPLY) && !c->data_request_paths.count) {
                /* requests for the replies are expected in another input file */
                if (++i == argc - optind) {
                    /* there is no such file */
                    fprintf(stderr, "yanglint error: Missing request input file for the reply input file %s.\n",
                            rec->file.path);
                    return -1;
                }

                request_expected = 1;
            }
        }
    }

    if (request_expected) {
        fprintf(stderr, "yanglint error: Missing request input file for the reply input file %s.\n",
                c->data_inputs[c->data_inputs_count - 1].file.path);
        return -1;
    }

    return 0;
}

/**
 * @brief Process command line options and store the settings into the context.
 *
 * return -1 in case of error;
 * return 0 in case of success and ready to process
 * return 1 in case of success, but expect to exit.
 */
static int
fill_context(int argc, char *argv[], struct context *c)
{
    int ret;

    int opt, opt_index;
    struct option options[] = {
        {"default",          required_argument, NULL, 'd'},
        {"disable-searchdir", no_argument,      NULL, 'D'},
        {"present",          no_argument,       NULL, 'e'},
        {"format",           required_argument, NULL, 'f'},
        {"features",         required_argument, NULL, 'F'},
#ifndef NDEBUG
        {"debug",            required_argument, NULL, 'G'},
#endif
        {"help",             no_argument,       NULL, 'h'},
        {"makeimplemented",  no_argument,       NULL, 'i'},
        {"list",             no_argument,       NULL, 'l'},
        {"merge",            no_argument,       NULL, 'm'},
        {"output",           required_argument, NULL, 'o'},
        {"operational",      required_argument, NULL, 'O'},
        {"path",             required_argument, NULL, 'p'},
        {"schema-node",      required_argument, NULL, 'P'},
        {"single-node",      no_argument,       NULL, 'q'},
        {"request",          required_argument, NULL, 'r'},
        {"strict",           no_argument,       NULL, 's'},
        {"type",             required_argument, NULL, 't'},
        {"version",          no_argument,       NULL, 'v'},
        {"verbose",          no_argument,       NULL, 'V'},
        {NULL,               0,                 NULL, 0}
    };

    uint16_t options_ctx = 0;
    uint8_t data_type_set = 0;

#ifndef NDEBUG
    while ((opt = getopt_long(argc, argv, "d:Df:F:hilmo:P:qr:st:vV", options, &opt_index)) != -1) {
#else
    while ((opt = getopt_long(argc, argv, "d:Df:F:G:hilmo:P:qr:st:vV", options, &opt_index)) != -1) {
#endif
        switch (opt) {
        case 'd': /* --default */
            if (!strcasecmp(optarg, "all")) {
                c->data_print_options = (c->data_print_options & ~LYD_PRINT_WD_MASK) | LYD_PRINT_WD_ALL;
            } else if (!strcasecmp(optarg, "all-tagged")) {
                c->data_print_options = (c->data_print_options & ~LYD_PRINT_WD_MASK) | LYD_PRINT_WD_ALL_TAG;
            } else if (!strcasecmp(optarg, "trim")) {
                c->data_print_options = (c->data_print_options & ~LYD_PRINT_WD_MASK) | LYD_PRINT_WD_TRIM;
            } else if (!strcasecmp(optarg, "implicit-tagged")) {
                c->data_print_options = (c->data_print_options & ~LYD_PRINT_WD_MASK) | LYD_PRINT_WD_IMPL_TAG;
            } else {
                fprintf(stderr, "yanglint error: unknown default mode %s\n", optarg);
                help(1);
                return -1;
            }
            break;

        case 'D': /* --disable-search */
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

        case 'p': { /* --path */
            struct stat st;

            if (stat(optarg, &st) == -1) {
                fprintf(stderr, "yanglint error: Unable to use search path (%s) - %s.\n", optarg, strerror(errno));
                return -1;
            }
            if (!S_ISDIR(st.st_mode)) {
                fprintf(stderr, "yanglint error: Provided search path is not a directory.\n");
                return -1;
            }

            if (ly_set_add(&c->searchpaths, optarg, 0, NULL)) {
                fprintf(stderr, "yanglint error: Storing searchpath failed.\n");
                return -1;
            }

            break;
        } /* case 'p' */

        case 'i': /* --makeimplemented */
            if (options_ctx & LY_CTX_REF_IMPLEMENTED) {
                options_ctx &= ~LY_CTX_REF_IMPLEMENTED;
                options_ctx |= LY_CTX_ALL_IMPLEMENTED;
            } else {
                options_ctx |= LY_CTX_REF_IMPLEMENTED;
            }
            break;

        case 'F': /* --features */
            if (parse_features(optarg, &c->schemas_features, &c->schemas_features_count)) {
                return -1;
            }
            break;

        case 'l': /* --list */
            c->list = 1;
            break;

        case 'o': /* --output */
            if (c->out) {
                if (ly_out_filepath(c->out, optarg) != NULL) {
                    fprintf(stderr, "yanglint error: unable open output file %s (%s)\n", optarg, strerror(errno));
                    return -1;
                }
            } else {
                if (ly_out_new_filepath(optarg, &c->out)) {
                    fprintf(stderr, "yanglint error: unable open output file %s (%s)\n", optarg, strerror(errno));
                    return -1;
                }
            }
            break;

        case 'f': /* --format */
            if (!strcasecmp(optarg, "yang")) {
                c->schema_out_format = LYS_OUT_YANG;
                c->data_out_format = 0;
            } else if (!strcasecmp(optarg, "yin")) {
                c->schema_out_format = LYS_OUT_YIN;
                c->data_out_format = 0;
            } else if (!strcasecmp(optarg, "info")) {
                c->schema_out_format = LYS_OUT_YANG_COMPILED;
                c->data_out_format = 0;
            } else if (!strcasecmp(optarg, "tree")) {
                c->schema_out_format = LYS_OUT_TREE;
                c->data_out_format = 0;
            } else if (!strcasecmp(optarg, "xml")) {
                c->schema_out_format = 0;
                c->data_out_format = LYD_XML;
            } else if (!strcasecmp(optarg, "json")) {
                c->schema_out_format = 0;
                c->data_out_format = LYD_JSON;
            } else {
                fprintf(stderr, "yanglint error: unknown output format %s\n", optarg);
                help(1);
                return -1;
            }
            break;

        case 'P': /* --schema-node */
            c->schema_node_path = optarg;
            break;

        case 'q': /* --single-node */
            c->schema_print_options |= LYS_PRINT_NO_SUBSTMT;
            break;

        case 's': /* --strict */
            c->data_parse_options |= LYD_PARSE_STRICT;
            break;

        case 'e': /* --present */
            c->data_validate_options |= LYD_VALIDATE_PRESENT;
            break;

        case 't': /* --type */
            if (data_type_set) {
                fprintf(stderr, "yanglint error: The data type (-t) cannot be set multiple times.\n");
                return -1;
            }

            if (!strcasecmp(optarg, "config")) {
                c->data_parse_options |= LYD_PARSE_NO_STATE;
            } else if (!strcasecmp(optarg, "get")) {
                c->data_parse_options |= LYD_PARSE_ONLY;
            } else if (!strcasecmp(optarg, "getconfig") || !strcasecmp(optarg, "get-config")) {
                c->data_parse_options |= LYD_PARSE_ONLY | LYD_PARSE_NO_STATE;
            } else if (!strcasecmp(optarg, "edit")) {
                c->data_parse_options |= LYD_PARSE_ONLY;
            } else if (!strcasecmp(optarg, "rpc") || !strcasecmp(optarg, "action")) {
                c->data_type = LYD_VALIDATE_OP_RPC;
            } else if (!strcasecmp(optarg, "reply") || !strcasecmp(optarg, "rpcreply")) {
                c->data_type = LYD_VALIDATE_OP_REPLY;
            } else if (!strcasecmp(optarg, "notif") || !strcasecmp(optarg, "notification")) {
                c->data_type = LYD_VALIDATE_OP_NOTIF;
            } else if (!strcasecmp(optarg, "data")) {
                /* default option */
            } else {
                fprintf(stderr, "yanglint error: unknown data tree type %s\n", optarg);
                help(1);
                return -1;
            }

            data_type_set = 1;
            break;

        case 'O': /* --operational */
            if (c->data_operational.path) {
                fprintf(stderr, "yanglint error: The operational datastore (-O) cannot be set multiple times.\n");
                return -1;
            }
            c->data_operational.path = optarg;
            break;

        case 'r': /* --request */
            if (ly_set_add(&c->data_request_paths, optarg, 0, NULL)) {
                fprintf(stderr, "yanglint error: Storing request path failed.\n");
                return -1;
            }
            break;

        case 'm': /* --merge */
            c->data_merge = 1;
            break;

        case 'h': /* --help */
            help(0);
            return 1;

        case 'v': /* --version */
            version();
            return 1;

        case 'V': { /* --verbose */
            LY_LOG_LEVEL verbosity = ly_log_level(LY_LLCUR);

            if (verbosity < LY_LLDBG) {
                ly_log_level(verbosity + 1);
            }
            break;
        } /* case 'V' */

#ifndef NDEBUG
        case 'G': { /* --debug */
            uint32_t dbg_groups = 0;
            const char *ptr = optarg;

            while (ptr[0]) {
                if (!strncasecmp(ptr, "dict", 4)) {
                    dbg_groups |= LY_LDGDICT;
                    ptr += 4;
                } else if (!strncasecmp(ptr, "yang", 4)) {
                    dbg_groups |= LY_LDGYANG;
                    ptr += 4;
                } else if (!strncasecmp(ptr, "yin", 3)) {
                    dbg_groups |= LY_LDGYIN;
                    ptr += 3;
                } else if (!strncasecmp(ptr, "xpath", 5)) {
                    dbg_groups |= LY_LDGXPATH;
                    ptr += 5;
                } else if (!strncasecmp(ptr, "diff", 4)) {
                    dbg_groups |= LY_LDGDIFF;
                    ptr += 4;
                }

                if (ptr[0]) {
                    if (ptr[0] != ',') {
                        fprintf(stderr, "yanglint error: unknown debug group string \"%s\"\n", optarg);
                        return -1;
                    }
                    ++ptr;
                }
            }
            ly_log_dbg_groups(dbg_groups);
            break;
        } /* case 'G' */
#endif
        } /* switch */
    }

    /* libyang context */
    if (ly_ctx_new(NULL, options_ctx, &c->ctx)) {
        fprintf(stderr, "yanglint error: unable to create libyang context.\n");
        return -1;
    }
    for (uint32_t u = 0; u < c->searchpaths.count; ++u) {
        ly_ctx_set_searchdir(c->ctx, c->searchpaths.objs[u]);
    }

    /* additional checks for the options combinations */
    if (!c->list && (optind >= argc)) {
        help(1);
        fprintf(stderr, "yanglint error: missing <schema> to process.\n");
        return 1;
    }

    if (c->data_merge) {
        if (c->data_type || (c->data_parse_options & LYD_PARSE_ONLY)) {
            /* switch off the option, incompatible input data type */
            c->data_merge = 0;
        } else {
            /* postpone validation after the merge of all the input data */
            c->data_parse_options |= LYD_PARSE_ONLY;
        }
    }

    if (c->data_operational.path && !c->data_type) {
        fprintf(stderr, "yanglint error: operational datastore takes effect only with RPCs/Actions/Replies/Notifications input data types.\n");
    }

    /* default output stream */
    if (!c->out) {
        if (ly_out_new_file(stdout, &c->out)) {
            fprintf(stderr, "yanglint error: unable to set stdout as output.\n");
            return -1;
        }
    }

    /* process input files provided as standalone command line arguments,
     * schema modules are parsed and inserted into the context,
     * data files are just checked and prepared into internal structures for further processing */
    ret = fill_context_inputs(argc, argv, c);
    if (ret) {
        return ret;
    }

    /* the second batch of checks */
    if (c->schema_print_options && !c->schema_out_format) {
        fprintf(stderr, "yanglint warning: schema printer options specified, but the schema output format is missing.\n");
    }
    if (c->schema_parse_options && !c->schema_modules.count) {
        fprintf(stderr, "yanglint warning: schema parser options specified, but no schema input file provided.\n");
    }
    if (c->data_print_options && !c->data_out_format) {
        fprintf(stderr, "yanglint warning: data printer options specified, but the data output format is missing.\n");
    }
    if ((c->data_parse_options || c->data_type) && !c->data_inputs) {
        fprintf(stderr, "yanglint warning: data parser options specified, but no data input file provided.\n");
    }

    if (c->schema_node_path) {
        c->schema_node = lys_find_path(c->ctx, NULL, c->schema_node_path, 0);
        if (!c->schema_node) {
            c->schema_node = lys_find_path(c->ctx, NULL, c->schema_node_path, 1);

            if (!c->schema_node) {
                fprintf(stderr, "yanglint error: invalid schema path.\n");
                return -1;
            }
        }
    }

    if (c->data_type == LYD_VALIDATE_OP_REPLY) {
        if ((c->data_request_paths.count > 1) && (c->data_request_paths.count != c->data_inputs_count)) {
            fprintf(stderr, "yanglint error: number of request paths does not match the number of reply data files (%u:%u).\n",
                    c->data_request_paths.count, c->data_inputs_count);
            return -1;
        }

        for (uint32_t u = 0; u < c->data_request_paths.count; ++u) {
            const char *path = (const char *)c->data_request_paths.objs[u];
            const struct lysc_node *action = NULL;

            action = lys_find_path(c->ctx, NULL, path, 0);
            if (!action) {
                fprintf(stderr, "yanglint error: the request path \"%s\" is not valid.\n", path);
                return -1;
            } else if (!(action->nodetype & (LYS_RPC | LYS_ACTION))) {
                fprintf(stderr, "yanglint error: the request path \"%s\" does not represent RPC/Action.\n", path);
                return -1;
            }
        }
    }

    return 0;
}

int
main_ni(int argc, char *argv[])
{
    int ret = EXIT_SUCCESS, r;
    struct context c = {0};
    struct lyd_node *tree = NULL, *merged_tree = NULL;
    struct lyd_node *operational;

    /* set callback for printing libyang messages */
    ly_set_log_clb(libyang_verbclb, 1);

    r = fill_context(argc, argv, &c);
    if (r < 0) {
        ret = EXIT_FAILURE;
    }
    if (r) {
        goto cleanup;
    }

    /* do the required job - parse, validate, print */

    if (c.list) {
        /* print the list of schemas */
        print_list(c.out, c.ctx, c.data_out_format);
    } else if (c.schema_out_format) {
        if (c.schema_node) {
            ret = lys_print_node(c.out, c.schema_node, c.schema_out_format, 0, c.schema_print_options);
            if (ret) {
                fprintf(stderr, "yanglint error: unable to print schema node %s.\n", c.schema_node_path);
                goto cleanup;
            }
        } else {
            for (uint32_t u = 0; u < c.schema_modules.count; ++u) {
                ret = lys_print_module(c.out, (struct lys_module *)c.schema_modules.objs[u], c.schema_out_format, 0,
                        c.schema_print_options);
                if (ret) {
                    fprintf(stderr, "yanglint error: unable to print module %s.\n",
                            ((struct lys_module *)c.schema_modules.objs[u])->name);
                    goto cleanup;
                }
            }
        }
    } else if (c.data_out_format) {
        /* additional operational datastore */
        if (c.data_operational.in) {
            ret = lyd_parse_data(c.ctx, c.data_operational.in, c.data_operational.format, LYD_PARSE_ONLY, 0, &operational);
            if (ret) {
                fprintf(stderr, "yanglint error: Failed to parse operational datastore file \"%s\".\n",
                        c.data_operational.path);
                goto cleanup;
            }
        }

        for (uint32_t u = 0; u < c.data_inputs_count; ++u) {
            switch (c.data_type) {
            case 0:
                ret = lyd_parse_data(c.ctx, c.data_inputs[u].file.in, c.data_inputs[u].file.format, c.data_parse_options,
                        c.data_validate_options, &tree);
                break;
            case LYD_VALIDATE_OP_RPC:
                ret = lyd_parse_rpc(c.ctx, c.data_inputs[u].file.in, c.data_inputs[u].file.format, &tree, NULL);
                break;
            case LYD_VALIDATE_OP_REPLY: {
                struct lyd_node *request = NULL;

                /* get the request data */
                if (c.data_request_paths.count) {
                    const char *path;
                    if (c.data_request_paths.count > 1) {
                        /* one to one */
                        path = (const char *)c.data_request_paths.objs[u];
                    } else {
                        /* one to all */
                        path = (const char *)c.data_request_paths.objs[0];
                    }
                    ret = lyd_new_path(NULL, c.ctx, path, NULL, 0, &request);
                    if (ret) {
                        fprintf(stderr, "yanglint error: Failed to create request data from path \"%s\".\n", path);
                        goto cleanup;
                    }
                } else {
                    ret = lyd_parse_rpc(c.ctx, c.data_inputs[u].request.in, c.data_inputs[u].request.format, &request, NULL);
                    if (ret) {
                        fprintf(stderr, "yanglint error: Failed to parse input data file \"%s\".\n",
                                c.data_inputs->file.path);
                        goto cleanup;
                    }
                }

                /* get the reply data */
                ret = lyd_parse_reply(request, c.data_inputs[u].file.in, c.data_inputs[u].file.format, &tree, NULL);
                lyd_free_all(request);

                break;
            } /* case PARSE_REPLY */
            case LYD_VALIDATE_OP_NOTIF:
                ret = lyd_parse_notif(c.ctx, c.data_inputs[u].file.in, c.data_inputs[u].file.format, &tree, NULL);
                break;
            }

            if (ret) {
                fprintf(stderr, "yanglint error: Failed to parse input data file \"%s\".\n", c.data_inputs->file.path);
                goto cleanup;
            }

            if (c.data_merge) {
                /* merge the data so far parsed for later validation and print */
                if (!merged_tree) {
                    merged_tree = tree;
                } else {
                    ret = lyd_merge_siblings(&merged_tree, tree, LYD_MERGE_DESTRUCT);
                    if (ret) {
                        fprintf(stderr, "yanglint error: Merging %s with previous data failed.\n",
                                c.data_inputs[u].file.path);
                        goto cleanup;
                    }
                }
                tree = NULL;
            } else if (c.data_out_format) {
                lyd_print_all(c.out, tree, c.data_out_format, c.data_print_options);
            } else if (operational) {
                /* additional validation of the RPC/Action/reply/Notification with the operational datastore */
                ret = lyd_validate_op(tree, operational, c.data_type, NULL);
                if (ret) {
                    fprintf(stderr,
                            "yanglint error: Failed to validate input data file \"%s\" with operational datastore \"%s\".\n",
                            c.data_inputs->file.path, c.data_operational.path);
                    goto cleanup;
                }
            }
            lyd_free_all(tree);
            tree = NULL;
        }

        if (c.data_out_format && c.data_merge) {
            /* validate the result */
            ret = lyd_validate_all(&merged_tree, c.ctx, LYD_VALIDATE_PRESENT, NULL);
            if (ret) {
                fprintf(stderr, "yanglint error: Merged data are not valid.\n");
                goto cleanup;
            }
            /* and print it */
            lyd_print_all(c.out, merged_tree, c.data_out_format, c.data_print_options);
        }
    }

cleanup:
    /* cleanup */
    lyd_free_all(merged_tree);
    lyd_free_all(tree);
    erase_context(&c);

    return ret;
}
