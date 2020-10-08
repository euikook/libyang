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

TEST(node, fully)
{
    out_t check = "+--rw prefix:node!   -> target {iffeature}?";
    trt_node node =
    {
        trd_status_current, trd_flags_rw,
        {trd_node_container, "prefix", "node"},
        trp_empty_opts_keys(),
        {trd_type_target, trp_set_breakable_str("target")},
        trp_set_iffeature()
    };
    trp_print_node(node, (trt_pck_print){NULL, {p_iff, p_key}},
        trp_default_indent_in_node(node), (trt_printing){&out, Out::print_string});

    EXPECT_EQ(out, check);
    out.clear();
}

TEST(node, onlyIffeature)
{
    out_t check = "+--rw node {iffeature}?";
    trt_node node =
    {
        trd_status_current, trd_flags_rw,
        {trd_node_else, "", "node"},
        trp_empty_opts_keys(),
        {trd_type_empty, {}},
        trp_set_iffeature()
    };
    trp_print_node(node, (trt_pck_print){NULL, {p_iff, p_key}},
        trp_default_indent_in_node(node), (trt_printing){&out, Out::print_string});

    EXPECT_EQ(out, check);
    out.clear();
}

PRINT_TESTS_STATS();

UNIT_TESTING_END;

}
