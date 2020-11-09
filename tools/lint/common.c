/**
 * @file common.c
 * @author Radek Krejci <rkrejci@cesnet.cz>
 * @brief libyang's yanglint tool - common functions for both interactive and non-interactive mode.
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

#include "common.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "libyang.h"

int
parse_schema_path(const char *path, char **dir, char **module)
{
    char *p;

    /* split the path to dirname and basename for further work */
    *dir = strdup(path);
    *module = strrchr(*dir, '/');
    if (!(*module)) {
        *module = *dir;
        *dir = strdup("./");
    } else {
        *module[0] = '\0'; /* break the dir */
        *module = strdup((*module) + 1);
    }
    /* get the pure module name without suffix or revision part of the filename */
    if ((p = strchr(*module, '@'))) {
        /* revision */
        *p = '\0';
    } else if ((p = strrchr(*module, '.'))) {
        /* fileformat suffix */
        *p = '\0';
    }

    return 0;
}

void
free_features(struct schema_features *flist, uint32_t fcounter)
{
    for (uint32_t u = 0; u < fcounter; ++u) {
        free(flist[u].module);
        if (flist[u].features) {
            for (uint32_t v = 0; flist[u].features[v]; ++v) {
                free(flist[u].features[v]);
            }
            free(flist[u].features);
        }
    }
    free(flist);
}

int
parse_features(const char *fstring, struct schema_features **flist_p, uint32_t *fcounter_p)
{
    struct schema_features *rec, *flist = *flist_p;
    char *p;

    rec = realloc(flist, ((*fcounter_p) + 1) * sizeof *flist);
    if (!rec) {
        fprintf(stderr, "yanglint error: Unable to store features information (%s).\n", strerror(errno));
        return -1;
    }
    (*fcounter_p)++;
    flist = *flist_p = rec;
    rec = &flist[(*fcounter_p) - 1];
    memset(rec, 0, sizeof *rec);

    /* fill the record */
    p = strchr(fstring, ':');
    if (!p) {
        fprintf(stderr, "yanglint error: Invalid format of the features specification (%s)", fstring);
        return -1;
    }
    rec->module = strndup(fstring, p - fstring);

    /* start count on 2 to include terminating NULL byte */
    for (int count = 2; p; ++count) {
        size_t len = 0;
        char *token = p + 1;
        p = strchr(token, ',');
        if (!p) {
            /* the last item, if any */
            len = strlen(token);
        } else {
            len = p - token;
        }
        if (len) {
            char **fp = realloc(rec->features, count * sizeof *rec->features);
            if (!fp) {
                fprintf(stderr, "Unable to store features list information (%s).\n",
                        strerror(errno));
                return -1;
            }
            rec->features = fp;
            rec->features[count - 1] = NULL; /* terminating NULL-byte */
            fp = &rec->features[count - 2]; /* array item to set */
            (*fp) = strndup(token, len);
        }
    }

    return 0;
}

void
free_cmdline(char *argv[])
{
    if (argv) {
        free(argv[0]);
        free(argv);
    }
}

int
parse_cmdline(const char *cmdline, int *argc_p, char **argv_p[])
{
    int count;
    char **vector;
    char *ptr, *end;
    char qmark = 0;

    assert(cmdline);
    assert(argc_p);
    assert(argv_p);

    /* init */
    count = 1;
    vector = malloc((count + 1) * sizeof *vector);
    vector[0] = strdup(cmdline);
    end = &vector[0][strlen(vector[0])];

    /* command name */
    strtok(vector[0], " ");

    /* arguments */
    while ((ptr = strtok(NULL, " "))) {
        size_t len;
        void *r;

        len = strlen(ptr);

        if (qmark) {
            /* still in quotated text */
            /* remove NULL termination of the previous token since it is not a token,
             * but a part of the quotation string */
            ptr[-1] = ' ';

            if ((ptr[len - 1] == qmark) && (ptr[len - 2] != '\\')) {
                /* end of quotation */
                qmark = 0;
                /* shorten the argument by the terminating quotation mark */
                ptr[len - 1] = '\0';
            }
            continue;
        }

        /* another token in cmdline */
        ++count;
        r = realloc(vector, (count + 1) * sizeof *vector);
        if (!r) {
            fprintf(stderr, "Memory allocation failed (%s:%d, %s),", __FILE__, __LINE__, strerror(errno));
            free(vector);
            return -1;
        }
        vector = r;
        vector[count - 1] = ptr;

        if ((ptr[0] == '"') || (ptr[0] == '\'')) {
            /* remember the quotation mark to identify end of quotation */
            qmark = ptr[0];

            /* move the remembered argument after the quotation mark */
            ++vector[count - 1];

            /* check if the quotation is terminated within this token */
            if ((ptr[len - 1] == qmark) && (ptr[len - 2] != '\\')) {
                /* end of quotation */
                qmark = 0;
                /* shorten the argument by the terminating quotation mark */
                ptr[len - 1] = '\0';
            }
        }
    }
    vector[count] = NULL;

    *argc_p = count;
    *argv_p = vector;

    return 0;
}

int
get_format(const char *filename, LYS_INFORMAT *schema, LYD_FORMAT *data)
{
    char *ptr;
    LYS_INFORMAT informat_s;
    LYD_FORMAT informat_d;

    /* get the file format */
    if ((ptr = strrchr(filename, '.')) != NULL) {
        ++ptr;
        if (!strcmp(ptr, "yang")) {
            informat_s = LYS_IN_YANG;
            informat_d = 0;
        } else if (!strcmp(ptr, "yin")) {
            informat_s = LYS_IN_YIN;
            informat_d = 0;
        } else if (!strcmp(ptr, "xml")) {
            informat_s = 0;
            informat_d = LYD_XML;
        } else if (!strcmp(ptr, "json")) {
            informat_s = 0;
            informat_d = LYD_JSON;
        } else {
            fprintf(stderr, "yanglint error: input file in an unknown format \"%s\".\n", ptr);
            return 0;
        }
    } else {
        fprintf(stderr, "yanglint error: input file \"%s\" without file extension - unknown format.\n", filename);
        return 1;
    }

    if (informat_d) {
        if (!data) {
            fprintf(stderr, "yanglint error: input file \"%s\" not expected to contain data instances (unexpected format).\n",
                    filename);
            return 2;
        }
        (*data) = informat_d;
    } else if (informat_s) {
        if (!schema) {
            fprintf(stderr, "yanglint error: input file \"%s\" not expected to contain schema definition (unexpected format).\n",
                    filename);
            return 3;
        }
        (*schema) = informat_s;
    }

    return 0;
}

int
print_list(struct ly_out *out, struct ly_ctx *ctx, LYD_FORMAT outformat)
{
    struct lyd_node *ylib;
    uint32_t idx = 0, has_modules = 0;
    const struct lys_module *mod;

    if (outformat != LYD_UNKNOWN) {
        if (ly_ctx_get_yanglib_data(ctx, &ylib)) {
            fprintf(stderr, "Getting context info (ietf-yang-library data) failed.\n");
            return 1;
        }

        lyd_print_all(out, ylib, outformat, 0);
        lyd_free_all(ylib);
        return 0;
    }

    /* iterate schemas in context and provide just the basic info */
    ly_print(out, "List of the loaded models:\n");
    while ((mod = ly_ctx_get_module_iter(ctx, &idx))) {
        has_modules++;

        /* conformance print */
        if (mod->implemented) {
            ly_print(out, "    I");
        } else {
            ly_print(out, "    i");
        }

        /* module print */
        ly_print(out, " %s", mod->name);
        if (mod->revision) {
            ly_print(out, "@%s", mod->revision);
        }

        /* submodules print */
        if (mod->parsed && mod->parsed->includes) {
            uint64_t u = 0;
            ly_print(out, " (");
            LY_ARRAY_FOR(mod->parsed->includes, u) {
                ly_print(out, "%s%s", !u ? "" : ",", mod->parsed->includes[u].name);
                if (mod->parsed->includes[u].rev[0]) {
                    ly_print(out, "@%s", mod->parsed->includes[u].rev);
                }
            }
            ly_print(out, ")");
        }

        /* finish the line */
        ly_print(out, "\n");
    }

    if (!has_modules) {
        ly_print(out, "\t(none)\n");
    }

    ly_print_flush(out);
    return 0;
}
