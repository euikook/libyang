#include "new.h"


/* ----------- <Definition of printer functions> ----------- */

trt_node_name trp_empty_node_name()
{
    trt_node_name ret;
    ret.str = NULL;
    return ret;
}

bool trp_node_name_is_empty(trt_node_name node_name)
{
    return node_name.str == NULL;
}

trt_opts trp_empty_opts()
{
    trt_opts ret;
    ret.id = trd_opts_type_empty;
    return ret;
}

bool trp_opts_is_empty(trt_opts opts)
{
    return opts.id == trd_opts_type_empty;
}

trt_type trp_empty_type()
{
    trt_type ret;
    ret.id = trd_type_type_empty;
    return ret;
}

bool trp_type_is_empty(trt_type type)
{
    return type.id == trd_type_type_empty;
}

trt_iffeature trp_empty_iffeature()
{
    return false;
}

bool trp_iffeature_is_empty(trt_iffeature iffeature)
{
    return iffeature;
}

trt_node trp_empty_node()
{
    trt_node ret = 
    {
        NULL, NULL,
        trp_empty_node_name(), trp_empty_opts(),
        trp_empty_type(), trp_empty_iffeature()
    };
    return ret;
}

bool trp_node_is_empty(trt_node node)
{
    return trp_node_name_is_empty(node.name);
}

/* ----------- <Definition of tree functions> ----------- */
