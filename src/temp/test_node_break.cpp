#include <iostream>
#include "print_func.hpp"
#include "unit_testing.hpp"

extern "C"
{
#include "new.h"
}

void p_iff(const struct trt_tree_ctx*, trt_printing p)
{
    trp_print(p, 1, "iffeature");
}

void p_key(const struct trt_tree_ctx*, trt_printing p)
{
    trp_print(p, 1, "key1 key2");
}

int main()
{

UNIT_TESTING_START;

using out_t = Out::VecLines;
using std::string;
out_t out;

TEST(nodeBreak, fits)
{
    out_t check = {"  +--rw prefix:node* [key1 key2]    type {iffeature}?"};
    trt_printing p = {&out, Out::print_vecLines};
    trt_node node =
    {
        trd_status_current, trd_flags_rw,
        {trd_node_keys, "prefix", "node"},
        trp_set_opts_keys(),
        {trd_type_name, trp_init_breakable_str("type")},
        trp_set_iffeature()
    };
    trt_pck_print ppck = {NULL, {p_iff, p_key}};
    trt_indent_in_node ind = trp_default_indent_in_node(node);
    trt_wrapper wr = trp_init_wrapper_top();
    trt_pck_indent ipck = {wr, ind};
    trp_print_entire_node(node, ppck, ipck, 72, p);

    EXPECT_EQ(out, check);
    out.clear();
}


TEST(nodeBreak, btwNameOpts)
{
    out_t base   =       {"  +--rw xxxprefix:node* [key1 key2]"};
    uint32_t mll = strlen("                       ^");
    string check1 =       "  +--rw xxxprefix:node*";
    string check2 =       "  |       [key1 key2]";
    out_t check = {check1, check2};

    trt_printing p = {&out, Out::print_vecLines};
    trt_node node =
    {
        trd_status_current, trd_flags_rw,
        {trd_node_keys, "xxxprefix", "node"},
        trp_set_opts_keys(),
        {trd_type_empty, trp_empty_breakable_str()},
        trp_empty_iffeature()
    };
    trt_pck_print ppck = {NULL, {p_iff, p_key}};
    trt_indent_in_node ind = trp_default_indent_in_node(node);
    trt_wrapper wr = trp_init_wrapper_top();
    trt_pck_indent ipck = {wr, ind};
    trp_print_entire_node(node, ppck, ipck, mll, p);

    EXPECT_EQ(out, check);

    out.clear();
}

PRINT_TESTS_STATS();

UNIT_TESTING_END;

}
