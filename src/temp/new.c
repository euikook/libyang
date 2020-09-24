#include "new.h"
#include <string.h> // strlen


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
    ret.type = trd_opts_type_empty;
    return ret;
}

bool trp_opts_is_empty(trt_opts opts)
{
    return opts.type == trd_opts_type_empty;
}

trt_type trp_empty_type()
{
    trt_type ret;
    ret.type = trd_type_type_empty;
    return ret;
}

bool trp_type_is_empty(trt_type type)
{
    return type.type == trd_type_type_empty;
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

void trp_print_node_name(trt_node_name a, trt_printing p)
{
    if(trp_node_name_is_empty(a))
        return;

    switch(a.type) {
    case trd_node_type_else:
        p.pf(p.out, 2, a.module_prefix, a.str);
        break;
    case trd_node_type_choice:
        p.pf(p.out, 4, a.module_prefix, trd_node_name_prefix_choice, a.str, trd_node_name_suffix_choice);
        break;
    case trd_node_type_case:
        p.pf(p.out, 4, a.module_prefix, trd_node_name_prefix_case, a.str, trd_node_name_suffix_case);
        break;
    default:
        break;
    }
}

void trp_print_opts(trt_opts a, trt_cf_print_keys cf, trt_printing p)
{
    if(trp_opts_is_empty(a))
        return;

    switch(a.type) {
    case trd_opts_type_mark:
        p.pf(p.out, 1, a.mark);
        break;
    case trd_opts_type_keys:
        p.pf(p.out, 1, trd_opts_keys_prefix);
        cf.pf(cf.ctx);
        p.pf(p.out, 1, trd_opts_keys_suffix);
        break;
    default:
        break;
    }
}

void trp_print_type(trt_type a, trt_printing p)
{
    if(trp_type_is_empty(a))
        return;

    switch(a.type) {
    case trd_type_type_target:
        p.pf(p.out, 2, trd_type_target_prefix, a.target);
        break;
    case trd_type_type_leafref:
        p.pf(p.out, 1, trd_type_leafref);
    default:
        break;
    }
}

void trp_print_iffeatures(trt_iffeature a, trt_cf_print_iffeatures cf, trt_printing p)
{
    if(trp_iffeature_is_empty(a))
        return;

    p.pf(p.out, 1, trd_iffeatures_prefix);
    cf.pf(cf.ctx);
    p.pf(p.out, 1, trd_iffeatures_suffix);
}

void trp_print_node(trt_node a, const struct trt_tree_ctx* ctx, struct trt_fp_print fps, trt_indent_in_node ind, trt_printing p)
{
    if(trp_node_is_empty(a))
        return;

    /* <status>--<flags> <name><opts> <type> <if-features> */

    const bool divided = ind.type == trd_indent_in_node_divided;
    const char char_space = trd_separator_space[0];

    if(!divided) {
        /* <status>--<flags> */
        p.pf(p.out, 3, a.status, trd_separator_dashes, a.flags);

        /* If the node is a case node, there is no space before the <name> */
        if(a.name.type != trd_node_type_case)
            p.pf(p.out, 1, trd_separator_space);

        /* <name> */
        trp_print_node_name(a.name, p);

        /* <name>___<opts>*/
        trp_print_n_times(ind.btw_name_opts, char_space, p);
    } else {
        /* skip these statements: */

        /* <status>--<flags> */
        const uint32_t status_len = 1;
        uint32_t space = status_len + strlen(trd_separator_dashes) + strlen(a.flags);

        if(a.name.type != trd_node_type_case) {
            /* :(<name> */
            space += strlen(trd_node_name_prefix_case);
        } else if(a.name.type != trd_node_type_choice) {
            /* (<name> */
            space += strlen(trd_node_name_prefix_choice);
        } else {
            /* _<name> */
            space += strlen(trd_separator_space);
        }

        /* <name> 
         * __
         */
        space += trd_indent_long_line_break;

        trp_print_n_times(space, char_space, p);
    }

    /* <opts> */
    trt_cf_print_keys cf_print_keys = {ctx, fps.print_keys};
    trp_print_opts(a.opts, cf_print_keys, p);

    /* <opts>__<type> */
    trp_print_n_times(ind.btw_opts_type, char_space, p);

    /* <type> */
    trp_print_type(a.type, p);

    /* <type>__<iffeatures> */
    trp_print_n_times(ind.btw_type_iffeatures, char_space, p);

    /* <iffeatures> */
    trt_cf_print_keys cf_print_iffeatures = {ctx, fps.print_features_names};
    trp_print_iffeatures(a.iffeatures, cf_print_iffeatures, p);
}

uint32_t trp_strlen_node_name(trt_node_name a)
{
    if(trp_node_name_is_empty(a))
        return 0;

    int prefSuff = 0;
    switch(a.type) {
    case trd_node_type_else:
        prefSuff = 0;
        break;
    case trd_node_type_choice:
        prefSuff = strlen(trd_node_name_prefix_choice) + strlen(trd_node_name_suffix_choice);
        break;
    case trd_node_type_case:
        prefSuff = strlen(trd_node_name_prefix_case) + strlen(trd_node_name_suffix_case);
        break;
    default:
        break;
    }
    return strlen(a.module_prefix) + strlen(a.str) + prefSuff;
}


uint32_t trp_strlen_opts(trt_opts a, trt_cf_strlen_keys cf)
{
    if(trp_opts_is_empty(a))
        return 0;

    uint32_t ret = 0;
    switch(a.type) {
    case trd_opts_type_empty:
        ret = 0;
        break;
    case trd_opts_type_mark:
        ret = strlen(a.mark);
        break;
    case trd_opts_type_keys:
        ret = strlen(trd_opts_keys_prefix) + strlen(trd_opts_keys_suffix) + cf.pf(cf.ctx);
        break;
    default:
        break;
    }

    return ret;
}

uint32_t trp_strlen_type(trt_type a)
{
    if(trp_type_is_empty(a))
        return 0;

    uint32_t ret = 0;
    switch(a.type) {
    case trd_type_type_target:
        ret = strlen(trd_type_target_prefix) + strlen(a.target);
        break;
    case trd_type_type_leafref:
        ret = strlen(trd_type_leafref);
        break;
    default:
        break;
    }

    return ret;
}


uint32_t trp_strlen_iffeatures(trt_iffeature a, trt_cf_strlen_iffeatures cf)
{
    if(trp_iffeature_is_empty(a))
        return 0;

    uint32_t ret = strlen(trd_iffeatures_prefix) + cf.pf(cf.ctx) + strlen(trd_iffeatures_suffix);
    return ret;
}


/* ----------- <Definition of tree functions> ----------- */

/* ----------- <Definition of the other functions> ----------- */

#define PRINT_N_TIMES_BUFFER_SIZE 16

void trp_print_n_times(uint32_t n, char c, trt_printing p)
{
    if(n == 0)
        return;
    
    static char buffer[PRINT_N_TIMES_BUFFER_SIZE];
    const uint32_t buffer_size = PRINT_N_TIMES_BUFFER_SIZE;
    buffer[buffer_size-1] = '\0';
    for(uint32_t i = 0; i < n / (buffer_size-1); i++) {
        memset(&buffer[0], c, buffer_size-1);
        p.pf(p.out, 1, &buffer[0]);
    }
    uint32_t rest = n % (buffer_size-1);
    buffer[rest] = '\0';
    memset(&buffer[0], c, rest);
    p.pf(p.out, 1, &buffer[0]);
}

