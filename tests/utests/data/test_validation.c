/**
 * @file test_validation.c
 * @author: Radek Krejci <rkrejci@cesnet.cz>
 * @brief unit tests for functions from validation.c
 *
 * Copyright (c) 2020 CESNET, z.s.p.o.
 *
 * This source code is licensed under BSD 3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://opensource.org/licenses/BSD-3-Clause
 */

#include "../macros.h"

#include <stdio.h>
#include <string.h>

#include "context.h"
#include "in.h"
#include "parser_data.h"
#include "out.h"
#include "printer_data.h"
#include "tests/config.h"
#include "tree_schema.h"
#include "tree_data_internal.h"

#define BUFSIZE 1024
char logbuf[BUFSIZE] = {0};
int store = -1; /* negative for infinite logging, positive for limited logging */

/* set to 0 to printing error messages to stderr instead of checking them in code */
#define ENABLE_LOGGER_CHECKING 1

#if ENABLE_LOGGER_CHECKING
static void
logger(LY_LOG_LEVEL level, const char *msg, const char *path)
{
    (void) level; /* unused */
    if (store) {
        if (path && path[0]) {
            snprintf(logbuf, BUFSIZE - 1, "%s %s", msg, path);
        } else {
            strncpy(logbuf, msg, BUFSIZE - 1);
        }
        if (store > 0) {
            --store;
        }
    }
}
#endif

const char *schema_a =
    "module a {"
        "namespace urn:tests:a;"
        "prefix a;"
        "yang-version 1.1;"

        "container cont {"
            "leaf a {"
                "when \"../../c = 'val_c'\";"
                "type string;"
            "}"
            "leaf b {"
                "type string;"
            "}"
        "}"
        "leaf c {"
            "when \"/cont/b = 'val_b'\";"
            "type string;"
        "}"
    "}";
const char *schema_b =
    "module b {"
        "namespace urn:tests:b;"
        "prefix b;"
        "yang-version 1.1;"

        "choice choic {"
            "mandatory true;"
            "leaf a {"
                "type string;"
            "}"
            "case b {"
                "leaf l {"
                    "type string;"
                "}"
            "}"
        "}"
        "leaf c {"
            "mandatory true;"
            "type string;"
        "}"
        "leaf d {"
            "type empty;"
        "}"
    "}";
const char *schema_c =
    "module c {"
        "namespace urn:tests:c;"
        "prefix c;"
        "yang-version 1.1;"

        "choice choic {"
            "leaf a {"
                "type string;"
            "}"
            "case b {"
                "leaf-list l {"
                    "min-elements 3;"
                    "type string;"
                "}"
            "}"
        "}"
        "list lt {"
            "max-elements 4;"
            "key \"k\";"
            "leaf k {"
                "type string;"
            "}"
        "}"
        "leaf d {"
            "type empty;"
        "}"
    "}";
const char *schema_d =
    "module d {"
        "namespace urn:tests:d;"
        "prefix d;"
        "yang-version 1.1;"

        "list lt {"
            "key \"k\";"
            "unique \"l1\";"
            "leaf k {"
                "type string;"
            "}"
            "leaf l1 {"
                "type string;"
            "}"
        "}"
        "list lt2 {"
            "key \"k\";"
            "unique \"cont/l2 l4\";"
            "unique \"l5 l6\";"
            "leaf k {"
                "type string;"
            "}"
            "container cont {"
                "leaf l2 {"
                    "type string;"
                "}"
            "}"
            "leaf l4 {"
                "type string;"
            "}"
            "leaf l5 {"
                "type string;"
            "}"
            "leaf l6 {"
                "type string;"
            "}"
            "list lt3 {"
                "key \"kk\";"
                "unique \"l3\";"
                "leaf kk {"
                    "type string;"
                "}"
                "leaf l3 {"
                    "type string;"
                "}"
            "}"
        "}"
    "}";
const char *schema_e =
    "module e {"
        "namespace urn:tests:e;"
        "prefix e;"
        "yang-version 1.1;"

        "choice choic {"
            "leaf a {"
                "type string;"
            "}"
            "case b {"
                "leaf-list l {"
                    "type string;"
                "}"
            "}"
        "}"
        "list lt {"
            "key \"k\";"
            "leaf k {"
                "type string;"
            "}"
        "}"
        "leaf d {"
            "type uint32;"
        "}"
        "leaf-list ll {"
            "type string;"
        "}"
        "container cont {"
            "list lt {"
                "key \"k\";"
                "leaf k {"
                    "type string;"
                "}"
            "}"
            "leaf d {"
                "type uint32;"
            "}"
            "leaf-list ll {"
                "type string;"
            "}"
            "leaf-list ll2 {"
                "type enumeration {"
                    "enum one;"
                    "enum two;"
                "}"
            "}"
        "}"
    "}";
const char *schema_f =
    "module f {"
        "namespace urn:tests:f;"
        "prefix f;"
        "yang-version 1.1;"

        "choice choic {"
            "default \"c\";"
            "leaf a {"
                "type string;"
            "}"
            "case b {"
                "leaf l {"
                    "type string;"
                "}"
            "}"
            "case c {"
                "leaf-list ll1 {"
                    "type string;"
                    "default \"def1\";"
                    "default \"def2\";"
                    "default \"def3\";"
                "}"
            "}"
        "}"
        "leaf d {"
            "type uint32;"
            "default 15;"
        "}"
        "leaf-list ll2 {"
            "type string;"
            "default \"dflt1\";"
            "default \"dflt2\";"
        "}"
        "container cont {"
            "choice choic {"
                "default \"c\";"
                "leaf a {"
                    "type string;"
                "}"
                "case b {"
                    "leaf l {"
                        "type string;"
                    "}"
                "}"
                "case c {"
                    "leaf-list ll1 {"
                        "type string;"
                        "default \"def1\";"
                        "default \"def2\";"
                        "default \"def3\";"
                    "}"
                "}"
            "}"
            "leaf d {"
                "type uint32;"
                "default 15;"
            "}"
            "leaf-list ll2 {"
                "type string;"
                "default \"dflt1\";"
                "default \"dflt2\";"
            "}"
        "}"
    "}";
const char *schema_g =
    "module g {"
        "namespace urn:tests:g;"
        "prefix g;"
        "yang-version 1.1;"

        "feature f1;"
        "feature f2;"
        "feature f3;"

        "container cont {"
            "if-feature \"f1\";"
            "choice choic {"
                "if-feature \"f2 or f3\";"
                "leaf a {"
                    "type string;"
                "}"
                "case b {"
                    "if-feature \"f2 and f1\";"
                    "leaf l {"
                        "type string;"
                    "}"
                "}"
            "}"
            "leaf d {"
                "type uint32;"
            "}"
            "container cont2 {"
                "if-feature \"f2\";"
                "leaf e {"
                    "type string;"
                "}"
            "}"
        "}"
    "}";
const char *schema_h =
    "module h {"
        "namespace urn:tests:h;"
        "prefix h;"
        "yang-version 1.1;"

        "container cont {"
            "container cont2 {"
                "config false;"
                "leaf l {"
                    "type string;"
                "}"
            "}"
        "}"
    "}";
const char *schema_i =
    "module i {"
        "namespace urn:tests:i;"
        "prefix i;"
        "yang-version 1.1;"

        "container cont {"
            "leaf l {"
                "type string;"
            "}"
            "leaf l2 {"
                "must \"../l = 'right'\";"
                "type string;"
            "}"
        "}"
    "}";
const char *schema_j =
    "module j {"
        "namespace urn:tests:j;"
        "prefix j;"
        "yang-version 1.1;"

        "feature feat1;"

        "container cont {"
            "must \"false()\";"
            "list l1 {"
                "key \"k\";"
                "leaf k {"
                    "type string;"
                "}"
                "action act {"
                    "if-feature feat1;"
                    "input {"
                        "must \"../../lf1 = 'true'\";"
                        "leaf lf2 {"
                            "type leafref {"
                                "path /lf3;"
                            "}"
                        "}"
                    "}"
                    "output {"
                        "must \"../../lf1 = 'true2'\";"
                        "leaf lf2 {"
                            "type leafref {"
                                "path /lf4;"
                            "}"
                        "}"
                    "}"
                "}"
            "}"

            "leaf lf1 {"
                "type string;"
            "}"
        "}"
        "leaf lf3 {"
            "type string;"
        "}"

        "leaf lf4 {"
            "type string;"
        "}"
    "}";

    const char *feats[] = {"feat1", NULL};


#define CONTEXT_CREATE() \
                CONTEXT_CREATE_PATH(TESTS_DIR_MODULES_YANG);\
                assert_non_null(ly_ctx_load_module(CONTEXT_GET, "ietf-netconf-with-defaults", "2011-06-01"));\
                assert_int_equal(LY_SUCCESS, lys_parse_mem(CONTEXT_GET, schema_a, LYS_IN_YANG, NULL));\
                assert_int_equal(LY_SUCCESS, lys_parse_mem(CONTEXT_GET, schema_b, LYS_IN_YANG, NULL));\
                assert_int_equal(LY_SUCCESS, lys_parse_mem(CONTEXT_GET, schema_c, LYS_IN_YANG, NULL));\
                assert_int_equal(LY_SUCCESS, lys_parse_mem(CONTEXT_GET, schema_d, LYS_IN_YANG, NULL));\
                assert_int_equal(LY_SUCCESS, lys_parse_mem(CONTEXT_GET, schema_e, LYS_IN_YANG, NULL));\
                assert_int_equal(LY_SUCCESS, lys_parse_mem(CONTEXT_GET, schema_f, LYS_IN_YANG, NULL));\
                assert_int_equal(LY_SUCCESS, lys_parse_mem(CONTEXT_GET, schema_g, LYS_IN_YANG, NULL));\
                assert_int_equal(LY_SUCCESS, lys_parse_mem(CONTEXT_GET, schema_h, LYS_IN_YANG, NULL));\
                assert_int_equal(LY_SUCCESS, lys_parse_mem(CONTEXT_GET, schema_i, LYS_IN_YANG, NULL));\
                assert_int_equal(LY_SUCCESS, lys_parse_mem(CONTEXT_GET, schema_j, LYS_IN_YANG, NULL));\
                {\
                    struct ly_in *in;\
                    assert_int_equal(LY_SUCCESS, ly_in_new_memory(schema_j, &in));\
                    assert_int_equal(LY_SUCCESS, lys_parse(ctx, in, LYS_IN_YANG, feats, NULL));\
                    ly_in_free(in, 0);\
                }\
                ly_set_log_clb(logger, 1)


#define MODEL_CREATE(INPUT, MODEL) \
                MODEL_CREATE_PARAM(INPUT, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_SUCCESS, "", MODEL)


#define MODEL_CHECK_CHAR(IN_MODEL, TEXT) \
                MODEL_CHECK_CHAR_PARAM(IN_MODEL, TEXT, LYD_XML, LYD_PRINT_SHRINK | LYD_PRINT_WITHSIBLINGS | LYD_PRINT_SHRINK)



void
logbuf_clean(void)
{
    logbuf[0] = '\0';
}

#if ENABLE_LOGGER_CHECKING
#   define logbuf_assert(str) assert_string_equal(logbuf, str)
#else
#   define logbuf_assert(str)
#endif

static void
test_when(void **state)
{
    (void) state;

    const char *data, *err_msg;
    struct lyd_node *tree;
    
    CONTEXT_CREATE();

    data    = "<c xmlns=\"urn:tests:a\">hey</c>";
    err_msg = "When condition \"/cont/b = 'val_b'\" not satisfied. /a:c";
    MODEL_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, err_msg, tree);


    data = "<cont xmlns=\"urn:tests:a\"><b>val_b</b></cont><c xmlns=\"urn:tests:a\">hey</c>";
    MODEL_CREATE(data, tree);
    LYSC_NODE_CHECK(tree->next->schema, LYS_LEAF, "c");
    assert_int_equal(LYD_WHEN_TRUE, tree->next->flags);
    MODEL_DESTROY(tree);

    data = "<cont xmlns=\"urn:tests:a\"><a>val</a><b>val_b</b></cont><c xmlns=\"urn:tests:a\">val_c</c>";

    MODEL_CREATE(data, tree);
    LYSC_NODE_CHECK(lyd_child(tree)->schema, LYS_LEAF, "a");
    assert_int_equal(LYD_WHEN_TRUE, lyd_child(tree)->flags);
    LYSC_NODE_CHECK(tree->next->schema, LYS_LEAF, "c");
    assert_int_equal(LYD_WHEN_TRUE, tree->next->flags);
    MODEL_DESTROY(tree);

    CONTEXT_DESTROY;
}

static void
test_mandatory(void **state)
{
    (void) state;

    CONTEXT_CREATE();

    const char *data, *err_msg;
    struct lyd_node *tree;

    data    = "<d xmlns=\"urn:tests:b\"/>";
    err_msg = "Mandatory node \"choic\" instance does not exist. /b:choic";
    MODEL_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, err_msg, tree);

    data    = "<l xmlns=\"urn:tests:b\">string</l><d xmlns=\"urn:tests:b\"/>";
    err_msg = "Mandatory node \"c\" instance does not exist. /b:c";
    MODEL_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, err_msg, tree);

    data    = "<a xmlns=\"urn:tests:b\">string</a>";
    err_msg = "Mandatory node \"c\" instance does not exist. /b:c";
    MODEL_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, err_msg, tree);

    data    = "<a xmlns=\"urn:tests:b\">string</a><c xmlns=\"urn:tests:b\">string2</c>";
    MODEL_CREATE(data, tree);
    lyd_free_siblings(tree);

    CONTEXT_DESTROY; 
}

static void
test_minmax(void **state)
{
    (void) state;

    const char *data, *err_msg;
    struct lyd_node *tree;

    CONTEXT_CREATE();

    data = "<d xmlns=\"urn:tests:c\"/>";
    err_msg = "Too few \"l\" instances. /c:choic/b/l";
    MODEL_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, err_msg, tree);

    data =
    "<l xmlns=\"urn:tests:c\">val1</l>"
    "<l xmlns=\"urn:tests:c\">val2</l>";
    err_msg = "Too few \"l\" instances. /c:choic/b/l";
    MODEL_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, err_msg, tree);


    data =
    "<l xmlns=\"urn:tests:c\">val1</l>"
    "<l xmlns=\"urn:tests:c\">val2</l>"
    "<l xmlns=\"urn:tests:c\">val3</l>";
    MODEL_CREATE(data, tree);
    MODEL_DESTROY(tree);

    data =
    "<l xmlns=\"urn:tests:c\">val1</l>"
    "<l xmlns=\"urn:tests:c\">val2</l>"
    "<l xmlns=\"urn:tests:c\">val3</l>"
    "<lt xmlns=\"urn:tests:c\"><k>val1</k></lt>"
    "<lt xmlns=\"urn:tests:c\"><k>val2</k></lt>"
    "<lt xmlns=\"urn:tests:c\"><k>val3</k></lt>"
    "<lt xmlns=\"urn:tests:c\"><k>val4</k></lt>"
    "<lt xmlns=\"urn:tests:c\"><k>val5</k></lt>";
    err_msg = "Too many \"lt\" instances. /c:lt";
    MODEL_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, err_msg, tree);

    CONTEXT_DESTROY;
}

static void
test_unique(void **state)
{

    (void) state;
    const char *data, *err_msg;
    struct lyd_node *tree;

    CONTEXT_CREATE();

    data =
    "<lt xmlns=\"urn:tests:d\">"
        "<k>val1</k>"
        "<l1>same</l1>"
    "</lt>"
    "<lt xmlns=\"urn:tests:d\">"
        "<k>val2</k>"
    "</lt>";
    MODEL_CREATE(data, tree);
    MODEL_DESTROY(tree);

    data =
    "<lt xmlns=\"urn:tests:d\">"
        "<k>val1</k>"
        "<l1>same</l1>"
    "</lt>"
    "<lt xmlns=\"urn:tests:d\">"
        "<k>val2</k>"
        "<l1>not-same</l1>"
    "</lt>";
    MODEL_CREATE(data, tree);
    MODEL_DESTROY(tree);

    data =
    "<lt xmlns=\"urn:tests:d\">"
        "<k>val1</k>"
        "<l1>same</l1>"
    "</lt>"
    "<lt xmlns=\"urn:tests:d\">"
        "<k>val2</k>"
        "<l1>same</l1>"
    "</lt>";
    err_msg = "Unique data leaf(s) \"l1\" not satisfied in \"/d:lt[k='val1']\" and \"/d:lt[k='val2']\". /d:lt[k='val2']";
    MODEL_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, err_msg, tree);

    /* now try with more instances */
    data =
    "<lt xmlns=\"urn:tests:d\">"
        "<k>val1</k>"
        "<l1>1</l1>"
    "</lt>"
    "<lt xmlns=\"urn:tests:d\">"
        "<k>val2</k>"
        "<l1>2</l1>"
    "</lt>"
    "<lt xmlns=\"urn:tests:d\">"
        "<k>val3</k>"
        "<l1>3</l1>"
    "</lt>"
    "<lt xmlns=\"urn:tests:d\">"
        "<k>val4</k>"
        "<l1>4</l1>"
    "</lt>"
    "<lt xmlns=\"urn:tests:d\">"
        "<k>val5</k>"
        "<l1>5</l1>"
    "</lt>"
    "<lt xmlns=\"urn:tests:d\">"
        "<k>val6</k>"
        "<l1>6</l1>"
    "</lt>"
    "<lt xmlns=\"urn:tests:d\">"
        "<k>val7</k>"
        "<l1>7</l1>"
    "</lt>"
    "<lt xmlns=\"urn:tests:d\">"
        "<k>val8</k>"
        "<l1>8</l1>"
    "</lt>";
    MODEL_CREATE(data, tree);
    MODEL_DESTROY(tree);

    data =
    "<lt xmlns=\"urn:tests:d\">"
        "<k>val1</k>"
        "<l1>1</l1>"
    "</lt>"
    "<lt xmlns=\"urn:tests:d\">"
        "<k>val2</k>"
        "<l1>2</l1>"
    "</lt>"
    "<lt xmlns=\"urn:tests:d\">"
        "<k>val3</k>"
        "<l1>3</l1>"
    "</lt>"
    "<lt xmlns=\"urn:tests:d\">"
        "<k>val4</k>"
    "</lt>"
    "<lt xmlns=\"urn:tests:d\">"
        "<k>val5</k>"
        "<l1>5</l1>"
    "</lt>"
    "<lt xmlns=\"urn:tests:d\">"
        "<k>val6</k>"
        "<l1>6</l1>"
    "</lt>"
    "<lt xmlns=\"urn:tests:d\">"
        "<k>val7</k>"
    "</lt>"
    "<lt xmlns=\"urn:tests:d\">"
        "<k>val8</k>"
    "</lt>";
    MODEL_CREATE(data, tree);
    MODEL_DESTROY(tree);

    data =
    "<lt xmlns=\"urn:tests:d\">"
        "<k>val1</k>"
        "<l1>1</l1>"
    "</lt>"
    "<lt xmlns=\"urn:tests:d\">"
        "<k>val2</k>"
        "<l1>2</l1>"
    "</lt>"
    "<lt xmlns=\"urn:tests:d\">"
        "<k>val3</k>"
    "</lt>"
    "<lt xmlns=\"urn:tests:d\">"
        "<k>val4</k>"
        "<l1>4</l1>"
    "</lt>"
    "<lt xmlns=\"urn:tests:d\">"
        "<k>val5</k>"
    "</lt>"
    "<lt xmlns=\"urn:tests:d\">"
        "<k>val6</k>"
    "</lt>"
    "<lt xmlns=\"urn:tests:d\">"
        "<k>val7</k>"
        "<l1>2</l1>"
    "</lt>"
    "<lt xmlns=\"urn:tests:d\">"
        "<k>val8</k>"
        "<l1>8</l1>"
    "</lt>";
    err_msg = "Unique data leaf(s) \"l1\" not satisfied in \"/d:lt[k='val7']\" and \"/d:lt[k='val2']\". /d:lt[k='val2']";
    MODEL_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, err_msg, tree);

    CONTEXT_DESTROY;
}

static void
test_unique_nested(void **state)
{
    (void) state;

    const char *data, *err_msg;
    struct lyd_node *tree;

    CONTEXT_CREATE();

    /* nested list uniquest are compared only with instances in the same parent list instance */
    data =
    "<lt2 xmlns=\"urn:tests:d\">"
        "<k>val1</k>"
        "<cont>"
            "<l2>1</l2>"
        "</cont>"
        "<l4>1</l4>"
    "</lt2>"
    "<lt2 xmlns=\"urn:tests:d\">"
        "<k>val2</k>"
        "<cont>"
            "<l2>2</l2>"
        "</cont>"
        "<l4>2</l4>"
        "<lt3>"
            "<kk>val1</kk>"
            "<l3>1</l3>"
        "</lt3>"
        "<lt3>"
            "<kk>val2</kk>"
            "<l3>2</l3>"
        "</lt3>"
    "</lt2>"
    "<lt2 xmlns=\"urn:tests:d\">"
        "<k>val3</k>"
        "<cont>"
            "<l2>3</l2>"
        "</cont>"
        "<l4>3</l4>"
        "<lt3>"
            "<kk>val1</kk>"
            "<l3>2</l3>"
        "</lt3>"
    "</lt2>"
    "<lt2 xmlns=\"urn:tests:d\">"
        "<k>val4</k>"
        "<cont>"
            "<l2>4</l2>"
        "</cont>"
        "<l4>4</l4>"
        "<lt3>"
            "<kk>val1</kk>"
            "<l3>3</l3>"
        "</lt3>"
    "</lt2>"
    "<lt2 xmlns=\"urn:tests:d\">"
        "<k>val5</k>"
        "<cont>"
            "<l2>5</l2>"
        "</cont>"
        "<l4>5</l4>"
        "<lt3>"
            "<kk>val1</kk>"
            "<l3>3</l3>"
        "</lt3>"
    "</lt2>";
    MODEL_CREATE(data, tree);
    MODEL_DESTROY(tree);

    data =
    "<lt2 xmlns=\"urn:tests:d\">"
        "<k>val1</k>"
        "<cont>"
            "<l2>1</l2>"
        "</cont>"
        "<l4>1</l4>"
    "</lt2>"
    "<lt2 xmlns=\"urn:tests:d\">"
        "<k>val2</k>"
        "<cont>"
            "<l2>2</l2>"
        "</cont>"
        "<lt3>"
            "<kk>val1</kk>"
            "<l3>1</l3>"
        "</lt3>"
        "<lt3>"
            "<kk>val2</kk>"
            "<l3>2</l3>"
        "</lt3>"
        "<lt3>"
            "<kk>val3</kk>"
            "<l3>1</l3>"
        "</lt3>"
    "</lt2>"
    "<lt2 xmlns=\"urn:tests:d\">"
        "<k>val3</k>"
        "<cont>"
            "<l2>3</l2>"
        "</cont>"
        "<l4>1</l4>"
        "<lt3>"
            "<kk>val1</kk>"
            "<l3>2</l3>"
        "</lt3>"
    "</lt2>"
    "<lt2 xmlns=\"urn:tests:d\">"
        "<k>val4</k>"
        "<cont>"
            "<l2>4</l2>"
        "</cont>"
        "<lt3>"
            "<kk>val1</kk>"
            "<l3>3</l3>"
        "</lt3>"
    "</lt2>"
    "<lt2 xmlns=\"urn:tests:d\">"
        "<k>val5</k>"
        "<cont>"
            "<l2>5</l2>"
        "</cont>"
        "<lt3>"
            "<kk>val1</kk>"
            "<l3>3</l3>"
        "</lt3>"
    "</lt2>";
    err_msg = "Unique data leaf(s) \"l3\" not satisfied in \"/d:lt2[k='val2']/lt3[kk='val3']\" and"
                  " \"/d:lt2[k='val2']/lt3[kk='val1']\". /d:lt2[k='val2']/lt3[kk='val1']";
    MODEL_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, err_msg, tree);


    data =
    "<lt2 xmlns=\"urn:tests:d\">"
        "<k>val1</k>"
        "<cont>"
            "<l2>1</l2>"
        "</cont>"
        "<l4>1</l4>"
    "</lt2>"
    "<lt2 xmlns=\"urn:tests:d\">"
        "<k>val2</k>"
        "<cont>"
            "<l2>2</l2>"
        "</cont>"
        "<l4>2</l4>"
    "</lt2>"
    "<lt2 xmlns=\"urn:tests:d\">"
        "<k>val3</k>"
        "<cont>"
            "<l2>3</l2>"
        "</cont>"
        "<l4>3</l4>"
    "</lt2>"
    "<lt2 xmlns=\"urn:tests:d\">"
        "<k>val4</k>"
        "<cont>"
            "<l2>2</l2>"
        "</cont>"
        "<l4>2</l4>"
    "</lt2>"
    "<lt2 xmlns=\"urn:tests:d\">"
        "<k>val5</k>"
        "<cont>"
            "<l2>5</l2>"
        "</cont>"
        "<l4>5</l4>"
    "</lt2>";
    err_msg = "Unique data leaf(s) \"cont/l2 l4\" not satisfied in \"/d:lt2[k='val4']\" and \"/d:lt2[k='val2']\". /d:lt2[k='val2']";
    MODEL_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, err_msg, tree);

    data =
    "<lt2 xmlns=\"urn:tests:d\">"
        "<k>val1</k>"
        "<cont>"
            "<l2>1</l2>"
        "</cont>"
        "<l4>1</l4>"
        "<l5>1</l5>"
        "<l6>1</l6>"
    "</lt2>"
    "<lt2 xmlns=\"urn:tests:d\">"
        "<k>val2</k>"
        "<cont>"
            "<l2>2</l2>"
        "</cont>"
        "<l4>1</l4>"
        "<l5>1</l5>"
    "</lt2>"
    "<lt2 xmlns=\"urn:tests:d\">"
        "<k>val3</k>"
        "<cont>"
            "<l2>3</l2>"
        "</cont>"
        "<l4>1</l4>"
        "<l5>3</l5>"
        "<l6>3</l6>"
    "</lt2>"
    "<lt2 xmlns=\"urn:tests:d\">"
        "<k>val4</k>"
        "<cont>"
            "<l2>4</l2>"
        "</cont>"
        "<l4>1</l4>"
        "<l6>1</l6>"
    "</lt2>"
    "<lt2 xmlns=\"urn:tests:d\">"
        "<k>val5</k>"
        "<cont>"
            "<l2>5</l2>"
        "</cont>"
        "<l4>1</l4>"
        "<l5>3</l5>"
        "<l6>3</l6>"
    "</lt2>";

    err_msg = "Unique data leaf(s) \"l5 l6\" not satisfied in \"/d:lt2[k='val5']\" and \"/d:lt2[k='val3']\". /d:lt2[k='val3']";
    MODEL_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, err_msg, tree);

    CONTEXT_DESTROY;
}

static void
test_dup(void **state)
{
    (void) state;

    const char *data, *err_msg;
    struct lyd_node *tree;

    CONTEXT_CREATE();

    data = "<d xmlns=\"urn:tests:e\">25</d><d xmlns=\"urn:tests:e\">50</d>";
    err_msg = "Duplicate instance of \"d\". /e:d";
    MODEL_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, err_msg, tree);

    data = "<lt xmlns=\"urn:tests:e\"><k>A</k></lt><lt xmlns=\"urn:tests:e\"><k>B</k></lt><lt xmlns=\"urn:tests:e\"><k>A</k></lt>";
    err_msg = "Duplicate instance of \"lt\". /e:lt[k='A']";
    MODEL_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, err_msg, tree);

    data = "<ll xmlns=\"urn:tests:e\">A</ll><ll xmlns=\"urn:tests:e\">B</ll><ll xmlns=\"urn:tests:e\">B</ll>";
    err_msg = "Duplicate instance of \"ll\". /e:ll[.='B']";
    MODEL_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, err_msg, tree);

    data = "<cont xmlns=\"urn:tests:e\"></cont><cont xmlns=\"urn:tests:e\"/>";
    err_msg = "Duplicate instance of \"cont\". /e:cont";
    MODEL_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, err_msg, tree);

    /* same tests again but using hashes */
    data = "<cont xmlns=\"urn:tests:e\"><d>25</d><d>50</d><ll>1</ll><ll>2</ll><ll>3</ll><ll>4</ll></cont>";
    err_msg = "Duplicate instance of \"d\". /e:cont/d";
    MODEL_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, err_msg, tree);

    data = "<cont xmlns=\"urn:tests:e\"><ll>1</ll><ll>2</ll><ll>3</ll><ll>4</ll>"
        "<lt><k>a</k></lt><lt><k>b</k></lt><lt><k>c</k></lt><lt><k>d</k></lt><lt><k>c</k></lt></cont>";
    err_msg = "Duplicate instance of \"lt\". /e:cont/lt[k='c']";
    MODEL_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, err_msg, tree);

    data = "<cont xmlns=\"urn:tests:e\"><ll>1</ll><ll>2</ll><ll>3</ll><ll>4</ll>"
        "<ll>a</ll><ll>b</ll><ll>c</ll><ll>d</ll><ll>d</ll></cont>";
    err_msg = "Duplicate instance of \"ll\". /e:cont/ll[.='d']";
    MODEL_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, err_msg, tree);

    /* cases */
    data = "<l xmlns=\"urn:tests:e\">a</l><l xmlns=\"urn:tests:e\">b</l><l xmlns=\"urn:tests:e\">c</l><l xmlns=\"urn:tests:e\">b</l>";
    err_msg = "Duplicate instance of \"l\". /e:l[.='b']";
    MODEL_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, err_msg, tree);

    data = "<l xmlns=\"urn:tests:e\">a</l><l xmlns=\"urn:tests:e\">b</l><l xmlns=\"urn:tests:e\">c</l><a xmlns=\"urn:tests:e\">aa</a>";
    err_msg = "Data for both cases \"a\" and \"b\" exist. /e:choic";
    MODEL_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, err_msg, tree);

    CONTEXT_DESTROY;
}

static void
test_defaults(void **state)
{
    (void) state;

    struct lyd_node *tree, *node, *diff;
    const struct lys_module *mod;
    const char *data, *err_msg;
    const char *str;

    CONTEXT_CREATE();
    mod = ly_ctx_get_module_latest(CONTEXT_GET, "f");

    /* get defaults */
    tree = NULL;
    assert_int_equal(lyd_validate_module(&tree, mod, 0, &diff), LY_SUCCESS);
    assert_non_null(tree);
    assert_non_null(diff);

    /* check all defaults exist */
    str = "<ll1 xmlns=\"urn:tests:f\" xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">def1</ll1>"
        "<ll1 xmlns=\"urn:tests:f\" xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">def2</ll1>"
        "<ll1 xmlns=\"urn:tests:f\" xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">def3</ll1>"
        "<d xmlns=\"urn:tests:f\" xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">15</d>"
        "<ll2 xmlns=\"urn:tests:f\" xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">dflt1</ll2>"
        "<ll2 xmlns=\"urn:tests:f\" xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">dflt2</ll2>"
        "<cont xmlns=\"urn:tests:f\">"
            "<ll1 xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">def1</ll1>"
            "<ll1 xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">def2</ll1>"
            "<ll1 xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">def3</ll1>"
            "<d xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">15</d>"
            "<ll2 xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">dflt1</ll2>"
            "<ll2 xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">dflt2</ll2>"
        "</cont>";
    MODEL_CHECK_CHAR_PARAM(tree, str, LYD_XML, LYD_PRINT_WD_IMPL_TAG | LYD_PRINT_WITHSIBLINGS | LYD_PRINT_SHRINK);
    //sigsegv
    //MODEL_CHECK_CHAR_PARAM(tree, str, LYD_XML, LYD_PRINT_SHRINK | LYD_PRINT_WITHSIBLINGS | LYD_PRINT_SHRINK)

    /* check diff */
    str = "<ll1 xmlns=\"urn:tests:f\" xmlns:yang=\"urn:ietf:params:xml:ns:yang:1\" yang:operation=\"create\">def1</ll1>"
        "<ll1 xmlns=\"urn:tests:f\" xmlns:yang=\"urn:ietf:params:xml:ns:yang:1\" yang:operation=\"create\">def2</ll1>"
        "<ll1 xmlns=\"urn:tests:f\" xmlns:yang=\"urn:ietf:params:xml:ns:yang:1\" yang:operation=\"create\">def3</ll1>"
        "<d xmlns=\"urn:tests:f\" xmlns:yang=\"urn:ietf:params:xml:ns:yang:1\" yang:operation=\"create\">15</d>"
        "<ll2 xmlns=\"urn:tests:f\" xmlns:yang=\"urn:ietf:params:xml:ns:yang:1\" yang:operation=\"create\">dflt1</ll2>"
        "<ll2 xmlns=\"urn:tests:f\" xmlns:yang=\"urn:ietf:params:xml:ns:yang:1\" yang:operation=\"create\">dflt2</ll2>"
        "<cont xmlns=\"urn:tests:f\" xmlns:yang=\"urn:ietf:params:xml:ns:yang:1\" yang:operation=\"create\">"
            "<ll1 yang:operation=\"create\">def1</ll1>"
            "<ll1 yang:operation=\"create\">def2</ll1>"
            "<ll1 yang:operation=\"create\">def3</ll1>"
            "<d yang:operation=\"create\">15</d>"
            "<ll2 yang:operation=\"create\">dflt1</ll2>"
            "<ll2 yang:operation=\"create\">dflt2</ll2>"
        "</cont>";

    MODEL_CHECK_CHAR_PARAM(diff, str, LYD_XML, LYD_PRINT_WD_ALL | LYD_PRINT_WITHSIBLINGS | LYD_PRINT_SHRINK);
    MODEL_DESTROY(diff);

    /* create another explicit case and validate */
    assert_int_equal(lyd_new_term(NULL, mod, "l", "value", 0, &node), LY_SUCCESS);
    assert_int_equal(lyd_insert_sibling(tree, node, &tree), LY_SUCCESS);
    assert_int_equal(lyd_validate_all(&tree, CONTEXT_GET, LYD_VALIDATE_PRESENT, &diff), LY_SUCCESS);

    /* check data tree */
    str = "<l xmlns=\"urn:tests:f\">value</l>"
        "<d xmlns=\"urn:tests:f\" xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">15</d>"
        "<ll2 xmlns=\"urn:tests:f\" xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">dflt1</ll2>"
        "<ll2 xmlns=\"urn:tests:f\" xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">dflt2</ll2>"
        "<cont xmlns=\"urn:tests:f\">"
            "<ll1 xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">def1</ll1>"
            "<ll1 xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">def2</ll1>"
            "<ll1 xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">def3</ll1>"
            "<d xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">15</d>"
            "<ll2 xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">dflt1</ll2>"
            "<ll2 xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">dflt2</ll2>"
        "</cont>";
    MODEL_CHECK_CHAR_PARAM(tree, str, LYD_XML, LYD_PRINT_WD_IMPL_TAG | LYD_PRINT_WITHSIBLINGS | LYD_PRINT_SHRINK);


    /* check diff */
    str = "<ll1 xmlns=\"urn:tests:f\" xmlns:yang=\"urn:ietf:params:xml:ns:yang:1\" yang:operation=\"delete\">def1</ll1>"
        "<ll1 xmlns=\"urn:tests:f\" xmlns:yang=\"urn:ietf:params:xml:ns:yang:1\" yang:operation=\"delete\">def2</ll1>"
        "<ll1 xmlns=\"urn:tests:f\" xmlns:yang=\"urn:ietf:params:xml:ns:yang:1\" yang:operation=\"delete\">def3</ll1>";
    MODEL_CHECK_CHAR_PARAM(diff, str, LYD_XML, LYD_PRINT_WD_ALL | LYD_PRINT_WITHSIBLINGS | LYD_PRINT_SHRINK);
    MODEL_DESTROY(diff);

    /* create explicit leaf-list and leaf and validate */
    assert_int_equal(lyd_new_term(NULL, mod, "d", "15", 0, &node), LY_SUCCESS);
    assert_int_equal(lyd_insert_sibling(tree, node, &tree), LY_SUCCESS);
    assert_int_equal(lyd_new_term(NULL, mod, "ll2", "dflt2", 0, &node), LY_SUCCESS);
    assert_int_equal(lyd_insert_sibling(tree, node, &tree), LY_SUCCESS);
    assert_int_equal(lyd_validate_all(&tree, CONTEXT_GET, LYD_VALIDATE_PRESENT, &diff), LY_SUCCESS);

    /* check data tree */
    str = "<l xmlns=\"urn:tests:f\">value</l>"
        "<d xmlns=\"urn:tests:f\">15</d>"
        "<ll2 xmlns=\"urn:tests:f\">dflt2</ll2>"
        "<cont xmlns=\"urn:tests:f\">"
            "<ll1 xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">def1</ll1>"
            "<ll1 xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">def2</ll1>"
            "<ll1 xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">def3</ll1>"
            "<d xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">15</d>"
            "<ll2 xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">dflt1</ll2>"
            "<ll2 xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">dflt2</ll2>"
        "</cont>";
    MODEL_CHECK_CHAR_PARAM(tree, str, LYD_XML, LYD_PRINT_WD_IMPL_TAG | LYD_PRINT_WITHSIBLINGS | LYD_PRINT_SHRINK);

    /* check diff */
    str = "<d xmlns=\"urn:tests:f\" xmlns:yang=\"urn:ietf:params:xml:ns:yang:1\" yang:operation=\"delete\">15</d>"
        "<ll2 xmlns=\"urn:tests:f\" xmlns:yang=\"urn:ietf:params:xml:ns:yang:1\" yang:operation=\"delete\">dflt1</ll2>"
        "<ll2 xmlns=\"urn:tests:f\" xmlns:yang=\"urn:ietf:params:xml:ns:yang:1\" yang:operation=\"delete\">dflt2</ll2>";
    MODEL_CHECK_CHAR_PARAM(diff, str, LYD_XML, LYD_PRINT_WD_ALL | LYD_PRINT_WITHSIBLINGS | LYD_PRINT_SHRINK);
    MODEL_DESTROY(diff);

    /* create first explicit container, which should become implicit */
    assert_int_equal(lyd_new_inner(NULL, mod, "cont", 0, &node), LY_SUCCESS);
    assert_int_equal(lyd_insert_sibling(tree, node, &tree), LY_SUCCESS);
    assert_int_equal(lyd_validate_all(&tree, CONTEXT_GET, LYD_VALIDATE_PRESENT, &diff), LY_SUCCESS);

    /* check data tree */
    str = "<l xmlns=\"urn:tests:f\">value</l>"
        "<d xmlns=\"urn:tests:f\">15</d>"
        "<ll2 xmlns=\"urn:tests:f\">dflt2</ll2>"
        "<cont xmlns=\"urn:tests:f\">"
            "<ll1 xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">def1</ll1>"
            "<ll1 xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">def2</ll1>"
            "<ll1 xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">def3</ll1>"
            "<d xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">15</d>"
            "<ll2 xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">dflt1</ll2>"
            "<ll2 xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">dflt2</ll2>"
        "</cont>";
    MODEL_CHECK_CHAR_PARAM(tree, str, LYD_XML, LYD_PRINT_WD_IMPL_TAG | LYD_PRINT_WITHSIBLINGS | LYD_PRINT_SHRINK);
    /* check diff */
    assert_null(diff);

    /* create second explicit container, which should become implicit, so the first tree node should be removed */
    assert_int_equal(lyd_new_inner(NULL, mod, "cont", 0, &node), LY_SUCCESS);
    assert_int_equal(lyd_insert_sibling(tree, node, &tree), LY_SUCCESS);
    assert_int_equal(lyd_validate_all(&tree, CONTEXT_GET, LYD_VALIDATE_PRESENT, &diff), LY_SUCCESS);

    /* check data tree */
    str = "<l xmlns=\"urn:tests:f\">value</l>"
        "<d xmlns=\"urn:tests:f\">15</d>"
        "<ll2 xmlns=\"urn:tests:f\">dflt2</ll2>"
        "<cont xmlns=\"urn:tests:f\">"
            "<ll1 xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">def1</ll1>"
            "<ll1 xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">def2</ll1>"
            "<ll1 xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">def3</ll1>"
            "<d xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">15</d>"
            "<ll2 xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">dflt1</ll2>"
            "<ll2 xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" ncwd:default=\"true\">dflt2</ll2>"
        "</cont>";
    MODEL_CHECK_CHAR_PARAM(tree, str, LYD_XML, LYD_PRINT_WD_IMPL_TAG | LYD_PRINT_WITHSIBLINGS | LYD_PRINT_SHRINK);
    /* check diff */
    assert_null(diff);

    /* similar changes for nested defaults */
    assert_int_equal(lyd_new_term(tree->prev, NULL, "ll1", "def3", 0, NULL), LY_SUCCESS);
    assert_int_equal(lyd_new_term(tree->prev, NULL, "d", "5", 0, NULL), LY_SUCCESS);
    assert_int_equal(lyd_new_term(tree->prev, NULL, "ll2", "non-dflt", 0, NULL), LY_SUCCESS);
    assert_int_equal(lyd_validate_all(&tree, CONTEXT_GET, LYD_VALIDATE_PRESENT, &diff), LY_SUCCESS);

    /* check data tree */
    str = "<l xmlns=\"urn:tests:f\">value</l>"
        "<d xmlns=\"urn:tests:f\">15</d>"
        "<ll2 xmlns=\"urn:tests:f\">dflt2</ll2>"
        "<cont xmlns=\"urn:tests:f\">"
            "<ll1>def3</ll1>"
            "<d>5</d>"
            "<ll2>non-dflt</ll2>"
        "</cont>";
    MODEL_CHECK_CHAR(tree, str);

    /* check diff */
    str = "<cont xmlns=\"urn:tests:f\" xmlns:yang=\"urn:ietf:params:xml:ns:yang:1\" yang:operation=\"none\">"
            "<ll1 yang:operation=\"delete\">def1</ll1>"
            "<ll1 yang:operation=\"delete\">def2</ll1>"
            "<ll1 yang:operation=\"delete\">def3</ll1>"
            "<d yang:operation=\"delete\">15</d>"
            "<ll2 yang:operation=\"delete\">dflt1</ll2>"
            "<ll2 yang:operation=\"delete\">dflt2</ll2>"
        "</cont>";
    MODEL_CHECK_CHAR_PARAM(diff, str, LYD_XML, LYD_PRINT_WD_ALL | LYD_PRINT_WITHSIBLINGS | LYD_PRINT_SHRINK);
    MODEL_DESTROY(diff);
    MODEL_DESTROY(tree);

    CONTEXT_DESTROY;
}

static void
test_state(void **state)
{
    (void) state;

    const char *data, *err_msg;
    struct lyd_node *tree;

    CONTEXT_CREATE();

    data =
    "<cont xmlns=\"urn:tests:h\">"
        "<cont2>"
            "<l>val</l>"
        "</cont2>"
    "</cont>";
    err_msg = "Invalid state data node \"cont2\" found. /h:cont/cont2";
    MODEL_CREATE_PARAM(data, LYD_XML, LYD_PARSE_ONLY | LYD_PARSE_NO_STATE, 0, LY_EVALID, err_msg, tree);

    MODEL_CREATE_PARAM(data, LYD_XML, LYD_PARSE_ONLY, 0, LY_SUCCESS, "", tree);
    assert_int_equal(LY_EVALID, lyd_validate_all(&tree, NULL, LYD_VALIDATE_PRESENT | LYD_VALIDATE_NO_STATE, NULL));
    logbuf_assert("Invalid state data node \"cont2\" found. /h:cont/cont2");

    MODEL_DESTROY(tree);

    CONTEXT_DESTROY;
}

static void
test_must(void **state)
{
    (void) state;
    const char *data, *err_msg;
    struct lyd_node *tree;

    CONTEXT_CREATE();

    data =
    "<cont xmlns=\"urn:tests:i\">"
        "<l>wrong</l>"
        "<l2>val</l2>"
    "</cont>";
    err_msg = "Must condition \"../l = 'right'\" not satisfied. /i:cont/l2";
    MODEL_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, err_msg, tree);

    data =
    "<cont xmlns=\"urn:tests:i\">"
        "<l>right</l>"
        "<l2>val</l2>"
    "</cont>";
    MODEL_CREATE(data, tree);
    MODEL_DESTROY(tree);
    
    CONTEXT_DESTROY;
}

static void
test_action(void **state)
{
    (void) state;

    const char *data, *err_msg;
    struct ly_in *in;
    struct lyd_node *tree, *op_tree;

    CONTEXT_CREATE();

    data =
    "<cont xmlns=\"urn:tests:j\">"
        "<l1>"
            "<k>val1</k>"
            "<act>"
                "<lf2>target</lf2>"
            "</act>"
        "</l1>"
    "</cont>";
    assert_int_equal(LY_SUCCESS, ly_in_new_memory(data, &in));
    assert_int_equal(LY_SUCCESS, lyd_parse_rpc(CONTEXT_GET, in, LYD_XML, &op_tree, NULL));
    assert_non_null(op_tree);

    /* missing leafref */
    assert_int_equal(LY_EVALID, lyd_validate_op(op_tree, NULL, LYD_VALIDATE_OP_RPC, NULL));
    logbuf_assert("Invalid leafref value \"target\" - no target instance \"/lf3\" with the same value."
        " /j:cont/l1[k='val1']/act/lf2");
    ly_in_free(in, 0);

    data =
    "<cont xmlns=\"urn:tests:j\">"
        "<lf1>not true</lf1>"
    "</cont>"
    "<lf3 xmlns=\"urn:tests:j\">target</lf3>";
    MODEL_CREATE_PARAM(data, LYD_XML, LYD_PARSE_ONLY | LYD_PARSE_TRUSTED, 0, LY_SUCCESS, "", tree);

    /* input must false */
    assert_int_equal(LY_EVALID, lyd_validate_op(op_tree, tree, LYD_VALIDATE_OP_RPC, NULL));
    logbuf_assert("Must condition \"../../lf1 = 'true'\" not satisfied. /j:cont/l1[k='val1']/act");

    MODEL_DESTROY(tree);
    data =
    "<cont xmlns=\"urn:tests:j\">"
        "<lf1>true</lf1>"
    "</cont>"
    "<lf3 xmlns=\"urn:tests:j\">target</lf3>";
    MODEL_CREATE_PARAM(data, LYD_XML,LYD_PARSE_ONLY | LYD_PARSE_TRUSTED, 0, LY_SUCCESS, "", tree);

    /* success */
    assert_int_equal(LY_SUCCESS, lyd_validate_op(op_tree, tree, LYD_VALIDATE_OP_RPC, NULL));

    lyd_free_tree(op_tree);
    lyd_free_siblings(tree);

    CONTEXT_DESTROY;
}

static void
test_reply(void **state)
{
    (void) state;
    const char *data, *err_msg;
    struct ly_in *in;
    struct lyd_node *tree, *op_tree, *request;

    CONTEXT_CREATE();

    data =
    "<cont xmlns=\"urn:tests:j\">"
        "<l1>"
            "<k>val1</k>"
            "<act>"
                "<lf2>target</lf2>"
            "</act>"
        "</l1>"
    "</cont>";
    assert_int_equal(LY_SUCCESS, ly_in_new_memory(data, &in));
    assert_int_equal(LY_SUCCESS, lyd_parse_rpc(CONTEXT_GET, in, LYD_XML, &request, NULL));
    assert_non_null(request);
    ly_in_free(in, 0);

    data = "<lf2 xmlns=\"urn:tests:j\">target</lf2>";
    assert_int_equal(LY_SUCCESS, ly_in_new_memory(data, &in));
    assert_int_equal(LY_SUCCESS, lyd_parse_reply(request, in, LYD_XML, &op_tree, NULL));
    lyd_free_all(request);
    assert_non_null(op_tree);
    ly_in_free(in, 0);

    /* missing leafref */
    assert_int_equal(LY_EVALID, lyd_validate_op(op_tree, NULL, LYD_VALIDATE_OP_REPLY, NULL));
    logbuf_assert("Invalid leafref value \"target\" - no target instance \"/lf4\" with the same value."
        " /j:cont/l1[k='val1']/act/lf2");

    data =
    "<cont xmlns=\"urn:tests:j\">"
        "<lf1>not true</lf1>"
    "</cont>"
    "<lf4 xmlns=\"urn:tests:j\">target</lf4>";
    MODEL_CREATE_PARAM(data, LYD_XML,LYD_PARSE_ONLY | LYD_PARSE_TRUSTED, 0, LY_SUCCESS, "", tree);

    /* input must false */
    assert_int_equal(LY_EVALID, lyd_validate_op(op_tree, tree, LYD_VALIDATE_OP_REPLY, NULL));
    logbuf_assert("Must condition \"../../lf1 = 'true2'\" not satisfied. /j:cont/l1[k='val1']/act");

    MODEL_DESTROY(tree);
    data =
    "<cont xmlns=\"urn:tests:j\">"
        "<lf1>true2</lf1>"
    "</cont>"
    "<lf4 xmlns=\"urn:tests:j\">target</lf4>";
    MODEL_CREATE_PARAM(data, LYD_XML,LYD_PARSE_ONLY | LYD_PARSE_TRUSTED, 0, LY_SUCCESS, "", tree);

    /* success */
    assert_int_equal(LY_SUCCESS, lyd_validate_op(op_tree, tree, LYD_VALIDATE_OP_REPLY, NULL));

    lyd_free_tree(op_tree);
    MODEL_DESTROY(tree);

    CONTEXT_DESTROY;
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_when),
        cmocka_unit_test(test_mandatory),
        cmocka_unit_test(test_minmax),
        cmocka_unit_test(test_unique),
        cmocka_unit_test(test_unique_nested),
        cmocka_unit_test(test_dup),
        cmocka_unit_test(test_defaults),
        cmocka_unit_test(test_state),
        cmocka_unit_test(test_must),
        cmocka_unit_test(test_action),
        cmocka_unit_test(test_reply),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
