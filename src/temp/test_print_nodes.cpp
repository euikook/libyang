#include <iostream>
#include <vector>
#include <string>
#include "print_func.hpp"
#include "node_rel.hpp"
#include "unit_testing.hpp"

extern "C"
{
#include "new.h"
}

/* Comment trt_tree_ctx struct definition in new.h */

using Node = Node;
using Tree = Tree;
using tree_iter = tree_iter;
using out_t = Out::VecLines;
using std::string;
out_t out;

int main()
{

UNIT_TESTING_START;

TEST(printNodes, oneNode)
{
    Tree tree
    ({
        Node{"A", {}}
    });
    out_t check = {"  +--rw A"};
    trt_tree_ctx ctx = {tree, tree.map.begin(), -1};
    struct trt_printer_ctx pc = 
    {
        0,
        (trt_printing){&out, Out::print_vecLines},
        { /* trt_fp_all */
            {parent, next_sibling, next_child, NULL, NULL, NULL},
            {NULL, node, next_sibling_read},
            {NULL, NULL}
        },
        72
    };
    trb_print_subtree_nodes(trp_init_wrapper_top(), pc, &ctx);
    EXPECT_EQ(out, check);
    out.clear();
}

TEST(printNodes, twoSiblings)
{
    Tree tree
    ({
        {"A", {"B", "C"}},
        {"B", {}}, {"C", {}}
    });
    string check1 = "  +--rw A";
    string check2 = "     +--rw B";
    string check3 = "     +--rw C";
    out_t check = {check1, check2, check3};
    trt_tree_ctx ctx = {tree, tree.map.begin(), -1};
    struct trt_printer_ctx pc = 
    {
        0,
        (trt_printing){&out, Out::print_vecLines},
        { /* trt_fp_all */
            {parent, next_sibling, next_child, NULL, NULL, NULL},
            {NULL, node, next_sibling_read},
            {NULL, NULL}
        },
        72
    };
    trb_print_subtree_nodes(trp_init_wrapper_top(), pc, &ctx);
    EXPECT_EQ(out, check);
    out.clear();
}

TEST(printNodes, twoSiblingsOneChild)
{
    Tree tree
    ({
        {"A", {"B", "C"}},
        {"B", {"D"}}, {"C", {}},
        {"D", {}}
    });
    string check1 = "  +--rw A";
    string check2 = "     +--rw B";
    string check3 = "     |  +--rw D";
    string check4 = "     +--rw C";
    out_t check = {check1, check2, check3, check4};
    trt_tree_ctx ctx = {tree, tree.map.begin(), -1};
    struct trt_printer_ctx pc = 
    {
        0,
        (trt_printing){&out, Out::print_vecLines},
        { /* trt_fp_all */
            {parent, next_sibling, next_child, NULL, NULL, NULL},
            {NULL, node, next_sibling_read},
            {NULL, NULL}
        },
        72
    };
    trb_print_subtree_nodes(trp_init_wrapper_top(), pc, &ctx);
    EXPECT_EQ(out, check);
    out.clear();
}

PRINT_TESTS_STATS();

UNIT_TESTING_END;

}
