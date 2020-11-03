/**
 * @file test_tree_schema.c
 * @author: Radek Krejci <rkrejci@cesnet.cz>
 * @brief unit tests for functions from tress_data.c
 *
 * Copyright (c) 2018-2019 CESNET, z.s.p.o.
 *
 * This source code is licensed under BSD 3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://opensource.org/licenses/BSD-3-Clause
 */

#include "../macros.h"

#include "libyang.h"
#include "common.h"
#include "path.h"
#include "xpath.h"

    const char *schema_a = "module a {namespace urn:tests:a;prefix a;yang-version 1.1;"
            "leaf bar {type string;}"
            "list l1 { key \"a b\"; leaf a {type string;} leaf b {type string;} leaf c {type string;}}"
            "leaf foo { type string;}"
            "leaf-list ll { type string;}"
            "container c {leaf-list x {type string;}}"
            "anydata any {config false;}"
            "list l2 {config false;"
                "container c{leaf x {type string;} leaf-list d {type string;}}"
            "}}";


#define CONTEXT_CREATE \
                ly_set_log_clb(logger_null, 1);\
                CONTEXT_CREATE_PATH(NULL);\
                assert_int_equal(LY_SUCCESS, lys_parse_mem(CONTEXT_GET, schema_a, LYS_IN_YANG, NULL))\


#define LYD_NODE_CREATE(INPUT, MODEL) \
                LYD_NODE_CREATE_PARAM(INPUT, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_SUCCESS, MODEL)


#define LYD_NODE_CHECK_CHAR(IN_MODEL, TEXT) \
                LYD_NODE_CHECK_CHAR_PARAM(IN_MODEL, TEXT, LYD_XML, LYD_PRINT_SHRINK | LYD_PRINT_WITHSIBLINGS | LYD_PRINT_SHRINK)



#define logbuf_assert(str)\
    {\
        const char * err_msg[]  = {str};\
        const char * err_path[] = {NULL};\
        LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);\
    }

static void
test_compare(void **state)
{
    (void) state;

    struct lyd_node *tree1, *tree2;

    const char *data1 = "<l1 xmlns=\"urn:tests:a\"><a>a</a><b>b</b><c>x</c></l1>";
    const char *data2 = "<l1 xmlns=\"urn:tests:a\"><a>a</a><b>b</b><c>y</c></l1>";

    CONTEXT_CREATE;

    assert_int_equal(LY_SUCCESS, lyd_compare_single(NULL, NULL, 0));

    LYD_NODE_CREATE(data1, tree1);
    LYD_NODE_CREATE(data2, tree2);
    assert_int_equal(LY_SUCCESS, lyd_compare_single(tree1, tree2, 0));
    assert_int_equal(LY_ENOT, lyd_compare_single(tree1, tree2, LYD_COMPARE_FULL_RECURSION));
    assert_int_equal(LY_ENOT, lyd_compare_single(((struct lyd_node_inner*)tree1)->child, tree2, 0));
    LYD_NODE_DESTROY(tree1);
    LYD_NODE_DESTROY(tree2);

    data1 = "<l2 xmlns=\"urn:tests:a\"><c><x>a</x></c></l2><l2 xmlns=\"urn:tests:a\"><c><x>b</x></c></l2>";
    data2 = "<l2 xmlns=\"urn:tests:a\"><c><x>b</x></c></l2>";
    LYD_NODE_CREATE(data1, tree1);
    LYD_NODE_CREATE(data2, tree2);
    assert_int_equal(LY_ENOT, lyd_compare_single(tree1, tree2, 0));
    assert_int_equal(LY_SUCCESS, lyd_compare_single(tree1->next, tree2, 0));
    LYD_NODE_DESTROY(tree1);
    LYD_NODE_DESTROY(tree2);

    data1 = "<ll xmlns=\"urn:tests:a\">a</ll><ll xmlns=\"urn:tests:a\">b</ll>";
    data2 = "<ll xmlns=\"urn:tests:a\">b</ll>";
    LYD_NODE_CREATE(data1, tree1);
    LYD_NODE_CREATE(data2, tree2);
    assert_int_equal(LY_ENOT, lyd_compare_single(tree1, tree2, 0));
    assert_int_equal(LY_ENOT, lyd_compare_single(NULL, tree2, 0));
    assert_int_equal(LY_ENOT, lyd_compare_single(tree1, NULL, 0));
    assert_int_equal(LY_SUCCESS, lyd_compare_single(tree1->next, tree2, 0));
    LYD_NODE_DESTROY(tree1);
    LYD_NODE_DESTROY(tree2);

    data1 = "<c xmlns=\"urn:tests:a\"><x>x</x></c>";
    data2 = "<c xmlns=\"urn:tests:a\"><x>y</x></c>";
    LYD_NODE_CREATE(data1, tree1);
    LYD_NODE_CREATE(data2, tree2);
    assert_int_equal(LY_SUCCESS, lyd_compare_single(tree1, tree2, 0));
    assert_int_equal(LY_ENOT, lyd_compare_single(tree1, tree2, LYD_COMPARE_FULL_RECURSION));
    LYD_NODE_DESTROY(tree1);
    LYD_NODE_DESTROY(tree2);

    data1 = "<c xmlns=\"urn:tests:a\"><x>x</x></c>";
    data2 = "<c xmlns=\"urn:tests:a\"><x>x</x><x>y</x></c>";
    LYD_NODE_CREATE(data1, tree1);
    LYD_NODE_CREATE(data2, tree2);
    assert_int_equal(LY_SUCCESS, lyd_compare_single(tree1, tree2, 0));
    assert_int_equal(LY_ENOT, lyd_compare_single(tree1, tree2, LYD_COMPARE_FULL_RECURSION));
    LYD_NODE_DESTROY(tree1);
    LYD_NODE_DESTROY(tree2);

    data1 = "<any xmlns=\"urn:tests:a\"><x>x</x></any>";
    data2 = "<any xmlns=\"urn:tests:a\"><x>x</x><x>y</x></any>";
    LYD_NODE_CREATE(data1, tree1);
    LYD_NODE_CREATE(data2, tree2);
    assert_int_equal(LY_ENOT, lyd_compare_single(tree1, tree2, 0));
    LYD_NODE_DESTROY(tree1);
    data1 = "<any xmlns=\"urn:tests:a\"><x>x</x><x>y</x></any>";
    LYD_NODE_CREATE(data1, tree1);
    assert_int_equal(LY_SUCCESS, lyd_compare_single(tree1, tree2, 0));
    LYD_NODE_DESTROY(tree1);
    LYD_NODE_DESTROY(tree2);

    CONTEXT_DESTROY;
}

static void
test_dup(void **state)
{
    (void) state;

    struct lyd_node *tree1, *tree2;
    const char *result;
    const char *data = "<l1 xmlns=\"urn:tests:a\"><a>a</a><b>b</b><c>x</c></l1>";

    CONTEXT_CREATE;
    LYD_NODE_CREATE(data, tree1);
    assert_int_equal(LY_SUCCESS, lyd_dup_single(tree1, NULL, LYD_DUP_RECURSIVE, &tree2));
    assert_int_equal(LY_SUCCESS, lyd_compare_single(tree1, tree2, LYD_COMPARE_FULL_RECURSION));
    LYD_NODE_DESTROY(tree1);
    LYD_NODE_DESTROY(tree2);

    data = "<l1 xmlns=\"urn:tests:a\"><a>a</a><b>b</b><c>x</c></l1>";
    result = "<l1 xmlns=\"urn:tests:a\"><a>a</a><b>b</b></l1>";
    LYD_NODE_CREATE(data, tree1);
    assert_int_equal(LY_SUCCESS, lyd_dup_single(tree1, NULL, 0, &tree2));
    LYD_NODE_DESTROY(tree1);
    LYD_NODE_CREATE(result, tree1);
    assert_int_equal(LY_SUCCESS, lyd_compare_single(tree1, tree2, LYD_COMPARE_FULL_RECURSION));
    LYD_NODE_DESTROY(tree1);
    LYD_NODE_DESTROY(tree2);

    data = "<l2 xmlns=\"urn:tests:a\"><c><x>a</x></c></l2><l2 xmlns=\"urn:tests:a\"><c><x>b</x></c></l2>";
    result = "<l2 xmlns=\"urn:tests:a\"><c><x>a</x></c></l2>";
    LYD_NODE_CREATE(data, tree1);
    assert_int_equal(LY_SUCCESS, lyd_dup_siblings(tree1, NULL, LYD_DUP_RECURSIVE, &tree2));
    assert_int_equal(LY_SUCCESS, lyd_compare_single(tree1, tree2, LYD_COMPARE_FULL_RECURSION));
    LYD_NODE_DESTROY(tree2);
    assert_int_equal(LY_SUCCESS, lyd_dup_single(tree1, NULL, LYD_DUP_RECURSIVE, &tree2));
    LYD_NODE_DESTROY(tree1);
    LYD_NODE_CREATE(result, tree1);
    assert_int_equal(LY_SUCCESS, lyd_compare_single(tree1, tree2, LYD_COMPARE_FULL_RECURSION));
    LYD_NODE_DESTROY(tree2);

    assert_int_equal(LY_SUCCESS, lyd_dup_single(tree1, NULL, 0, &tree2));
    LYD_NODE_DESTROY(tree1);
    result = "<l2 xmlns=\"urn:tests:a\"/>";
    LYD_NODE_CREATE_PARAM(result, LYD_XML, LYD_PARSE_ONLY, 0, LY_SUCCESS, tree1);
    assert_int_equal(LY_SUCCESS, lyd_compare_single(tree1, tree2, LYD_COMPARE_FULL_RECURSION));
    LYD_NODE_DESTROY(tree1);
    LYD_NODE_DESTROY(tree2);

    data = "<any xmlns=\"urn:tests:a\"><c><a>a</a></c></any>";
    LYD_NODE_CREATE(data, tree1);
    assert_int_equal(LY_SUCCESS, lyd_dup_single(tree1, NULL, 0, &tree2));
    assert_int_equal(LY_SUCCESS, lyd_compare_single(tree1, tree2, LYD_COMPARE_FULL_RECURSION));
    LYD_NODE_DESTROY(tree1);
    LYD_NODE_DESTROY(tree2);

    data = "<l2 xmlns=\"urn:tests:a\"><c><x>b</x></c></l2>";
    LYD_NODE_CREATE(data, tree1);
    assert_int_equal(LY_SUCCESS, lyd_dup_single(((struct lyd_node_inner*)((struct lyd_node_inner*)tree1)->child)->child, NULL,
                                         LYD_DUP_WITH_PARENTS, &tree2));
    LYSC_NODE_CHECK(tree2->schema, LYS_LEAF, "x");
    assert_non_null(tree2->parent);
    assert_int_equal(LY_SUCCESS, lyd_compare_single(tree1, (struct lyd_node*)tree2->parent->parent, LYD_COMPARE_FULL_RECURSION));
    LYD_NODE_DESTROY(tree1);
    LYD_NODE_DESTROY(tree2);

    data = "<l1 xmlns=\"urn:tests:a\"><a>a</a><b>b</b><c>c</c></l1>";
    LYD_NODE_CREATE(data, tree1);
    assert_int_equal(LY_SUCCESS, lyd_dup_single(((struct lyd_node_inner*)tree1)->child->prev, NULL,
                                                LYD_DUP_WITH_PARENTS, &tree2));
    LYSC_NODE_CHECK(tree2->schema, LYS_LEAF, "c");
    assert_non_null(tree2->parent);
    assert_int_equal(LY_SUCCESS, lyd_compare_single(tree1, (struct lyd_node*)tree2->parent, LYD_COMPARE_FULL_RECURSION));
    LYD_NODE_DESTROY(tree1);
    LYD_NODE_DESTROY(tree2);

    data = "<l2 xmlns=\"urn:tests:a\"><c><x>b</x></c></l2>";
    LYD_NODE_CREATE(data, tree1);
    assert_int_equal(LY_SUCCESS, lyd_dup_single(tree1, NULL, 0, &tree2));
    assert_int_equal(LY_SUCCESS, lyd_dup_single(((struct lyd_node_inner*)((struct lyd_node_inner*)tree1)->child)->child,
                                 (struct lyd_node_inner*)tree2, LYD_DUP_WITH_PARENTS, NULL));
    assert_int_equal(LY_SUCCESS, lyd_compare_single(tree1, tree2, LYD_COMPARE_FULL_RECURSION));
    LYD_NODE_DESTROY(tree1);
    LYD_NODE_DESTROY(tree2);

    /* invalid */
    data = "<l1 xmlns=\"urn:tests:a\"><a>a</a><b>b</b><c>c</c></l1><l2 xmlns=\"urn:tests:a\"><c><x>b</x></c></l2>";
    LYD_NODE_CREATE(data, tree1);
    assert_int_equal(LY_EINVAL, lyd_dup_single(((struct lyd_node_inner*)tree1)->child->prev,
                                               (struct lyd_node_inner*)tree1->next, LYD_DUP_WITH_PARENTS, NULL));
    LYD_NODE_DESTROY(tree1);

    CONTEXT_DESTROY;
}

static void
test_target(void **state)
{
    (void) state;
    *state = test_target;

    const struct lyd_node_term *term;
    struct lyd_node *tree;
    struct lyxp_expr *exp;
    struct ly_path *path;
    const char *path_str = "/a:l2[2]/c/d[3]";
    const char *data =
        "<l2 xmlns=\"urn:tests:a\"><c>"
            "<d>a</d>"
        "</c></l2>"
        "<l2 xmlns=\"urn:tests:a\"><c>"
            "<d>a</d>"
            "<d>b</d>"
            "<d>b</d>"
            "<d>c</d>"
        "</c></l2>"
        "<l2 xmlns=\"urn:tests:a\"><c>"
        "</c></l2>";

    CONTEXT_CREATE;

    LYD_NODE_CREATE(data, tree);
    assert_int_equal(LY_SUCCESS, ly_path_parse(CONTEXT_GET, NULL, path_str, strlen(path_str), LY_PATH_BEGIN_EITHER, LY_PATH_LREF_FALSE,
                                               LY_PATH_PREFIX_OPTIONAL, LY_PATH_PRED_SIMPLE, &exp));
    assert_int_equal(LY_SUCCESS, ly_path_compile(CONTEXT_GET, NULL, NULL, exp, LY_PATH_LREF_FALSE, LY_PATH_OPER_INPUT,
                                                 LY_PATH_TARGET_SINGLE, LY_PREF_JSON, NULL, &path));
    term = lyd_target(path, tree);

    LYSC_NODE_CHECK(term->schema, LYS_LEAFLIST, "d");
    assert_string_equal(LYD_CANON_VALUE(term), "b");
    assert_string_equal(LYD_CANON_VALUE(term->prev), "b");

    LYD_NODE_DESTROY(tree);
    ly_path_free(CONTEXT_GET, path);
    lyxp_expr_free(CONTEXT_GET, exp);

    CONTEXT_DESTROY;
}

static void
test_list_pos(void **state)
{
    (void) state;

    const char *data;
    struct lyd_node *tree;

    CONTEXT_CREATE;

    data = "<bar xmlns=\"urn:tests:a\">test</bar>"
        "<l1 xmlns=\"urn:tests:a\"><a>one</a><b>one</b></l1>"
        "<l1 xmlns=\"urn:tests:a\"><a>two</a><b>two</b></l1>"
        "<foo xmlns=\"urn:tests:a\">test</foo>";
    assert_int_equal(LY_SUCCESS, lyd_parse_data_mem(ctx, data, LYD_XML, 0, LYD_VALIDATE_PRESENT, &tree));
    assert_int_equal(0, lyd_list_pos(tree));
    assert_int_equal(1, lyd_list_pos(tree->next));
    assert_int_equal(2, lyd_list_pos(tree->next->next));
    assert_int_equal(0, lyd_list_pos(tree->next->next->next));
    lyd_free_all(tree);

    data = "<ll xmlns=\"urn:tests:a\">one</ll>"
        "<ll xmlns=\"urn:tests:a\">two</ll>"
        "<ll xmlns=\"urn:tests:a\">three</ll>";
    assert_int_equal(LY_SUCCESS, lyd_parse_data_mem(ctx, data, LYD_XML, 0, LYD_VALIDATE_PRESENT, &tree));
    assert_int_equal(1, lyd_list_pos(tree));
    assert_int_equal(2, lyd_list_pos(tree->next));
    assert_int_equal(3, lyd_list_pos(tree->next->next));
    lyd_free_all(tree);

    data = "<ll xmlns=\"urn:tests:a\">one</ll>"
        "<l1 xmlns=\"urn:tests:a\"><a>one</a><b>one</b></l1>"
        "<ll xmlns=\"urn:tests:a\">two</ll>"
        "<l1 xmlns=\"urn:tests:a\"><a>two</a><b>two</b></l1>"
        "<ll xmlns=\"urn:tests:a\">three</ll>"
        "<l1 xmlns=\"urn:tests:a\"><a>three</a><b>three</b></l1>";
    assert_int_equal(LY_SUCCESS, lyd_parse_data_mem(ctx, data, LYD_XML, 0, LYD_VALIDATE_PRESENT, &tree));
    assert_string_equal("l1", tree->schema->name);
    assert_int_equal(1, lyd_list_pos(tree));
    assert_int_equal(2, lyd_list_pos(tree->next));
    assert_int_equal(3, lyd_list_pos(tree->next->next));
    assert_string_equal("ll", tree->next->next->next->schema->name);
    assert_int_equal(1, lyd_list_pos(tree->next->next->next));
    assert_int_equal(2, lyd_list_pos(tree->next->next->next->next));
    assert_int_equal(3, lyd_list_pos(tree->next->next->next->next->next));
    lyd_free_all(tree);
}

static void
test_first_sibling(void **state)
{
    (void) state;

    const char *data;
    struct lyd_node *tree;
    struct lyd_node_inner *parent;

    CONTEXT_CREATE;

    data = "<bar xmlns=\"urn:tests:a\">test</bar>"
        "<l1 xmlns=\"urn:tests:a\"><a>one</a><b>one</b><c>one</c></l1>"
        "<foo xmlns=\"urn:tests:a\">test</foo>";
    assert_int_equal(LY_SUCCESS, lyd_parse_data_mem(ctx, data, LYD_XML, 0, LYD_VALIDATE_PRESENT, &tree));
    assert_ptr_equal(tree, lyd_first_sibling(tree->next));
    assert_ptr_equal(tree, lyd_first_sibling(tree));
    assert_ptr_equal(tree, lyd_first_sibling(tree->prev));
    parent = (struct lyd_node_inner*)tree->next;
    assert_int_equal(LYS_LIST, parent->schema->nodetype);
    assert_ptr_equal(parent->child, lyd_first_sibling(parent->child->next));
    assert_ptr_equal(parent->child, lyd_first_sibling(parent->child));
    assert_ptr_equal(parent->child, lyd_first_sibling(parent->child->prev));
    lyd_free_all(tree);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_compare),
        cmocka_unit_test(test_dup),
        cmocka_unit_test(test_target),
        cmocka_unit_test_setup_teardown(test_list_pos),
        cmocka_unit_test_setup_teardown(test_first_sibling),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
