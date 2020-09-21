#include <iostream>
#include <vector>
#include <string>
#include "node_rel.hpp"
#include "unit_testing.hpp"

extern "C"
{
#include "new.h"
}

using Node = Node_rel::Node;
using Tree = Node_rel::Tree;
using tree_iter = Node_rel::tree_iter;
using std::string;

int main()
{

UNIT_TESTING_START;

TEST(nodeRel, firstNode)
{
    Tree tree
    ({
        {"A", {"B", "C"}},
        {"B", {}}, {"C", {}}
    });
    Node_rel::trt_tree_ctx ctx = {tree, tree.map.begin(), -1};
    trt_node uut;
    using namespace Node_rel;

    uut = node(&ctx);
    ASSERT_STREQ(uut.name.str, "A");
}

TEST(nodeRel, firstGetFirstChild)
{
    Tree tree
    ({
        {"A", {"B", "C"}},
        {"B", {}}, {"C", {}}
    });
    Node_rel::trt_tree_ctx ctx = {tree, tree.map.begin(), -1};
    trt_node uut;
    using namespace Node_rel;

    uut = next_child(&ctx);
    ASSERT_STREQ(uut.name.str, "B");
    uut = node(&ctx);
    ASSERT_STREQ(uut.name.str, "B");
}

TEST(nodeRel, firstNoSiblings)
{
    Tree tree
    ({
        {"A", {"B", "C"}},
        {"B", {}}, {"C", {}}
    });
    Node_rel::trt_tree_ctx ctx = {tree, tree.map.begin(), -1};
    trt_node uut;
    using namespace Node_rel;

    uut = next_sibling(&ctx);
    EXPECT_STREQ(uut.name.str, nullptr);
    uut = node(&ctx);
    ASSERT_STREQ(uut.name.str, "A");
}

TEST(nodeRel, childThenSibling)
{
    Tree tree
    ({
        {"A", {"B", "C"}},
        {"B", {}}, {"C", {}}
    });
    Node_rel::trt_tree_ctx ctx = {tree, tree.map.begin(), -1};
    trt_node uut;
    using namespace Node_rel;

    uut = next_child(&ctx);
    ASSERT_STREQ(uut.name.str, "B");
    uut = next_sibling(&ctx);
    ASSERT_STREQ(uut.name.str, "C");
    uut = node(&ctx);
    ASSERT_STREQ(uut.name.str, "C");
}

TEST(nodeRel, endOfSiblings)
{
    Tree tree
    ({
        {"A", {"B", "C"}},
        {"B", {}}, {"C", {}}
    });
    Node_rel::trt_tree_ctx ctx = {tree, tree.map.begin(), -1};
    trt_node uut;
    using namespace Node_rel;

    uut = next_child(&ctx);
    ASSERT_STREQ(uut.name.str, "B");
    uut = next_sibling(&ctx);
    ASSERT_STREQ(uut.name.str, "C");
    uut = next_sibling(&ctx);
    EXPECT_STREQ(uut.name.str, nullptr);
    uut = node(&ctx);
    ASSERT_STREQ(uut.name.str, "C");
}

TEST(nodeRel, noChilds)
{
    Tree tree
    ({
        {"A", {"B", "C"}},
        {"B", {}}, {"C", {}}
    });
    Node_rel::trt_tree_ctx ctx = {tree, tree.map.begin(), -1};
    trt_node uut;
    using namespace Node_rel;

    uut = next_child(&ctx);
    ASSERT_STREQ(uut.name.str, "B");
    uut = next_sibling(&ctx);
    ASSERT_STREQ(uut.name.str, "C");
    uut = next_child(&ctx);
    EXPECT_STREQ(uut.name.str, nullptr);
    uut = node(&ctx);
    ASSERT_STREQ(uut.name.str, "C");
}

PRINT_TESTS_STATS();

UNIT_TESTING_END;

}
