#ifndef NODE_REL_H_
#define NODE_REL_H_

#include <map>
#include <vector>
#include <string>
#include <algorithm>

extern "C"
{
#include "new.h"
}

namespace Node_rel
{

using std::vector;
using std::string;

struct Node
{
    using childs_v = vector<string>;

    string name;
    childs_v childs;

    Node(string p_name, vector<string> p_childs):
        name(p_name), childs(p_childs){}
};
using childs_iter = Node::childs_v::iterator;

struct Tree
{
    using tree_t = std::map<string, vector<string>>;
    tree_t map;

    Tree(vector<Node> nodes)
    {
        for(auto const& node: nodes)
            map.insert({node.name, node.childs});
    }
};
using tree_iter = Tree::tree_t::iterator;

struct trt_tree_ctx
{
    Tree tree;
    tree_iter row;
    int64_t child_idx;
};

trt_node node(struct trt_tree_ctx *ctx)
{
    /* if pointing to parent */
    if(ctx->child_idx < 0) {
        /* get parent name */ 
        auto& node_name = ctx->row->first;
        /* return node */
        trt_node ret = trp_empty_node();
        ret.name.str = node_name.c_str();
        return ret;
    } else {
        /* get child name */ 
        auto& node_name = ctx->row->second[ctx->child_idx];
        /* return node */
        trt_node ret = trp_empty_node();
        ret.name.str = node_name.c_str();
        return ret;
    }
}

trt_node next_sibling(struct trt_tree_ctx *ctx)
{
    auto& child_idx = ctx->child_idx;
    /* if pointing to parent name */ 
    if(child_idx < 0) {
        /* find if has siblings */
        childs_iter iter;
        bool succ = false;
        auto node_name = ctx->row->first;
        for(auto item = ctx->tree.map.begin(); item != ctx->tree.map.end(); ++item) {
            auto& childs = item->second;
            if((iter = std::find(childs.begin(), childs.end(), node_name)) != childs.end()) {
                /* he is already last sibling -> no sibling? */
                if(iter+1 == childs.end())
                    continue;
                succ = true;
                /* store location */
                ctx->row = item;
                break;
            }
        }

        if(succ) {
            /* siblings was founded */
            /* iterator to index */
            child_idx = std::distance(ctx->row->second.begin(), iter);
            /* continue to get sibling */
        } else {
            /* no siblings */
            return trp_empty_node();
        }
    }
    /* else we are already pointing to siblings */ 

    /* if there is no sibling */ 
    if(child_idx + 1 >= static_cast<int64_t>(ctx->row->second.size())) {
        return trp_empty_node();
    } else {
        /* return sibling */
        child_idx++;
        auto& sibl_name = ctx->row->second[child_idx];
        trt_node ret = trp_empty_node();
        ret.name.str = sibl_name.c_str();
        return ret;
    }
}

trt_node next_child(struct trt_tree_ctx* ctx)
{
    auto& child_idx = ctx->child_idx;
    /* if pointing to parent */ 
    if(child_idx < 0) {
        /* get his child */ 
        child_idx = 0;
        trt_node ret = trp_empty_node();
        ret.name.str = ctx->row->second[child_idx].c_str();
        return ret;
    }
    /* else find child of child */ 
    auto& node_name = ctx->row->second[child_idx];
    childs_iter iter;
    bool succ = false;
    /* find child as key in map */ 
    for(auto& item: ctx->tree.map) {
        auto& childs = item.second;
        if((iter = std::find(childs.begin(), childs.end(), node_name)) != childs.end()) {
            succ = true;
            break;
        }
    }

    /* actualize info -> pointing to parent */
    ctx->row = ctx->tree.map.find(*iter);
    child_idx = -1;

    if(succ) {
        /* try get child of child */ 
        /* if child has child */
        if(!ctx->row->second.empty()){
            trt_node ret = trp_empty_node();
            ret.name.str = (*iter).c_str();
            return ret;
        } else {
            /* child has no child */
            return trp_empty_node();
        }
    } else {
        /* child has no child */ 
        return trp_empty_node();
    }
}

}

#endif
