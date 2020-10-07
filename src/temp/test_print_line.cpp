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
    trp_print(p, 1, "key");
}

int main()
{

UNIT_TESTING_START;

using out_t = std::string;
out_t out;

TEST(line, fully)
{
    out_t check = "  |  |  +--rw prefix:node!   -> target {iffeature}?";
    trt_printing p = {&out, Out::print_string};
    trt_node node =
    {
        trd_status_current, trd_flags_rw,
        {trd_node_container, "prefix", "node"},
        trp_empty_opts_keys(),
        {trd_type_target, trp_init_breakable_str("target")},
        trp_set_iffeature()
    };
    trt_pck_print ppck = {NULL, {p_iff, p_key}};
    trt_indent_in_node ind = trp_default_indent_in_node(node);
    trt_wrapper wr = trp_init_wrapper_top();
    wr = trp_wrapper_set_mark(wr);
    wr = trp_wrapper_set_shift(wr);
    wr = trp_wrapper_set_mark(wr);
    trt_pck_indent ipck = {wr, ind};

    trp_print_line(node, ppck, ipck, p);

    EXPECT_EQ(out, check);
    out.clear();
}

TEST(line, firstNode)
{
    out_t check = "  +--rw prefix:node!   -> target {iffeature}?";
    trt_printing p = {&out, Out::print_string};
    trt_node node =
    {
        trd_status_current, trd_flags_rw,
        {trd_node_container, "prefix", "node"},
        trp_empty_opts_keys(),
        {trd_type_target, trp_init_breakable_str("target")},
        trp_set_iffeature()
    };
    trt_pck_print ppck = {NULL, {p_iff, p_key}};
    trt_indent_in_node ind = trp_default_indent_in_node(node);
    trt_wrapper wr = trp_init_wrapper_top();
    trt_pck_indent ipck = {wr, ind};

    trp_print_line(node, ppck, ipck, p);

    EXPECT_EQ(out, check);
    out.clear();
}

PRINT_TESTS_STATS();

UNIT_TESTING_END;

}
