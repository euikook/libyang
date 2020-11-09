/**
 * @file main.c
 * @author Radek Krejci <rkrejci@cesnet.cz>
 * @brief libyang's yanglint tool
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libyang.h"

#include "cmd.h"
#include "common.h"
#include "compat.h"
#include "completion.h"
#include "configuration.h"
#include "linenoise/linenoise.h"
#include "tools/config.h"

int done;
struct ly_ctx *ctx = NULL;

/* main_ni.c */
int main_ni(int argc, char *argv[]);

int
main(int argc, char *argv[])
{
    char *cmdline;
    int cmdlen;

    if (argc > 1) {
        /* run in non-interactive mode */
        return main_ni(argc, argv);
    }

    /* continue in interactive mode */
    linenoiseSetCompletionCallback(complete_cmd);
    load_config();

    if (ly_ctx_new(NULL, 0, &ctx)) {
        fprintf(stderr, "Failed to create context.\n");
        return 1;
    }

    while (!done) {
        uint8_t executed = 0;

        /* get the command from user */
        cmdline = linenoise(PROMPT);

        /* EOF -> exit */
        if (cmdline == NULL) {
            done = 1;
            cmdline = strdup("quit");
        }

        /* empty line -> wait for another command */
        if (*cmdline == '\0') {
            free(cmdline);
            continue;
        }

        /* isolate the command word. */
        for (cmdlen = 0; cmdline[cmdlen] && (cmdline[cmdlen] != ' '); cmdlen++) {}

        /* execute the command if any valid specified */
        for (uint16_t i = 0; commands[i].name; i++) {
            if (strncmp(cmdline, commands[i].name, (size_t)cmdlen) || (commands[i].name[cmdlen] != '\0')) {
                continue;
            }

            commands[i].func(&ctx, cmdline);
            executed = 1;
            break;
        }

        if (!executed) {
            /* if unknown command specified, tell it to user */
            fprintf(stderr, "%.*s: no such command, type 'help' for more information.\n", cmdlen, cmdline);
        }

        linenoiseHistoryAdd(cmdline);
        free(cmdline);
    }

    store_config();
    ly_ctx_destroy(ctx, NULL);

    return 0;
}
