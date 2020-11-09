/*
 * @file test_parser_xml.c
 * @author: Radek Krejci <rkrejci@cesnet.cz>
 * @brief unit tests for functions from parser_xml.c
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
#include <string.h>

#include "context.h"
#include "in.h"
#include "parser_data.h"
#include "out.h"
#include "printer_data.h"
#include "tests/config.h"
#include "tree_data_internal.h"
#include "tree_schema.h"


const char *schema_a = "module a {namespace urn:tests:a;prefix a;yang-version 1.1;"
            "list l1 { key \"a b c\"; leaf a {type string;} leaf b {type string;} leaf c {type int16;} leaf d {type string;}}"
            "leaf foo { type string;}"
            "container c {"
                "leaf x {type string;}"
                "action act { input { leaf al {type string;} } output { leaf al {type uint8;} } }"
                "notification n1 { leaf nl {type string;} }"
            "}"
            "container cp {presence \"container switch\"; leaf y {type string;} leaf z {type int8;}}"
            "anydata any {config false;}"
            "leaf foo2 { type string; default \"default-val\"; }"
            "leaf foo3 { type uint32; }"
            "notification n2;}";



#define CONTEXT_CREATE \
                ly_set_log_clb(logger_null, 1);\
                CONTEXT_CREATE_PATH(TESTS_DIR_MODULES_YANG);\
                {\
                    const struct lys_module *mod;\
                    assert_non_null((mod = ly_ctx_load_module(CONTEXT_GET, "ietf-netconf", "2011-06-01", feats)));\
                }\
                assert_non_null(ly_ctx_load_module(CONTEXT_GET, "ietf-netconf-with-defaults", "2011-06-01", NULL));\
                assert_int_equal(LY_SUCCESS, lys_parse_mem(CONTEXT_GET, schema_a, LYS_IN_YANG, NULL))\



#define LYD_NODE_CREATE(INPUT, PARSE_OPTION, MODEL) \
                LYD_NODE_CREATE_PARAM(INPUT, LYD_XML, PARSE_OPTION, LYD_VALIDATE_PRESENT, LY_SUCCESS, MODEL)

#define PARSER_CHECK_ERROR(INPUT, PARSE_OPTION, MODEL, RET_VAL, ERR_MESSAGE, ERR_PATH) \
                assert_int_equal(RET_VAL, lyd_parse_data_mem(CONTEXT_GET, data, LYD_XML, PARSE_OPTION, LYD_VALIDATE_PRESENT, &MODEL));\
                LY_ERROR_CHECK(CONTEXT_GET, ERR_MESSAGE, ERR_PATH);\
                assert_null(MODEL)



#define LYD_NODE_CHECK_CHAR(IN_MODEL, TEXT) \
                LYD_NODE_CHECK_CHAR_PARAM(IN_MODEL, TEXT, LYD_XML, LYD_PRINT_SHRINK | LYD_PRINT_WITHSIBLINGS)





#define logbuf_assert(str)\
    {\
        const char * err_msg[]  = {str};\
        const char * err_path[] = {NULL};\
        LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);\
    }

const char *feats[] = {"writable-running", NULL};

static void
test_leaf(void **state)
{
    *state = test_leaf;

    const char *data = "<foo xmlns=\"urn:tests:a\">foo value</foo>";
    struct lyd_node *tree;
    struct lyd_node_term *leaf;

    CONTEXT_CREATE;

    LYD_NODE_CREATE(data, 0, tree);
    LYSC_NODE_CHECK(tree->schema, LYS_LEAF, "foo");
    leaf = (struct lyd_node_term*)tree;
    LYD_VALUE_CHECK(leaf->value, STRING, "foo value");

    LYSC_NODE_CHECK(tree->next->next->schema, LYS_LEAF, "foo2");
    leaf = (struct lyd_node_term*)tree->next->next;
    LYD_VALUE_CHECK(leaf->value, STRING, "default-val");
    assert_true(leaf->flags & LYD_DEFAULT);

    LYD_NODE_DESTROY(tree);

    /* make foo2 explicit */
    data = "<foo2 xmlns=\"urn:tests:a\">default-val</foo2>";
    LYD_NODE_CREATE(data, 0, tree);
    LYSC_NODE_CHECK(tree->schema, LYS_LEAF, "foo2");
    leaf = (struct lyd_node_term*)tree;
    LYD_VALUE_CHECK(leaf->value, STRING, "default-val");
    assert_false(leaf->flags & LYD_DEFAULT);

    LYD_NODE_DESTROY(tree);

    /* parse foo2 but make it implicit, skip metadata xxx from missing schema */
    data = "<foo2 xmlns=\"urn:tests:a\" xmlns:wd=\"urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults\" wd:default=\"true\" xmlns:x=\"urn:x\" x:xxx=\"false\">default-val</foo2>";
    LYD_NODE_CREATE(data, 0, tree);
    LYSC_NODE_CHECK(tree->schema, LYS_LEAF, "foo2");
    leaf = (struct lyd_node_term*)tree;
    LYD_VALUE_CHECK(leaf->value, STRING, "default-val");
    assert_true(leaf->flags & LYD_DEFAULT);

    LYD_NODE_DESTROY(tree);

    CONTEXT_DESTROY;
}

static void
test_anydata(void **state)
{
    (void) state;

    const char *data;
    struct lyd_node *tree;


    CONTEXT_CREATE;

    data =
    "<any xmlns=\"urn:tests:a\">"
        "<element1>"
            "<x:element2 x:attr2=\"test\" xmlns:a=\"urn:tests:a\" xmlns:x=\"urn:x\">a:data</x:element2>"
        "</element1>"
        "<element1a/>"
    "</any>";
    LYD_NODE_CREATE(data, 0, tree);
    LYSC_NODE_CHECK(tree->schema, LYS_ANYDATA, "any");
    const char *data_expected = "<any xmlns=\"urn:tests:a\">"
            "<element1>"
                "<element2 xmlns=\"urn:x\" xmlns:x=\"urn:x\" x:attr2=\"test\" xmlns:a=\"urn:tests:a\">a:data</element2>"
            "</element1>"
            "<element1a/>"
        "</any>";
    LYD_NODE_CHECK_CHAR(tree, data_expected);

    LYD_NODE_DESTROY(tree);
    CONTEXT_DESTROY;
}

static void
test_list(void **state)
{
    (void) state;

    const char *err_msg[1];
    const char *err_path[1];
    const char *data = "<l1 xmlns=\"urn:tests:a\"><a>one</a><b>one</b><c>1</c></l1>";
    struct lyd_node *tree, *iter;
    struct lyd_node_inner *list;
    struct lyd_node_term *leaf;

    /* check hashes */
    CONTEXT_CREATE;

    LYD_NODE_CREATE(data, 0, tree);
    LYSC_NODE_CHECK(tree->schema, LYS_LIST, "l1");
    list = (struct lyd_node_inner*)tree;
    LY_LIST_FOR(list->child, iter) {
        assert_int_not_equal(0, iter->hash);
    }
    LYD_NODE_DESTROY(tree);

    /* missing keys */
    data = "<l1 xmlns=\"urn:tests:a\"><c>1</c><b>b</b></l1>";
    err_msg[0] =  "List instance is missing its key \"a\".";
    err_path[0]= "/a:l1[b='b'][c='1']";
    PARSER_CHECK_ERROR(data, 0, tree, LY_EVALID, err_msg, err_path);

    data = "<l1 xmlns=\"urn:tests:a\"><a>a</a></l1>";
    err_msg[0] =  "List instance is missing its key \"b\".";
    err_path[0]= "/a:l1[a='a']";
    PARSER_CHECK_ERROR(data, 0, tree, LY_EVALID, err_msg, err_path);

    data = "<l1 xmlns=\"urn:tests:a\"><b>b</b><a>a</a></l1>";
    err_msg[0] =  "List instance is missing its key \"c\".";
    err_path[0]= "/a:l1[a='a'][b='b']";
    PARSER_CHECK_ERROR(data, 0, tree, LY_EVALID, err_msg, err_path);

    /* key duplicate */
    data = "<l1 xmlns=\"urn:tests:a\"><c>1</c><b>b</b><a>a</a><c>1</c></l1>";
    err_msg[0] =  "Duplicate instance of \"c\".";
    err_path[0]= "/a:l1[a='a'][b='b'][c='1'][c='1']/c";
    PARSER_CHECK_ERROR(data, 0, tree, LY_EVALID, err_msg, err_path);

    /* keys order */
    data = "<l1 xmlns=\"urn:tests:a\"><d>d</d><a>a</a><c>1</c><b>b</b></l1>";
    LYD_NODE_CREATE(data, 0, tree);
    LYSC_NODE_CHECK(tree->schema, LYS_LIST, "l1");
    list = (struct lyd_node_inner*)tree;
    assert_non_null(leaf = (struct lyd_node_term*)list->child);
    LYSC_NODE_CHECK(leaf->schema, LYS_LEAF, "a");
    assert_non_null(leaf = (struct lyd_node_term*)leaf->next);
    LYSC_NODE_CHECK(leaf->schema, LYS_LEAF, "b");
    assert_non_null(leaf = (struct lyd_node_term*)leaf->next);
    LYSC_NODE_CHECK(leaf->schema, LYS_LEAF, "c");
    assert_non_null(leaf = (struct lyd_node_term*)leaf->next);
    LYSC_NODE_CHECK(leaf->schema, LYS_LEAF, "d");
    err_msg[0]  = "Invalid position of the key \"b\" in a list.";
    err_path[0] = NULL;
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    LYD_NODE_DESTROY(tree);

    data = "<l1 xmlns=\"urn:tests:a\"><c>1</c><b>b</b><a>a</a></l1>";
    LYD_NODE_CREATE(data, 0, tree);
    LYSC_NODE_CHECK(tree->schema, LYS_LIST, "l1");
    list = (struct lyd_node_inner*)tree;
    assert_non_null(leaf = (struct lyd_node_term*)list->child);
    LYSC_NODE_CHECK(leaf->schema, LYS_LEAF, "a");
    assert_non_null(leaf = (struct lyd_node_term*)leaf->next);
    LYSC_NODE_CHECK(leaf->schema, LYS_LEAF, "b");
    assert_non_null(leaf = (struct lyd_node_term*)leaf->next);
    LYSC_NODE_CHECK(leaf->schema, LYS_LEAF, "c");
    logbuf_assert("Invalid position of the key \"a\" in a list.");
    LYD_NODE_DESTROY(tree);

    err_msg[0]  = "Invalid position of the key \"b\" in a list.";
    err_path[0] = "Line number 1.";
    PARSER_CHECK_ERROR(data, LYD_PARSE_STRICT, tree, LY_EVALID, err_msg, err_path);

    CONTEXT_DESTROY;
}

static void
test_container(void **state)
{
    (void) state;

    const char *data = "<c xmlns=\"urn:tests:a\"/>";
    struct lyd_node *tree;
    struct lyd_node_inner *cont;

    CONTEXT_CREATE;
    LYD_NODE_CREATE(data, 0, tree);
    LYSC_NODE_CHECK(tree->schema, LYS_CONTAINER, "c");
    cont = (struct lyd_node_inner*)tree;
    assert_true(cont->flags & LYD_DEFAULT);
    LYD_NODE_DESTROY(tree);

    data = "<cp xmlns=\"urn:tests:a\"/>";
    LYD_NODE_CREATE(data, 0, tree);
    LYSC_NODE_CHECK(tree->schema, LYS_CONTAINER, "cp");
    cont = (struct lyd_node_inner*)tree;
    assert_false(cont->flags & LYD_DEFAULT);
    LYD_NODE_DESTROY(tree);

    CONTEXT_DESTROY;
}

static void
test_opaq(void **state)
{
    (void) state;
    const char *data;
    struct lyd_node *tree;
    const char *err_msg[1];
    const char *err_path[1];

    CONTEXT_CREATE;

    /* invalid value, no flags */
    data = "<foo3 xmlns=\"urn:tests:a\"/>";
    err_msg[0] =  "Invalid empty uint32 value.";
    err_path[0]= "/a:foo3";
    PARSER_CHECK_ERROR(data, 0, tree, LY_EVALID, err_msg, err_path);

    /* opaq flag */
    LYD_NODE_CREATE(data, LYD_PARSE_OPAQ, tree);
    LYD_NODE_OPAQ_CHECK((struct lyd_node_opaq *)tree, 0, 0, LYD_XML, "foo3", 0, 0, NULL,  0,  "");
    LYD_NODE_CHECK_CHAR(tree, "<foo3 xmlns=\"urn:tests:a\"/>");
    LYD_NODE_DESTROY(tree);

    /* missing key, no flags */
    data = "<l1 xmlns=\"urn:tests:a\"><a>val_a</a><b>val_b</b><d>val_d</d></l1>";
    err_msg[0] =  "List instance is missing its key \"c\".";
    err_path[0]= "/a:l1[a='val_a'][b='val_b']";
    PARSER_CHECK_ERROR(data, 0, tree, LY_EVALID, err_msg, err_path);

    /* opaq flag */
    LYD_NODE_CREATE(data, LYD_PARSE_OPAQ, tree);
    LYD_NODE_OPAQ_CHECK((struct lyd_node_opaq *)tree, 0, 0x1, LYD_XML, "l1", 0, 0, NULL,  0,  "");
    LYD_NODE_CHECK_CHAR(tree, data);
    LYD_NODE_DESTROY(tree);

    /* invalid key, no flags */
    data = "<l1 xmlns=\"urn:tests:a\"><a>val_a</a><b>val_b</b><c>val_c</c></l1>";
    err_msg[0] =  "Invalid int16 value \"val_c\".";
    err_path[0]= "/a:l1/c";
    PARSER_CHECK_ERROR(data, 0, tree, LY_EVALID, err_msg, err_path);

    /* opaq flag */
    LYD_NODE_CREATE(data, LYD_PARSE_OPAQ, tree);
    LYD_NODE_OPAQ_CHECK((struct lyd_node_opaq *)tree, 0, 0x1, LYD_XML, "l1", 0, 0, NULL,  0,  "");
    LYD_NODE_CHECK_CHAR(tree, data);
    LYD_NODE_DESTROY(tree);

    /* opaq flag and fail */
    assert_int_equal(LY_EVALID, lyd_parse_data_mem(CONTEXT_GET,
                "<a xmlns=\"ns\"><b>x</b><c xml:id=\"D\">1</c></a>",
                LYD_XML, LYD_PARSE_OPAQ, LYD_VALIDATE_PRESENT, &tree));
    err_msg[0]  = "Unknown XML prefix \"xml\".";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    CONTEXT_DESTROY;
}

static void
test_rpc(void **state)
{
    (void ) state;

    const char *data;
    struct ly_in *in;
    struct lyd_node *tree, *op;
    const struct lyd_node *node;

    data =
         "<rpc xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\" msgid=\"25\" custom-attr=\"val\">"
            "<edit-config>"
                "<target>"
                    "<running/>"
                "</target>"
                "<config xmlns:nc=\"urn:ietf:params:xml:ns:netconf:base:1.0\">"
                    "<l1 xmlns=\"urn:tests:a\" nc:operation=\"replace\">"
                        "<a>val_a</a>"
                        "<b>val_b</b>"
                        "<c>val_c</c>"
                    "</l1>"
                    "<cp xmlns=\"urn:tests:a\">"
                        "<z nc:operation=\"delete\"/>"
                    "</cp>"
                "</config>"
            "</edit-config>"
        "</rpc>";

    CONTEXT_CREATE;
    assert_int_equal(LY_SUCCESS, ly_in_new_memory(data, &in));
    assert_int_equal(LY_SUCCESS, lyd_parse_rpc(CONTEXT_GET, in, LYD_XML, &tree, &op));
    ly_in_free(in, 0);

    assert_non_null(op);
    LYSC_ACTION_CHECK(op->schema, LYS_RPC , "edit-config");

    assert_non_null(tree);
    assert_null(tree->schema);

    LYD_NODE_OPAQ_CHECK((struct lyd_node_opaq *)tree, 0x1, 0x1, LYD_XML, "rpc", 0, 0, NULL,  0,  "");
    node = lyd_child(tree);
    LYSC_ACTION_CHECK(node->schema, LYS_RPC , "edit-config");
    node = lyd_child(node)->next;
    LYSC_NODE_CHECK(node->schema, LYS_ANYXML, "config");

    node = ((struct lyd_node_any *)node)->value.tree;
    LYSC_NODE_CHECK(node->schema, LYS_CONTAINER, "cp");

    node = lyd_child(node);
    /* z has no value */
    LYD_NODE_OPAQ_CHECK((struct lyd_node_opaq *)node, 0x1, 0, LYD_XML, "z", 0, 0, NULL,  0,  "");
    node = node->parent->next;
    /* l1 key c has invalid value so it is at the end */
    LYD_NODE_OPAQ_CHECK((struct lyd_node_opaq *)node, 0x1, 0x1, LYD_XML, "l1", 0, 0, NULL,  0,  "");

    const char *str_expected = "<rpc xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\" msgid=\"25\" custom-attr=\"val\">"
            "<edit-config>"
                "<target>"
                    "<running/>"
                "</target>"
                "<config>"
                    "<cp xmlns=\"urn:tests:a\">"
                        "<z xmlns:nc=\"urn:ietf:params:xml:ns:netconf:base:1.0\" nc:operation=\"delete\"/>"
                    "</cp>"
                    "<l1 xmlns=\"urn:tests:a\" xmlns:nc=\"urn:ietf:params:xml:ns:netconf:base:1.0\" nc:operation=\"replace\">"
                        "<a>val_a</a>"
                        "<b>val_b</b>"
                        "<c>val_c</c>"
                    "</l1>"
                "</config>"
            "</edit-config>"
        "</rpc>";
    LYD_NODE_CHECK_CHAR(tree, str_expected);

    LYD_NODE_DESTROY(tree);
    CONTEXT_DESTROY;
    /* wrong namespace, element name, whatever... */
    /* TODO */

}

static void
test_action(void **state)
{
    (void) state;

    const char *data;
    struct ly_in *in;
    struct lyd_node *tree, *op;
    const struct lyd_node *node;

    CONTEXT_CREATE;

    data =
        "<rpc xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\" msgid=\"25\" custom-attr=\"val\">"
            "<action xmlns=\"urn:ietf:params:xml:ns:yang:1\">"
                "<c xmlns=\"urn:tests:a\">"
                    "<act>"
                        "<al>value</al>"
                    "</act>"
                "</c>"
            "</action>"
        "</rpc>";
    assert_int_equal(LY_SUCCESS, ly_in_new_memory(data, &in));
    assert_int_equal(LY_SUCCESS, lyd_parse_rpc(CONTEXT_GET, in, LYD_XML, &tree, &op));
    ly_in_free(in, 0);

    assert_non_null(op);
    LYSC_ACTION_CHECK(op->schema, LYS_ACTION,"act");


    LYD_NODE_OPAQ_CHECK((struct lyd_node_opaq *)tree, 0x1, 0x1, LYD_XML, "rpc", 0, 0, NULL,  0,  "");
    node = lyd_child(tree);
    LYD_NODE_OPAQ_CHECK((struct lyd_node_opaq *)node, 0x0, 0x1, LYD_XML, "action", 0, 0, NULL,  0,  "");

    const char *str_exp = "<rpc xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\" msgid=\"25\" custom-attr=\"val\">"
            "<action xmlns=\"urn:ietf:params:xml:ns:yang:1\">"
                "<c xmlns=\"urn:tests:a\">"
                    "<act>"
                        "<al>value</al>"
                    "</act>"
                "</c>"
            "</action>"
        "</rpc>";

    LYD_NODE_CHECK_CHAR(tree, str_exp);

    LYD_NODE_DESTROY(tree);
    CONTEXT_DESTROY;
    /* wrong namespace, element name, whatever... */
    /* TODO */
}

static void
test_notification(void **state)
{
    (void) state;

    const char *data;
    struct ly_in *in;
    struct lyd_node *tree, *ntf;
    const struct lyd_node *node;

    CONTEXT_CREATE;

    data =
        "<notification xmlns=\"urn:ietf:params:xml:ns:netconf:notification:1.0\">"
            "<eventTime>2037-07-08T00:01:00Z</eventTime>"
            "<c xmlns=\"urn:tests:a\">"
                "<n1>"
                    "<nl>value</nl>"
                "</n1>"
            "</c>"
        "</notification>";
    assert_int_equal(LY_SUCCESS, ly_in_new_memory(data, &in));
    assert_int_equal(LY_SUCCESS, lyd_parse_notif(CONTEXT_GET, in, LYD_XML, &tree, &ntf));
    ly_in_free(in, 0);

    assert_non_null(ntf);
    LYSC_NOTIF_CHECK(ntf->schema, "n1");

    LYD_NODE_OPAQ_CHECK((struct lyd_node_opaq *)tree, 0x0, 0x1, LYD_XML, "notification", 0, 0, NULL,  0,  "");
    node = lyd_child(tree);
    LYD_NODE_OPAQ_CHECK((struct lyd_node_opaq *)node, 0x0, 0, LYD_XML, "eventTime", 0, 0, NULL,  0,  "2037-07-08T00:01:00Z");
    node = node->next;
    LYSC_NODE_CHECK(node->schema, LYS_CONTAINER, "c");

    LYD_NODE_CHECK_CHAR(tree, data);
    LYD_NODE_DESTROY(tree);

    /* top-level notif without envelope */
    data = "<n2 xmlns=\"urn:tests:a\"/>";
    assert_int_equal(LY_SUCCESS, ly_in_new_memory(data, &in));
    assert_int_equal(LY_SUCCESS, lyd_parse_notif(CONTEXT_GET, in, LYD_XML, &tree, &ntf));
    ly_in_free(in, 0);

    assert_non_null(ntf);
    LYSC_NOTIF_CHECK(ntf->schema, "n2");

    assert_non_null(tree);
    assert_ptr_equal(ntf, tree);

    LYD_NODE_CHECK_CHAR(tree, data);
    LYD_NODE_DESTROY(tree);

    CONTEXT_DESTROY;
    /* wrong namespace, element name, whatever... */
    /* TODO */
}

static void
test_reply(void **state)
{
    (void )state;

    const char *data;
    struct ly_in *in;
    struct lyd_node *request, *tree, *op;
    const struct lyd_node *node;

    CONTEXT_CREATE;

    data =
        "<c xmlns=\"urn:tests:a\">"
            "<act>"
                "<al>value</al>"
            "</act>"
        "</c>";
    assert_int_equal(LY_SUCCESS, ly_in_new_memory(data, &in));
    assert_int_equal(LY_SUCCESS, lyd_parse_rpc(CONTEXT_GET, in, LYD_XML, &request, NULL));
    ly_in_free(in, 0);

    data =
        "<rpc-reply xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\" msgid=\"25\">"
            "<al xmlns=\"urn:tests:a\">25</al>"
        "</rpc-reply>";
    assert_int_equal(LY_SUCCESS, ly_in_new_memory(data, &in));
    assert_int_equal(LY_SUCCESS, lyd_parse_reply(request, in, LYD_XML, &tree, &op));
    ly_in_free(in, 0);
    lyd_free_all(request);

    assert_non_null(op);

    LYSC_ACTION_CHECK(op->schema, LYS_ACTION, "act");
    node = lyd_child(op);
    LYSC_NODE_CHECK(node->schema, LYS_LEAF, "al");
    assert_true(node->schema->flags & LYS_CONFIG_R);


    LYD_NODE_OPAQ_CHECK((struct lyd_node_opaq *)tree, 0x1, 0x1, LYD_XML, "rpc-reply", 0, 0, NULL,  0,  "");
    node = lyd_child(tree);
    LYSC_NODE_CHECK(node->schema, LYS_CONTAINER, "c");

    /* TODO print only rpc-reply node and then output subtree */
    LYD_NODE_CHECK_CHAR(lyd_child(op), "<al xmlns=\"urn:tests:a\">25</al>");
    LYD_NODE_DESTROY(tree);

    /* wrong namespace, element name, whatever... */
    /* TODO */
    CONTEXT_DESTROY;
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_leaf),
        cmocka_unit_test(test_anydata),
        cmocka_unit_test(test_list),
        cmocka_unit_test(test_container),
        cmocka_unit_test(test_opaq),
        cmocka_unit_test(test_rpc),
        cmocka_unit_test(test_action),
        cmocka_unit_test(test_notification),
        cmocka_unit_test(test_reply),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
