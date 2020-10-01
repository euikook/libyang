#include "new.h"
#include <string.h> // strlen


/* ----------- <Definition of printer functions> ----------- */

void trp_print(trt_printing p, int arg_count, ...)
{
    va_list ap;
    va_start(ap, arg_count);
    p.pf(p.out, arg_count, ap);
    va_end(ap);
}

void trp_injected_strlen(void *out, int arg_count, va_list ap)
{
    trt_counter* cnt = (trt_counter*)out;

    for(int i = 0; i < arg_count; i++)
        cnt->bytes += strlen(va_arg(ap, char*));
}

trt_wrapper trp_init_wrapper_top()
{
    /* module: <module-name>
     *   +--<node>
     *   |
     */
    trt_wrapper wr;
    wr.type = trd_wrapper_type_top;
    wr.actual_pos = 0;
    wr.bit_marks1 = 0;
    return wr;
}

trt_wrapper trp_init_wrapper_body()
{
    /* module: <module-name>
     *   +--<node>
     *
     *   augment <target-node>:
     *     +--<node>
     */
    trt_wrapper wr;
    wr.type = trd_wrapper_type_body;
    wr.actual_pos = 0;
    wr.bit_marks1 = 0;
    return wr;
}

trt_wrapper trp_wrapper_set_mark(trt_wrapper wr)
{
    wr.bit_marks1 |= 1U << wr.actual_pos;
    return wr;
}

trt_wrapper trp_wrapper_set_shift(trt_wrapper wr)
{
    /* +--<node>
     * |  +--<node>
     */
    wr.actual_pos++;
    return wr;
}

void trp_print_wrapper(trt_wrapper wr, trt_printing p)
{
    const char char_space = trd_separator_space[0];

    {
        uint32_t lb;
        if(wr.type == trd_wrapper_type_top) {
            lb = trd_indent_line_begin;
        } else if(wr.type == trd_wrapper_type_body) {
            lb = trd_indent_line_begin * 2;
        } else
            lb = 0;

        trg_print_n_times(lb, char_space, p);
    }

    for(uint32_t i = 0; i <= wr.actual_pos; i++) {
        if(trg_test_bit(wr.bit_marks1, i)){
            trp_print(p, 1, trd_symbol_sibling);
        } else {
            trp_print(p, 1, trd_separator_space);
        }

        if(i != wr.actual_pos)
            trg_print_n_times(trd_indent_btw_siblings, char_space, p);
    }
}

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
    return !iffeature;
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

trt_keyword_stmt trp_empty_keyword_stmt()
{
    trt_keyword_stmt ret;
    ret.name = NULL;
    return ret;
}

bool trp_keyword_stmt_is_empty(trt_keyword_stmt ks)
{
    return ks.name == NULL;
}

void trp_print_node_name(trt_node_name a, trt_printing p)
{
    if(trp_node_name_is_empty(a))
        return;

    const char* colon = a.module_prefix == NULL || a.module_prefix[0] == '\0' ? "" : trd_separator_colon;

    switch(a.type) {
    case trd_node_type_else:
        trp_print(p, 3, a.module_prefix, colon, a.str);
        break;
    case trd_node_type_choice:
        trp_print(p, 5, trd_node_name_prefix_choice,  a.module_prefix, colon, a.str, trd_node_name_suffix_choice);
        break;
    case trd_node_type_case:
        trp_print(p, 5, trd_node_name_prefix_case, a.module_prefix, colon, a.str, trd_node_name_suffix_case);
        break;
    default:
        break;
    }
}

void trp_print_opts(trt_opts a, trt_indent_btw ind, trt_cf_print_keys cf, trt_printing p)
{
    if(trp_opts_is_empty(a))
        return;

    switch(a.type) {
    case trd_opts_type_mark_only:
        trp_print(p, 1, a.mark);
        break;
    case trd_opts_type_keys:
        trp_print(p, 1, trd_opts_list);
        /* <name><mark>___<opts>*/
        trg_print_n_times(ind, trd_separator_space[0], p);
        trp_print(p, 1, trd_opts_keys_prefix);
        cf.pf(cf.ctx, p);
        trp_print(p, 1, trd_opts_keys_suffix);
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
        trp_print(p, 2, trd_type_target_prefix, a.target);
        break;
    case trd_type_type_leafref:
        trp_print(p, 1, trd_type_leafref);
    default:
        break;
    }
}

void trp_print_iffeatures(trt_iffeature a, trt_cf_print_iffeatures cf, trt_printing p)
{
    if(trp_iffeature_is_empty(a))
        return;

    trp_print(p, 1, trd_iffeatures_prefix);
    cf.pf(cf.ctx, p);
    trp_print(p, 1, trd_iffeatures_suffix);
}

void trp_print_node(trt_node a, trt_pck_print pck, trt_indent_in_node ind, trt_printing p)
{
    if(trp_node_is_empty(a))
        return;

    /* <status>--<flags> <name><opts> <type> <if-features> */

    const bool divided = ind.type == trd_indent_in_node_divided;
    const char char_space = trd_separator_space[0];

    if(!divided) {
        /* <status>--<flags> */
        trp_print(p, 3, a.status, trd_separator_dashes, a.flags);

        /* If the node is a case node, there is no space before the <name> */
        if(a.name.type != trd_node_type_case)
            trp_print(p, 1, trd_separator_space);

        /* <name> */
        trp_print_node_name(a.name, p);
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

        trg_print_n_times(space, char_space, p);
    }

    /* <opts> */
    /* <name>___<opts>*/
    trt_cf_print_keys cf_print_keys = {pck.tree_ctx, pck.fps.print_keys};
    trp_print_opts(a.opts, ind.btw_name_opts, cf_print_keys, p);

    /* <opts>__<type> */
    trg_print_n_times(ind.btw_opts_type, char_space, p);

    /* <type> */
    trp_print_type(a.type, p);

    /* <type>__<iffeatures> */
    trg_print_n_times(ind.btw_type_iffeatures, char_space, p);

    /* <iffeatures> */
    trt_cf_print_keys cf_print_iffeatures = {pck.tree_ctx, pck.fps.print_features_names};
    trp_print_iffeatures(a.iffeatures, cf_print_iffeatures, p);
}

void trp_print_keyword_stmt(trt_keyword_stmt a, trt_printing p)
{
    if(trp_keyword_stmt_is_empty(a))
        return;

    switch(a.type) {
    case trd_keyword_stmt_top:
        trp_print(p, 4, a.keyword, trd_separator_colon, trd_separator_space, a.name);
        break;
    case trd_keyword_stmt_body:
        trp_print(p, 4, a.keyword, trd_separator_space, a.name, trd_separator_colon);
        break;
    default:
        break;
    }
}

void trp_print_line(trt_node node, trt_pck_print pck, trt_pck_indent ind, trt_printing p)
{
    trp_print_wrapper(ind.wrapper, p);
    trg_print_n_times(trd_indent_btw_siblings, trd_separator_space[0], p); 
    trp_print_node(node, pck, ind.in_node, p);
}

trt_node trp_divide_node(trt_node node, trt_indent_in_node ind)
{
    if(ind.type != trd_indent_in_node_divided || ind.btw_name_opts < 0)
        return node;

    if(ind.btw_opts_type < 0)
        node.opts = trp_empty_opts();

    if(ind.btw_type_iffeatures < 0) {
        node.opts = trp_empty_opts();
        node.type = trp_empty_type();
    }
    return node;
}

/* ----------- <Definition of tree functions> ----------- */

/* ----------- <Definition of the other functions> ----------- */

#define PRINT_N_TIMES_BUFFER_SIZE 16

void trg_print_n_times(int32_t n, char c, trt_printing p)
{
    if(n <= 0)
        return;
    
    static char buffer[PRINT_N_TIMES_BUFFER_SIZE];
    const uint32_t buffer_size = PRINT_N_TIMES_BUFFER_SIZE;
    buffer[buffer_size-1] = '\0';
    for(uint32_t i = 0; i < n / (buffer_size-1); i++) {
        memset(&buffer[0], c, buffer_size-1);
        trp_print(p, 1, &buffer[0]);
    }
    uint32_t rest = n % (buffer_size-1);
    buffer[rest] = '\0';
    memset(&buffer[0], c, rest);
    trp_print(p, 1, &buffer[0]);
}

bool trg_test_bit(uint64_t number, uint32_t bit)
{
    return (number >> bit) & 1U;
}

