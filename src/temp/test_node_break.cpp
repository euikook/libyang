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

#if 0

TEST(nodeBreak, fits)
{
    out_t check = {"  +--rw prefix:node* [key1 key2]    type {iffeature}?"};
    trt_node node =
    {
        trd_status_current, trd_flags_rw,
        {trd_node_keys, "prefix", "node"},
        trp_set_opts_keys(),
        {trd_type_name, trp_set_breakable_str("type")},
        trp_set_iffeature()
    };
    trp_print_entire_node(node, (trt_pck_print){NULL, {p_iff, p_key}},
        (trt_pck_indent){trp_init_wrapper_top(), trp_default_indent_in_node(node)},
        72, (trt_printing){&out, Out::print_vecLines});

    EXPECT_EQ(out, check);
    out.clear();
}

TEST(nodeBreak, fitsTight)
{
    out_t check =        {"  +--rw prefix:node* [key1 key2]    type {iffeature}?"};
    uint32_t mll = strlen("                                                    ^");
    trt_node node =
    {
        trd_status_current, trd_flags_rw,
        {trd_node_keys, "prefix", "node"},
        trp_set_opts_keys(),
        {trd_type_name, trp_set_breakable_str("type")},
        trp_set_iffeature()
    };
    trp_print_entire_node(node, (trt_pck_print){NULL, {p_iff, p_key}},
        (trt_pck_indent){trp_init_wrapper_top(), trp_default_indent_in_node(node)},
        mll, (trt_printing){&out, Out::print_vecLines});

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
    trt_node node =
    {
        trd_status_current, trd_flags_rw,
        {trd_node_keys, "xxxprefix", "node"},
        trp_set_opts_keys(),
        {trd_type_empty, trp_empty_breakable_str()},
        trp_empty_iffeature()
    };
    trp_print_entire_node(node, (trt_pck_print){NULL, {p_iff, p_key}},
        (trt_pck_indent){trp_init_wrapper_top(), trp_default_indent_in_node(node)},
        mll, (trt_printing){&out, Out::print_vecLines});

    EXPECT_EQ(out, check);
    out.clear();
}

TEST(nodeBreak, btwOptsType)
{
    out_t base   =       {"  +--rw xxxprefix:node*    string"};
    uint32_t mll = strlen("                       ^");
    string check1 =       "  +--rw xxxprefix:node*";
    string check2 =       "  |       string";
    out_t check = {check1, check2};
    trt_node node =
    {
        trd_status_current, trd_flags_rw,
        {trd_node_listLeaflist, "xxxprefix", "node"},
        trp_empty_opts_keys(),
        {trd_type_name, trp_set_breakable_str("string")},
        trp_empty_iffeature()
    };
    trp_print_entire_node(node, (trt_pck_print){NULL, {p_iff, p_key}},
        (trt_pck_indent){trp_init_wrapper_top(), trp_default_indent_in_node(node)},
        mll, (trt_printing){&out, Out::print_vecLines});

    EXPECT_EQ(out, check);
    out.clear();
}

TEST(nodeBreak, btwOptsTypeWithIffeatures)
{
    out_t base   =       {"  +--rw xxxprefix:node*   st {iffeature}?"};
    uint32_t mll = strlen("                         ^");
    string check1 =       "  +--rw xxxprefix:node*";
    string check2 =       "  |       st {iffeature}?";
    out_t check = {check1, check2};
    trt_node node =
    {
        trd_status_current, trd_flags_rw,
        {trd_node_keys, "xxxprefix", "node"},
        trp_empty_opts_keys(),
        {trd_type_name, trp_set_breakable_str("st")},
        trp_set_iffeature()
    };
    trp_print_entire_node(node, (trt_pck_print){NULL, {p_iff, p_key}},
        (trt_pck_indent){trp_init_wrapper_top(), trp_default_indent_in_node(node)},
        mll, (trt_printing){&out, Out::print_vecLines});


    EXPECT_EQ(out, check);
    out.clear();
}

TEST(nodeBreak, btwTypeIffeatures)
{
    out_t base   =       {"  +--rw xxxprefix:node* {iffeature}?"};
    uint32_t mll = strlen("                       ^");
    string check1 =       "  +--rw xxxprefix:node*";
    string check2 =       "  |       {iffeature}?";
    out_t check = {check1, check2};
    trt_node node =
    {
        trd_status_current, trd_flags_rw,
        {trd_node_listLeaflist, "xxxprefix", "node"},
        trp_empty_opts_keys(),
        {trd_type_empty, trp_empty_breakable_str()},
        trp_set_iffeature()
    };
    trp_print_entire_node(node, (trt_pck_print){NULL, {p_iff, p_key}},
        (trt_pck_indent){trp_init_wrapper_top(), trp_default_indent_in_node(node)},
        mll, (trt_printing){&out, Out::print_vecLines});

    EXPECT_EQ(out, check);
    out.clear();
}

TEST(nodeBreak, btwTypeIffeaturesWithKeys)
{
    out_t base   =       {"  +--rw xxxprefix:node* [key1 key2] {iffeature}?"};
    uint32_t mll = strlen("                                           ^");
    string check1 =       "  +--rw xxxprefix:node* [key1 key2]";
    string check2 =       "  |       {iffeature}?";
    out_t check = {check1, check2};
    trt_node node =
    {
        trd_status_current, trd_flags_rw,
        {trd_node_keys, "xxxprefix", "node"},
        trp_set_opts_keys(),
        {trd_type_empty, trp_empty_breakable_str()},
        trp_set_iffeature()
    };
    trp_print_entire_node(node, (trt_pck_print){NULL, {p_iff, p_key}},
        (trt_pck_indent){trp_init_wrapper_top(), trp_default_indent_in_node(node)},
        mll, (trt_printing){&out, Out::print_vecLines});

    EXPECT_EQ(out, check);
    out.clear();
}

TEST(nodeBreak, btwTypeIffeaturesWithKeysType)
{
    out_t base   =       {"  +--rw xxxprefix:node* [key1 key2]    string {iffeature}?"};
    uint32_t mll = strlen("                                             ^");
    string check1 =       "  +--rw xxxprefix:node* [key1 key2]    string";
    string check2 =       "  |       {iffeature}?";
    out_t check = {check1, check2};
    trt_node node =
    {
        trd_status_current, trd_flags_rw,
        {trd_node_keys, "xxxprefix", "node"},
        trp_set_opts_keys(),
        {trd_type_name, trp_set_breakable_str("string")},
        trp_set_iffeature()
    };
    trp_print_entire_node(node, (trt_pck_print){NULL, {p_iff, p_key}},
        (trt_pck_indent){trp_init_wrapper_top(), trp_default_indent_in_node(node)},
        mll, (trt_printing){&out, Out::print_vecLines});

    EXPECT_EQ(out, check);
    out.clear();
}

#endif

TEST(nodeBreak, allInNewLines)
{
    out_t base   =       {"  +--rw xxxprefix:node* [key1 key2]    string {iffeature}?"};
    uint32_t mll = strlen("                     ^");
    string check1 =       "  +--rw xxxprefix:node*";
    string check2 =       "  |       [key1 key2]";
    string check3 =       "  |       string";
    string check4 =       "  |       {iffeature}?";
    out_t check = {check1, check2, check3, check4};
    trt_node node =
    {
        trd_status_current, trd_flags_rw,
        {trd_node_keys, "xxxprefix", "node"},
        trp_set_opts_keys(),
        {trd_type_name, trp_set_breakable_str("string")},
        trp_set_iffeature()
    };
    trp_print_entire_node(node, (trt_pck_print){NULL, {p_iff, p_key}},
        (trt_pck_indent){trp_init_wrapper_top(), trp_default_indent_in_node(node)},
        mll, (trt_printing){&out, Out::print_vecLines});

    EXPECT_EQ(out, check);
    out.clear();
}

PRINT_TESTS_STATS();

UNIT_TESTING_END;

}
