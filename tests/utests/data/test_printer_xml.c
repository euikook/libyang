/*
 * @file test_printer_xml.c
 * @author: Radek Krejci <rkrejci@cesnet.cz>
 * @brief unit tests for functions from printer_yang.c
 *
 * Copyright (c) 2019 CESNET, z.s.p.o.
 *
 * This source code is licensed under BSD 3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://opensource.org/licenses/BSD-3-Clause
 */

#include "../macros.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "context.h"
#include "parser_data.h"
#include "out.h"
#include "printer_data.h"
#include "tests/config.h"
#include "tree_schema.h"


struct state_s {
    void *func;
    struct ly_ctx *ctx;
};


const char *schema_a = "module defs {namespace urn:tests:defs;prefix d;yang-version 1.1;"
        "identity crypto-alg; identity interface-type; identity ethernet {base interface-type;} identity fast-ethernet {base ethernet;}}";
const char *schema_b = "module types {namespace urn:tests:types;prefix t;yang-version 1.1; import defs {prefix defs;}"
        "feature f; identity gigabit-ethernet { base defs:ethernet;}"
        "container cont {leaf leaftarget {type empty;}"
                        "list listtarget {key id; max-elements 5;leaf id {type uint8;} leaf value {type string;}"
                                         "action test {input {leaf a {type string;}} output {leaf b {type string;}}}}"
                        "leaf-list leaflisttarget {type uint8; max-elements 5;}}"
        "list list {key id; leaf id {type string;} leaf value {type string;} leaf-list targets {type string;}}"
        "list list2 {key \"id value\"; leaf id {type string;} leaf value {type string;}}"
        "list list_inst {key id; leaf id {type instance-identifier {require-instance true;}} leaf value {type string;}}"
        "list list_ident {key id; leaf id {type identityref {base defs:interface-type;}} leaf value {type string;}}"
        "leaf-list leaflisttarget {type string;}"
        "leaf binary {type binary {length 5 {error-message \"This base64 value must be of length 5.\";}}}"
        "leaf binary-norestr {type binary;}"
        "leaf int8 {type int8 {range 10..20;}}"
        "leaf uint8 {type uint8 {range 150..200;}}"
        "leaf int16 {type int16 {range -20..-10;}}"
        "leaf uint16 {type uint16 {range 150..200;}}"
        "leaf int32 {type int32;}"
        "leaf uint32 {type uint32;}"
        "leaf int64 {type int64;}"
        "leaf uint64 {type uint64;}"
        "leaf bits {type bits {bit zero; bit one {if-feature f;} bit two;}}"
        "leaf enums {type enumeration {enum white; enum yellow {if-feature f;}}}"
        "leaf dec64 {type decimal64 {fraction-digits 1; range 1.5..10;}}"
        "leaf dec64-norestr {type decimal64 {fraction-digits 18;}}"
        "leaf str {type string {length 8..10; pattern '[a-z ]*';}}"
        "leaf str-norestr {type string;}"
        "leaf bool {type boolean;}"
        "leaf empty {type empty;}"
        "leaf ident {type identityref {base defs:interface-type;}}"
        "leaf inst {type instance-identifier {require-instance true;}}"
        "leaf inst-noreq {type instance-identifier {require-instance false;}}"
        "leaf lref {type leafref {path /leaflisttarget; require-instance true;}}"
        "leaf lref2 {type leafref {path \"../list[id = current()/../str-norestr]/targets\"; require-instance true;}}"
        "leaf un1 {type union {"
          "type leafref {path /int8; require-instance true;}"
          "type union { type identityref {base defs:interface-type;} type instance-identifier {require-instance true;} }"
          "type string {length 1..20;}}}"
        "anydata any;"
        "rpc sum {input {leaf x {type uint8;} leaf y {type uint8;}} output {leaf result {type uint16;}}}}";
const char *schema_c =
"module defaults {"
    "namespace \"urn:defaults\";"
    "prefix d;"
    "leaf a {"
        "type union {"
            "type instance-identifier;"
            "type string;"
        "}"
        "default \"/d:b\";"
    "}"
    "leaf b {"
        "type string;"
    "}"
    "leaf c {"
        "type string;"
    "}"
"}";




#define CONTEXT_CREATE \
                ly_set_log_clb(logger_null, 1);\
                CONTEXT_CREATE_PATH(TESTS_DIR_MODULES_YANG);\
                assert_non_null(ly_ctx_load_module(CONTEXT_GET, "ietf-netconf-with-defaults", "2011-06-01", NULL));\
                assert_int_equal(LY_SUCCESS, lys_parse_mem(CONTEXT_GET, schema_a, LYS_IN_YANG, NULL));\
                assert_int_equal(LY_SUCCESS, lys_parse_mem(CONTEXT_GET, schema_b, LYS_IN_YANG, NULL));\
                assert_int_equal(LY_SUCCESS, lys_parse_mem(CONTEXT_GET, schema_c, LYS_IN_YANG, NULL))\


#define LYD_NODE_CREATE(INPUT, MODEL) \
                LYD_NODE_CREATE_PARAM(INPUT, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_SUCCESS, MODEL)

#define PARSER_CHECK_ERROR(INPUT, PARSE_OPTION, MODEL, RET_VAL, ERR_MESSAGE) \
                assert_int_equal(RET_VAL, lyd_parse_data_mem(CONTEXT_GET, data, LYD_XML, PARSE_OPTION, LYD_VALIDATE_PRESENT, &MODEL));\
                logbuf_assert(ERR_MESSAGE);\
                assert_null(MODEL)


#define LYD_NODE_CHECK_CHAR(IN_MODEL, TEXT, PARAM) \
                LYD_NODE_CHECK_CHAR_PARAM(IN_MODEL, TEXT, LYD_XML, PARAM | LYD_PRINT_WITHSIBLINGS | LYD_PRINT_SHRINK)


#define logbuf_assert(str)\
    {\
        const char * err_msg[]  = {str};\
        const char * err_path[] = {NULL};\
        LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);\
    }


static void
test_leaf(void **state)
{
    (void) state;
    struct lyd_node *tree;
    const char *data;
    const char *result;

    CONTEXT_CREATE;

    data = "<int8 xmlns=\"urn:tests:types\">\n 15 \t\n  </int8>";
    result = "<int8 xmlns=\"urn:tests:types\">15</int8>";
    LYD_NODE_CREATE(data, tree);
    LYD_NODE_CHECK_CHAR(tree, result, LYD_PRINT_SHRINK);
    LYD_NODE_DESTROY(tree);

    CONTEXT_DESTROY;
}

static void
test_anydata(void **state)
{
    (void)(state);
    struct lyd_node *tree;
    const char *data;

    CONTEXT_CREATE;

    data = "<any xmlns=\"urn:tests:types\"><somexml xmlns:x=\"url:x\" xmlns=\"example.com\"><x:x/></somexml></any>";
    LYD_NODE_CREATE(data, tree);
    /* canonized */
    data = "<any xmlns=\"urn:tests:types\"><somexml xmlns=\"example.com\"><x xmlns=\"url:x\"/></somexml></any>";
    LYD_NODE_CHECK_CHAR(tree, data, LYD_PRINT_SHRINK);
    LYD_NODE_DESTROY(tree);

    data = "<any xmlns=\"urn:tests:types\"/>";
    LYD_NODE_CREATE(data, tree);
    LYD_NODE_CHECK_CHAR(tree, data, LYD_PRINT_SHRINK);
    LYD_NODE_DESTROY(tree);

    data =
        "<any xmlns=\"urn:tests:types\">"
            "<cont>"
                "<defs:elem1 xmlns:defs=\"urn:tests:defs\">"
                    "<elem2 xmlns:defaults=\"urn:defaults\" defs:attr1=\"defaults:val\" attr2=\"/defaults:node/defs:node2\">"
                    "</elem2>"
                "</defs:elem1>"
            "</cont>"
        "</any>";
    LYD_NODE_CREATE(data, tree);
    /* cont should be normally parsed */
    LYSC_NODE_CHECK(tree->schema, LYS_ANYDATA, "any");
    LYD_NODE_ANY_CHECK((struct lyd_node_any *)tree, LYD_ANYDATA_DATATREE);
    LYSC_NODE_CHECK(((struct lyd_node_any *)tree)->value.tree->schema, LYS_CONTAINER, "cont");
    /* but its children not */
    assert_null(((struct lyd_node_inner *)(((struct lyd_node_any *)tree)->value.tree))->child->schema);
    /* canonized */
    data =
        "<any xmlns=\"urn:tests:types\">"
            "<cont>"
                "<elem1 xmlns=\"urn:tests:defs\">"
                    "<elem2 xmlns=\"urn:tests:types\" xmlns:defs=\"urn:tests:defs\" xmlns:defaults=\"urn:defaults\""
                    " defs:attr1=\"defaults:val\" attr2=\"/defaults:node/defs:node2\"/>"
                "</elem1>"
            "</cont>"
        "</any>";
    LYD_NODE_CHECK_CHAR(tree, data, LYD_PRINT_SHRINK);

    LYD_NODE_DESTROY(tree);
    CONTEXT_DESTROY;
}

static void
test_defaults(void **state)
{
    (void) state;
    struct lyd_node *tree;
    const char *data;
    const char *data_trim;
    const char *data_all;
    const char *data_all_tag;
    const char *data_impl_tag;

    CONTEXT_CREATE;

    /* standard default value */
    data = "<c xmlns=\"urn:defaults\">aa</c>";
    LYD_NODE_CREATE(data, tree);
    ROOT_GET(tree);
    LYD_NODE_CHECK_CHAR(tree, data, LYD_PRINT_WD_TRIM);

    data = "<a xmlns=\"urn:defaults\" xmlns:d=\"urn:defaults\">/d:b</a><c xmlns=\"urn:defaults\">aa</c>";
    LYD_NODE_CHECK_CHAR(tree, data, LYD_PRINT_WD_ALL);

    data = "<a xmlns=\"urn:defaults\" xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\""
        " ncwd:default=\"true\" xmlns:d=\"urn:defaults\">/d:b</a>"
        "<c xmlns=\"urn:defaults\">aa</c>";
    LYD_NODE_CHECK_CHAR(tree, data, LYD_PRINT_WD_ALL_TAG);

    data = "<a xmlns=\"urn:defaults\" xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\""
        " ncwd:default=\"true\" xmlns:d=\"urn:defaults\">/d:b</a>"
        "<c xmlns=\"urn:defaults\">aa</c>";
    LYD_NODE_CHECK_CHAR(tree, data, LYD_PRINT_WD_IMPL_TAG);
    LYD_NODE_DESTROY(tree);

    /* string value equal to the default but default is an unresolved instance-identifier, so they are not considered equal */
    data = "<a xmlns=\"urn:defaults\">/d:b</a><c xmlns=\"urn:defaults\">aa</c>";
    LYD_NODE_CREATE(data, tree);
    ROOT_GET(tree);
    LYD_NODE_CHECK_CHAR(tree, data, LYD_PRINT_WD_TRIM);
    LYD_NODE_CHECK_CHAR(tree, data, LYD_PRINT_WD_ALL);
    LYD_NODE_CHECK_CHAR(tree, data, LYD_PRINT_WD_ALL_TAG);
    LYD_NODE_CHECK_CHAR(tree, data, LYD_PRINT_WD_IMPL_TAG);
    LYD_NODE_DESTROY(tree);

    /* instance-identifier value equal to the default, should be considered equal */
    data = "<a xmlns=\"urn:defaults\" xmlns:d=\"urn:defaults\">/d:b</a><b xmlns=\"urn:defaults\">val</b><c xmlns=\"urn:defaults\">aa</c>";
    data_trim = "<b xmlns=\"urn:defaults\">val</b><c xmlns=\"urn:defaults\">aa</c>";
    data_all  = "<a xmlns=\"urn:defaults\" xmlns:d=\"urn:defaults\">/d:b</a><b xmlns=\"urn:defaults\">val</b><c xmlns=\"urn:defaults\">aa</c>";
    data_all_tag = "<a xmlns=\"urn:defaults\" xmlns:ncwd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\""
        " ncwd:default=\"true\" xmlns:d=\"urn:defaults\">/d:b</a>"
        "<b xmlns=\"urn:defaults\">val</b>"
        "<c xmlns=\"urn:defaults\">aa</c>";
    data_impl_tag = "<a xmlns=\"urn:defaults\" xmlns:d=\"urn:defaults\">/d:b</a><b xmlns=\"urn:defaults\">val</b><c xmlns=\"urn:defaults\">aa</c>";

    LYD_NODE_CREATE(data, tree);
    ROOT_GET(tree);
    LYD_NODE_CHECK_CHAR(tree, data_trim, LYD_PRINT_WD_TRIM);
    LYD_NODE_CHECK_CHAR(tree, data_all, LYD_PRINT_WD_ALL);
    LYD_NODE_CHECK_CHAR(tree, data_all_tag, LYD_PRINT_WD_ALL_TAG);
    LYD_NODE_CHECK_CHAR(tree, data_impl_tag, LYD_PRINT_WD_IMPL_TAG);
    LYD_NODE_DESTROY(tree);

    CONTEXT_DESTROY;
}

#if 0

static void
test_rpc(void **state)
{
    struct state_s *s = (struct state_s*)(*state);
    struct lyd_node *tree1;
    struct lyd_node *tree2;
    const struct lyd_node **trees;
    const char *request;
    const char *reply, *result;
    char *printed;
    ssize_t len;
    struct ly_out *out;

    s->func = test_rpc;
    assert_non_null(out = ly_out_new_memory(&printed, 0));

    request = "<sum xmlns=\"urn:tests:types\"><x>10</x><y>20</y></sum>";
    reply = "<result xmlns=\"urn:tests:types\">30</result>";
    result = "<sum xmlns=\"urn:tests:types\"><result>30</result></sum>";
    assert_non_null(tree1 = lyd_parse_mem(s->ctx, request, LYD_XML, LYD_OPT_RPC, NULL));
    assert_true((len = lyd_print_tree(out, tree1, LYD_XML, LYD_PRINT_SHRINK)) >= 0);
    assert_int_equal(len, strlen(printed));
    assert_string_equal(printed, request);
    ly_out_reset(out);
    assert_non_null(trees = lyd_trees_new(1, tree1));
    assert_non_null(tree2 = lyd_parse_mem(s->ctx, reply, LYD_XML, LYD_OPT_RPCREPLY, trees));
    assert_true((len = lyd_print_tree(out, tree2, LYD_XML, LYD_PRINT_SHRINK)) >= 0);
    assert_int_equal(len, strlen(printed));
    assert_string_equal(printed, result);
    ly_out_reset(out);
    lyd_trees_free(trees, 0);
    lyd_free_all(tree1);
    lyd_free_all(tree2);

    /* no arguments */
    request = "<sum xmlns=\"urn:tests:types\"/>";
    reply = "";
    result = "<sum xmlns=\"urn:tests:types\"/>";
    assert_non_null(tree1 = lyd_parse_mem(s->ctx, request, LYD_XML, LYD_OPT_RPC, NULL));
    assert_true((len = lyd_print_tree(out, tree1, LYD_XML, LYD_PRINT_SHRINK)) >= 0);
    assert_int_equal(len, strlen(printed));
    assert_string_equal(printed, request);
    ly_out_reset(out);
    assert_non_null(trees = lyd_trees_new(1, tree1));
    assert_non_null(tree2 = lyd_parse_mem(s->ctx, reply, LYD_XML, LYD_OPT_RPCREPLY, trees));
    assert_true((len = lyd_print_tree(out, tree2, LYD_XML, LYD_PRINT_SHRINK)) >= 0);
    assert_int_equal(len, strlen(printed));
    assert_string_equal(printed, result);
    ly_out_reset(out);
    lyd_trees_free(trees, 0);
    lyd_free_all(tree1);
    lyd_free_all(tree2);

    /* action
     * "container cont {leaf leaftarget {type empty;}"
                            "list listtarget {key id; max-elements 5;leaf id {type uint8;} leaf value {type string;}"
                                             "action test {input {leaf a {type string;}} output {leaf b {type string;}}}}"
                            "leaf-list leaflisttarget {type uint8; max-elements 5;}}"
                            */
    request = "<cont xmlns=\"urn:tests:types\"><listtarget><id>10</id><test><a>test</a></test></listtarget></cont>";
    reply = "<b xmlns=\"urn:tests:types\">test-reply</b>";
    result = "<cont xmlns=\"urn:tests:types\"><listtarget><id>10</id><test><b>test-reply</b></test></listtarget></cont>";;
    assert_non_null(tree1 = lyd_parse_mem(s->ctx, request, LYD_XML, LYD_OPT_RPC, NULL));
    assert_true((len = lyd_print_tree(out, tree1, LYD_XML, LYD_PRINT_SHRINK)) >= 0);
    assert_int_equal(len, strlen(printed));
    assert_string_equal(printed, request);
    ly_out_reset(out);
    assert_non_null(trees = lyd_trees_new(1, tree1));
    assert_non_null(tree2 = lyd_parse_mem(s->ctx, reply, LYD_XML, LYD_OPT_RPCREPLY, trees));
    assert_true((len = lyd_print_tree(out, tree2, LYD_XML, LYD_PRINT_SHRINK)) >= 0);
    assert_int_equal(len, strlen(printed));
    assert_string_equal(printed, result);
    ly_out_reset(out);
    lyd_trees_free(trees, 0);
    lyd_free_all(tree1);
    lyd_free_all(tree2);

    ly_out_free(out, NULL, 1);
    s->func = NULL;
}

#endif

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_leaf),
        cmocka_unit_test(test_anydata),
        cmocka_unit_test(test_defaults),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
