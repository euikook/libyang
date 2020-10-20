
#include "new.h"
#include <string.h> // strlen

/* -######-- Definitions start -#######- */

/* ----------- <Definition of printer functions> ----------- */

void
trp_print(trt_printing p, int arg_count, ...)
{
    va_list ap;
    va_start(ap, arg_count);
    p.pf(p.out, arg_count, ap);
    va_end(ap);
}

void
trp_injected_strlen(void *out, int arg_count, va_list ap)
{
    trt_counter* cnt = (trt_counter*)out;

    for(int i = 0; i < arg_count; i++)
        cnt->bytes += strlen(va_arg(ap, char*));
}

trt_indent_in_node trp_empty_indent_in_node()
{
    return (trt_indent_in_node){trd_indent_in_node_normal, 0, 0, 0};
}

ly_bool
trp_indent_in_node_are_eq(trt_indent_in_node f, trt_indent_in_node s)
{
    const ly_bool a = f.type == s.type;
    const ly_bool b = f.btw_name_opts == s.btw_name_opts;
    const ly_bool c = f.btw_opts_type == s.btw_opts_type;
    const ly_bool d = f.btw_type_iffeatures == s.btw_type_iffeatures;
    return a && b && c && d;
}

trt_wrapper
trp_init_wrapper_top()
{
    /* module: <module-name>
     *   +--<node>
     *   |
     */
    trt_wrapper wr;
    wr.type = trd_wrapper_top;
    wr.actual_pos = 0;
    wr.bit_marks1 = 0;
    return wr;
}

trt_wrapper
trp_init_wrapper_body()
{
    /* module: <module-name>
     *   +--<node>
     *
     *   augment <target-node>:
     *     +--<node>
     */
    trt_wrapper wr;
    wr.type = trd_wrapper_body;
    wr.actual_pos = 0;
    wr.bit_marks1 = 0;
    return wr;
}

trt_wrapper
trp_wrapper_set_mark(trt_wrapper wr)
{
    wr.bit_marks1 |= 1U << wr.actual_pos;
    return trp_wrapper_set_shift(wr);
}

trt_wrapper
trp_wrapper_set_shift(trt_wrapper wr)
{
    /* +--<node>
     *    +--<node>
     */
    wr.actual_pos++;
    return wr;
}

ly_bool
trp_wrapper_eq(trt_wrapper f, trt_wrapper s)
{
    const ly_bool a = f.type == s.type;
    const ly_bool b = f.bit_marks1 == s.bit_marks1;
    const ly_bool c = f.actual_pos == s.actual_pos;
    return a && b && c;
}

void
trp_print_wrapper(trt_wrapper wr, trt_printing p)
{
    const char char_space = trd_separator_space[0];

    {
        uint32_t lb;
        if (wr.type == trd_wrapper_top) {
          lb = trd_indent_line_begin;
        } else if (wr.type == trd_wrapper_body) {
          lb = trd_indent_line_begin * 2;
        } else
          lb = trd_indent_line_begin;

        trg_print_n_times(lb, char_space, p);
    }

    if(trp_wrapper_eq(wr, trp_init_wrapper_top()))
        return;

    for(uint32_t i = 0; i < wr.actual_pos; i++) {
        if(trg_test_bit(wr.bit_marks1, i)){
            trp_print(p, 1, trd_symbol_sibling);
        } else {
            trp_print(p, 1, trd_separator_space);
        }

        if(i != wr.actual_pos)
            trg_print_n_times(trd_indent_btw_siblings, char_space, p);
    }
}

trt_node_name
trp_empty_node_name()
{
    trt_node_name ret;
    ret.str = NULL;
    return ret;
}

ly_bool
trp_node_name_is_empty(trt_node_name node_name)
{
    return node_name.str == NULL;
}

trt_opts_keys
trp_set_opts_keys()
{
    return 1;
}

trt_opts_keys
trp_empty_opts_keys()
{
    return 0;
}

ly_bool
trp_opts_keys_is_empty(trt_opts_keys keys)
{
    return keys == 0;
}

trt_type
trp_empty_type()
{
    trt_type ret;
    ret.type = trd_type_empty;
    return ret;
}

ly_bool
trp_type_is_empty(trt_type type)
{
    return type.type == trd_type_empty;
}

trt_iffeature
trp_set_iffeature()
{
    return 1;
}

trt_iffeature
trp_empty_iffeature()
{
    return 0;
}

ly_bool
trp_iffeature_is_empty(trt_iffeature iffeature)
{
    return !iffeature;
}

trt_node
trp_empty_node()
{
    trt_node ret = 
    {
        trd_status_type_empty, trd_flags_type_empty,
        trp_empty_node_name(), trp_empty_opts_keys(),
        trp_empty_type(), trp_empty_iffeature()
    };
    return ret;
}

ly_bool
trp_node_is_empty(trt_node node)
{
    const ly_bool a = trp_iffeature_is_empty(node.iffeatures);
    const ly_bool b = trp_type_is_empty(node.type);
    const ly_bool c = trp_opts_keys_is_empty(node.opts_keys);
    const ly_bool d = trp_node_name_is_empty(node.name);
    const ly_bool e = node.flags == trd_flags_type_empty;
    const ly_bool f = node.status == trd_status_type_empty;
    return a && b && c && d && e && f;
}

ly_bool
trp_node_body_is_empty(trt_node node)
{
    const ly_bool a = trp_iffeature_is_empty(node.iffeatures);
    const ly_bool b = trp_type_is_empty(node.type);
    const ly_bool c = trp_opts_keys_is_empty(node.opts_keys);
    return a && b && c;
}

trt_keyword_stmt
trp_empty_keyword_stmt()
{
    trt_keyword_stmt ret;
    ret.str = NULL;
    return ret;
}

ly_bool
trp_keyword_stmt_is_empty(trt_keyword_stmt ks)
{
    return ks.str == NULL;
}

void
trp_print_status(trt_status_type a, trt_printing p)
{
    switch(a) {
    case trd_status_type_current:
        trp_print(p, 1, trd_status_current);
        break;
    case trd_status_type_deprecated:
        trp_print(p, 1, trd_status_deprecated);
        break;
    case trd_status_type_obsolete:
        trp_print(p, 1, trd_status_obsolete);
        break;
    default:
        break;
    }
}

void
trp_print_flags(trt_flags_type a, trt_printing p)
{
    switch(a) {
    case trd_flags_type_rw:
        trp_print(p, 1, trd_flags_rw);
        break;
    case trd_flags_type_ro:
        trp_print(p, 1, trd_flags_ro);
        break;
    case trd_flags_type_rpc_input_params:
        trp_print(p, 1, trd_flags_rpc_input_params);
        break;
    case trd_flags_type_uses_of_grouping:
        trp_print(p, 1, trd_flags_uses_of_grouping);
        break;
    case trd_flags_type_rpc:
        trp_print(p, 1, trd_flags_rpc);
        break;
    case trd_flags_type_notif:
        trp_print(p, 1, trd_flags_notif);
        break;
    case trd_flags_type_mount_point:
        trp_print(p, 1, trd_flags_mount_point);
        break;
    default:
        break;
    }
}

size_t
trp_print_flags_strlen(trt_flags_type a)
{
    return a == trd_flags_type_empty ? 0 : 2;
}

void
trp_print_node_name(trt_node_name a, trt_printing p)
{
    if(trp_node_name_is_empty(a))
        return;

    const char* colon = a.module_prefix == NULL || a.module_prefix[0] == '\0' ? "" : trd_separator_colon;

    switch(a.type) {
    case trd_node_else:
        trp_print(p, 3, a.module_prefix, colon, a.str);
        break;
    case trd_node_case:
        trp_print(p, 5, trd_node_name_prefix_case, a.module_prefix, colon, a.str, trd_node_name_suffix_case);
        break;
    case trd_node_choice:
        trp_print(p, 5, trd_node_name_prefix_choice,  a.module_prefix, colon, a.str, trd_node_name_suffix_choice);
        break;
    case trd_node_optional_choice:
        trp_print(p, 6, trd_node_name_prefix_choice,  a.module_prefix, colon, a.str, trd_node_name_suffix_choice, trd_opts_optional);
        break;
    case trd_node_optional:
        trp_print(p, 4, a.module_prefix, colon, a.str, trd_opts_optional);
        break;
    case trd_node_container:
        trp_print(p, 4, a.module_prefix, colon, a.str, trd_opts_container);
        break;
    case trd_node_listLeaflist:
        trp_print(p, 4, a.module_prefix, colon, a.str, trd_opts_list);
        break;
    case trd_node_keys:
        trp_print(p, 4, a.module_prefix, colon, a.str, trd_opts_list);
        break;
    case trd_node_top_level1:
        trp_print(p, 4, a.module_prefix, colon, a.str, trd_opts_slash);
        break;
    case trd_node_top_level2:
        trp_print(p, 4, a.module_prefix, colon, a.str, trd_opts_at_sign);
        break;
    default:
        break;
    }
}

ly_bool
trp_mark_is_used(trt_node_name a)
{
    if(trp_node_name_is_empty(a))
        return 0;

    switch(a.type) {
    case trd_node_else:
    case trd_node_case:
    case trd_node_keys:
        return 0;
    default:
        return 1;
    }
}

void
trp_print_opts_keys(trt_opts_keys a, trt_indent_btw btw_name_opts, trt_cf_print_keys cf, trt_printing p)
{
    if(trp_opts_keys_is_empty(a))
        return;

    /* <name><mark>___<keys>*/
    trg_print_n_times(btw_name_opts, trd_separator_space[0], p);
    trp_print(p, 1, trd_opts_keys_prefix);
    cf.pf(cf.ctx, p);
    trp_print(p, 1, trd_opts_keys_suffix);
}

void
trp_print_type(trt_type a, trt_printing p)
{
    if(trp_type_is_empty(a))
        return;

    switch(a.type) {
    case trd_type_name:
        trp_print(p, 1, a.str);
        break;
    case trd_type_target:
        trp_print(p, 2, trd_type_target_prefix, a.str);
        break;
    case trd_type_leafref:
        trp_print(p, 1, trd_type_leafref_keyword);
    default:
        break;
    }
}

void
trp_print_iffeatures(trt_iffeature a, trt_cf_print_iffeatures cf, trt_printing p)
{
    if(trp_iffeature_is_empty(a))
        return;

    trp_print(p, 1, trd_iffeatures_prefix);
    cf.pf(cf.ctx, p);
    trp_print(p, 1, trd_iffeatures_suffix);
}

void
trp_print_node_up_to_name(trt_node a, trt_printing p)
{
    /* <status>--<flags> */
    trp_print_status(a.status, p);
    trp_print(p, 1, trd_separator_dashes);
    trp_print_flags(a.flags, p);
    /* If the node is a case node, there is no space before the <name> */
    if(a.name.type != trd_node_case)
        trp_print(p, 1, trd_separator_space);
    /* <name> */
    trp_print_node_name(a.name, p);
}

void
trp_print_divided_node_up_to_name(trt_node a, trt_printing p)
{
    uint32_t space = trp_print_flags_strlen(a.flags);

    if(a.name.type == trd_node_case) {
        /* :(<name> */
        space += strlen(trd_node_name_prefix_case);
    } else if(a.name.type == trd_node_choice) {
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

    trg_print_n_times(space, trd_separator_space[0], p);
}

void
trp_print_node(trt_node a, trt_pck_print pck, trt_indent_in_node ind, trt_printing p)
{
    if(trp_node_is_empty(a))
        return;

    /* <status>--<flags> <name><opts> <type> <if-features> */

    const ly_bool divided = ind.type == trd_indent_in_node_divided;
    const char char_space = trd_separator_space[0];

    if(!divided) {
        trp_print_node_up_to_name(a, p);
    } else {
        trp_print_divided_node_up_to_name(a, p);
    }

    /* <opts> */
    /* <name>___<opts>*/
    trt_cf_print_keys cf_print_keys = {pck.tree_ctx, pck.fps.print_keys};
    trp_print_opts_keys(a.opts_keys, ind.btw_name_opts, cf_print_keys, p);

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

void trt_print_keyword_stmt_begin(trt_keyword_stmt a, trt_printing p)
{
    switch(a.type) {
    case trd_keyword_stmt_top:
        switch(a.keyword) {
        case trd_keyword_module:
            trp_print(p, 1, trd_top_keyword_module);
            break;
        case trd_keyword_submodule:
            trp_print(p, 1, trd_top_keyword_submodule);
            break;
        default:
            break;
        }
        trp_print(p, 2, trd_separator_colon, trd_separator_space);
        break;
    case trd_keyword_stmt_body:
        trg_print_n_times(trd_indent_line_begin, trd_separator_space[0], p);
        switch(a.keyword) {
        case trd_keyword_augment:
            trp_print(p, 1, trd_body_keyword_augment);
            break;
        case trd_keyword_rpc:
            trp_print(p, 1, trd_body_keyword_rpc);
            break;
        case trd_keyword_notif:
            trp_print(p, 1, trd_body_keyword_notif);
            break;
        case trd_keyword_grouping:
            trp_print(p, 1, trd_body_keyword_grouping);
            break;
        case trd_keyword_yang_data:
            trp_print(p, 1, trd_body_keyword_yang_data);
            break;
        default:
            break;
        }
        trp_print(p, 1, trd_separator_space);
        break;
    default:
        break;
    }
}

size_t trp_keyword_type_strlen(trt_keyword_type a)
{
    switch(a) {
    case trd_keyword_module:
        return sizeof(trd_top_keyword_module) - 1;
    case trd_keyword_submodule:
        return sizeof(trd_top_keyword_submodule) - 1;
    case trd_keyword_augment:
        return sizeof(trd_body_keyword_augment) - 1;
    case trd_keyword_rpc:
        return sizeof(trd_body_keyword_rpc) - 1;
    case trd_keyword_notif:
        return sizeof(trd_body_keyword_notif) - 1;
    case trd_keyword_grouping:
        return sizeof(trd_body_keyword_grouping) - 1;
    case trd_keyword_yang_data:
        return sizeof(trd_body_keyword_yang_data) - 1;
    default:
        return 0;
    }
}

void
trt_print_keyword_stmt_str(trt_keyword_stmt a, uint32_t mll, trt_printing p)
{
    if(a.str == NULL || a.str[0] == '\0')
        return;

    /* module name cannot be splitted */
    if(a.type == trd_keyword_stmt_top) {
        trp_print(p, 1, a.str);
        return;
    }

    /* else for trd_keyword_stmt_body do */

    const char slash = trd_separator_slash[0];
    /* set begin indentation */
    const uint32_t ind_initial = trd_indent_line_begin + trp_keyword_type_strlen(a.keyword) + 1;
    const uint32_t ind_divided = ind_initial + trd_indent_long_line_break; 
    /* flag if path must be splitted to more lines */
    ly_bool linebreak_was_set = 0;
    /* flag if at least one subpath was printed */
    ly_bool subpath_printed = 0;
    /* the sum of the sizes of the substrings on the current line */
    uint32_t how_far = 0;

    /* pointer to start of the subpath */
    const char* sub_ptr = a.str;
    /* size of subpath from sub_ptr */
    size_t sub_len = 0;

    while(sub_ptr[0] != '\0') {
        /* skip slash */
        const char* tmp = sub_ptr[0] == slash ? sub_ptr + 1 : sub_ptr;
        /* get position of the end of substr */
        tmp = strchr(tmp, slash);
        /* set correct size if this is a last substring */
        sub_len = tmp == NULL ? strlen(sub_ptr) : (size_t)(tmp - sub_ptr);
        /* actualize sum of the substring's sizes on the current line */
        how_far += sub_len;
        /* correction due to colon character if it this is last substring */
        how_far = *(sub_ptr + sub_len + 1) == '\0' ? how_far + 1 : how_far;
        /* choose indentation which depends on
         * whether the string is printed on multiple lines or not
         */
        uint32_t ind = linebreak_was_set ? ind_divided : ind_initial;
        if(ind + how_far <= mll) {
            /* printing before max line length */
            sub_ptr = trg_print_substr(sub_ptr, sub_len, p);
            subpath_printed = 1;
        } else {
            /* printing on new line */
            if(subpath_printed == 0) {
                /* first subpath is too long but print it at first line anyway */
                sub_ptr = trg_print_substr(sub_ptr, sub_len, p);
                subpath_printed = 1;
                continue;
            }
            trg_print_linebreak(p);
            trg_print_n_times(ind_divided, trd_separator_space[0], p);
            linebreak_was_set = 1;
            sub_ptr = trg_print_substr(sub_ptr, sub_len, p);
            how_far = sub_len;
            subpath_printed = 1;
        }
    }
}

void
trt_print_keyword_stmt_end(trt_keyword_stmt a, trt_printing p)
{
    if(a.type == trd_keyword_stmt_body)
        trp_print(p, 1, trd_separator_colon);
}

void
trp_print_keyword_stmt(trt_keyword_stmt a, uint32_t mll, trt_printing p)
{
    if(trp_keyword_stmt_is_empty(a))
        return;
    trt_print_keyword_stmt_begin(a, p);
    trt_print_keyword_stmt_str(a, mll, p);
    trt_print_keyword_stmt_end(a, p);
}

void
trp_print_line(trt_node node, trt_pck_print pck, trt_pck_indent ind, trt_printing p)
{
    trp_print_wrapper(ind.wrapper, p);
    trp_print_node(node, pck, ind.in_node, p);
}

void
trp_print_line_up_to_node_name(trt_node node, trt_wrapper wr, trt_printing p)
{
    trp_print_wrapper(wr, p);
    trp_print_node_up_to_name(node, p);
}


ly_bool trp_leafref_target_is_too_long(trt_node node, trt_wrapper wr, uint32_t mll)
{
    if(node.type.type != trd_type_target)
        return 0;

    trt_counter cnt = {0};
    /* inject print function with strlen */
    trt_injecting_strlen func = {&cnt, trp_injected_strlen};
    /* count number of printed bytes */
    trp_print_wrapper(wr, func);
    trg_print_n_times(trd_indent_btw_siblings, trd_separator_space[0], func);
    trp_print_divided_node_up_to_name(node, func);

    return cnt.bytes + strlen(node.type.str) > mll;
}

trt_indent_in_node
trp_default_indent_in_node(trt_node node)
{
    trt_indent_in_node ret;
    ret.type = trd_indent_in_node_normal;

    /* btw_name_opts */
    ret.btw_name_opts = !trp_opts_keys_is_empty(node.opts_keys) ? 
        trd_indent_before_keys : 0;

    /* btw_opts_type */
    if(!trp_type_is_empty(node.type)) {
        ret.btw_opts_type = trp_mark_is_used(node.name) ? 
            trd_indent_before_type - trd_opts_mark_length:
            trd_indent_before_type;
    } else {
        ret.btw_opts_type = 0;
    }

    /* btw_type_iffeatures */
    ret.btw_type_iffeatures = !trp_iffeature_is_empty(node.iffeatures) ?
        trd_indent_before_iffeatures : 0;

    return ret;
}

void
trp_print_entire_node(trt_node node, trt_pck_print ppck, trt_pck_indent ipck, uint32_t mll, trt_printing p)
{
    if(ipck.in_node.type == trd_indent_in_node_unified) {
        /* TODO: special case */
        trp_print_line(node, ppck, ipck, p);
        return;
    }

    if(trp_leafref_target_is_too_long(node, ipck.wrapper, mll)) {
        node.type.type = trd_type_leafref;
    }

    /* check if normal indent is possible */
    trt_pair_indent_node ind_node1 = trp_try_normal_indent_in_node(node, ppck, ipck, mll);

    if(ind_node1.indent.type == trd_indent_in_node_normal) {
        /* node fits to one line */
        trp_print_line(node, ppck, ipck, p);
    } else if(ind_node1.indent.type == trd_indent_in_node_divided) {
        /* node will be divided */
        /* print first half */
        {
            trt_pck_indent tmp = {ipck.wrapper, ind_node1.indent};
            /* pretend that this is normal node */
            tmp.in_node.type = trd_indent_in_node_normal;
            trp_print_line(ind_node1.node, ppck, tmp, p);
        }
        trg_print_linebreak(p);
        /* continue with second half on new line */
        {
            trt_pair_indent_node ind_node2 = trp_second_half_node(node, ind_node1.indent);
            trt_pck_indent tmp = {trp_wrapper_set_mark(ipck.wrapper), ind_node2.indent};
            trp_print_divided_node(ind_node2.node, ppck, tmp, mll, p);
        }
    } else if(ind_node1.indent.type == trd_indent_in_node_failed){
        /* node name is too long */
        trp_print_line_up_to_node_name(node, ipck.wrapper, p);
        if(trp_node_body_is_empty(node)) {
            return;
        } else {
            trg_print_linebreak(p);
            trt_pair_indent_node ind_node2 = trp_second_half_node(node, ind_node1.indent);
            ind_node2.indent.type = trd_indent_in_node_divided;
            trt_pck_indent tmp = {trp_wrapper_set_mark(ipck.wrapper), ind_node2.indent};
            trp_print_divided_node(ind_node2.node, ppck, tmp, mll, p);
        }

    }
}

void
trp_print_divided_node(trt_node node, trt_pck_print ppck, trt_pck_indent ipck, uint32_t mll, trt_printing p)
{
    trt_pair_indent_node ind_node = trp_try_normal_indent_in_node(node, ppck, ipck, mll);

    if(ind_node.indent.type == trd_indent_in_node_failed) {
        /* nothing can be done, continue as usual */
        ind_node.indent.type = trd_indent_in_node_divided;
    }

    trp_print_line(ind_node.node, ppck, (trt_pck_indent){ipck.wrapper, ind_node.indent}, p);

    const ly_bool entire_node_was_printed = trp_indent_in_node_are_eq(ipck.in_node, ind_node.indent);
    if(!entire_node_was_printed) {
        trg_print_linebreak(p);
        /* continue with second half node */
        //ind_node = trp_second_half_node(ind_node.node, ind_node.indent);
        ind_node = trp_second_half_node(node, ind_node.indent);
        /* continue with printing entire node */
        trp_print_divided_node(ind_node.node, ppck, (trt_pck_indent){ipck.wrapper, ind_node.indent}, mll, p);
    } else { 
        return;
    }
}

trt_pair_indent_node trp_first_half_node(trt_node node, trt_indent_in_node ind)
{
    trt_pair_indent_node ret = {ind, node};

    if(ind.btw_name_opts == trd_linebreak) {
        ret.node.opts_keys = trp_empty_opts_keys();
        ret.node.type = trp_empty_type();
        ret.node.iffeatures = trp_empty_iffeature();
    } else if(ind.btw_opts_type == trd_linebreak) {
        ret.node.type = trp_empty_type();
        ret.node.iffeatures = trp_empty_iffeature();
    } else if(ind.btw_type_iffeatures == trd_linebreak) {
        ret.node.iffeatures = trp_empty_iffeature();
    }

    return ret;
}

trt_pair_indent_node trp_second_half_node(trt_node node, trt_indent_in_node ind)
{
    trt_pair_indent_node ret = {ind, node};

    if(ind.btw_name_opts < 0) {
        /* Logically, the information up to token <opts> should be deleted,
         * but the the trp_print_node function needs it to create
         * the correct indent.
         */
        ret.indent.btw_name_opts = 0;
        ret.indent.btw_opts_type = trp_type_is_empty(node.type) ?
            0 : trd_indent_before_type;
        ret.indent.btw_type_iffeatures = trp_iffeature_is_empty(node.iffeatures) ?
            0 : trd_indent_before_iffeatures;
    } else if(ind.btw_opts_type == trd_linebreak) {
        ret.node.opts_keys = trp_empty_opts_keys();
        ret.indent.btw_name_opts = 0;
        ret.indent.btw_opts_type = 0;
        ret.indent.btw_type_iffeatures = trp_iffeature_is_empty(node.iffeatures) ?
            0 : trd_indent_before_iffeatures;
    } else if(ind.btw_type_iffeatures == trd_linebreak) {
        ret.node.opts_keys = trp_empty_opts_keys();
        ret.node.type = trp_empty_type();
        ret.indent.btw_name_opts = 0;
        ret.indent.btw_opts_type = 0;
        ret.indent.btw_type_iffeatures = 0;
    }
    return ret;
}

trt_indent_in_node trp_indent_in_node_place_break(trt_indent_in_node ind)
{
    /* somewhere must be set a line break in node */
    trt_indent_in_node ret = ind;
    /* gradually break the node from the end */
    if(ind.btw_type_iffeatures != trd_linebreak && ind.btw_type_iffeatures != 0) {
        ret.btw_type_iffeatures = trd_linebreak;
    } else if(ind.btw_opts_type != trd_linebreak && ind.btw_opts_type != 0) {
        ret.btw_opts_type = trd_linebreak;
    } else if(ind.btw_name_opts != trd_linebreak && ind.btw_name_opts != 0) {
        /* set line break between name and opts */
        ret.btw_name_opts = trd_linebreak;
    } else {
        /* it is not possible to place a more line breaks,
         * unfortunately the max_line_length constraint is violated
         */
        ret.type = trd_indent_in_node_failed;
    }
    return ret;
}

trt_pair_indent_node
trp_try_normal_indent_in_node(trt_node n, trt_pck_print p, trt_pck_indent ind, uint32_t mll)
{
    trt_counter cnt = {0};
    /* inject print function with strlen */
    trt_injecting_strlen func = {&cnt, trp_injected_strlen};
    /* count number of printed bytes */
    trp_print_line(n, p, ind, func);

    trt_pair_indent_node ret = {ind.in_node, n};

    if(cnt.bytes <= mll) {
        /* success */
        return ret;
    } else {
        ret.indent = trp_indent_in_node_place_break(ret.indent);
        if(ret.indent.type != trd_indent_in_node_failed) {
            /* erase information in node due to line break */
            ret = trp_first_half_node(n, ret.indent);
            /* check if line fits, recursive call */
            ret = trp_try_normal_indent_in_node(ret.node, p, (trt_pck_indent){ind.wrapper, ret.indent}, mll);
            /* make sure that the result will be with the status divided
             * or eventually with status failed */
            ret.indent.type = ret.indent.type == trd_indent_in_node_failed ?
                trd_indent_in_node_failed : trd_indent_in_node_divided;
        }
        return ret;
    }
}

/* ----------- <Definition of tree functions> ----------- */

/* ----------- <Definition of tree browsing functions> ----------- */

trt_indent_in_node
trb_try_unified_indent(struct trt_printer_ctx* UNUSED(pc))
{
    trt_indent_in_node ind;
    ind.type = trd_indent_in_node_failed;
    return ind;
    /* TODO: get longest node name,
     * try indentation in all siblings
     * if failed, than second longest node and repeat
     * restore to the initial pc session by next_child(parent)
     */
}

ly_bool
trb_node_is_last_sibling(struct trt_fp_read read, struct trt_tree_ctx* tc)
{
    return trp_node_is_empty(read.next_sibling(tc));
}

ly_bool
trb_parent_is_last_sibling(struct trt_fp_all fp, struct trt_tree_ctx* tc)
{
    fp.modify.parent(tc);
    ly_bool ret = trp_node_is_empty(fp.read.next_sibling(tc));
    fp.modify.next_child(tc);
    return ret;
}

void
trb_print_nodes(trt_wrapper wr, struct trt_printer_ctx* pc, struct trt_tree_ctx* tc)
{
    /* try unified indentation in node */
    trt_indent_in_node unified_ind = trb_try_unified_indent(pc);
    /* if node is last sibling, then do not add '|' to wrapper */
    wr = trb_parent_is_last_sibling(pc->fp, tc) ?
        trp_wrapper_set_shift(wr) :
        trp_wrapper_set_mark(wr);

    /* print all siblings */
    do {
        /* print linebreak before printing actual node */
        trg_print_linebreak(pc->print);
        /* set unified_indentation if trb_try_unified_indent succeeded */
        if(unified_ind.type == trd_indent_in_node_unified) {
            /* print actual node with unified indent */
            trp_print_entire_node(pc->fp.read.node(tc), (trt_pck_print){tc, pc->fp.print},
                (trt_pck_indent){wr, unified_ind},
                pc->max_line_length, pc->print);
        } else {
            /* print actual node with default node indent */
            trt_node node = pc->fp.read.node(tc);
            trp_print_entire_node(node, (trt_pck_print){tc, pc->fp.print},
                (trt_pck_indent){wr, trp_default_indent_in_node(node)},
                pc->max_line_length, pc->print);
        }
        /* go to the actual node's child or stay in actual node */
        if(!trp_node_is_empty(pc->fp.modify.next_child(tc))) {
            /* print all childs - recursive call */
            trb_print_nodes(wr, pc, tc);
            /* get back from child node to actual node */
            pc->fp.modify.parent(tc);
        }
        /* go to the next sibling or stay in actual node */
    } while(!trp_node_is_empty(pc->fp.modify.next_sibling(tc)));
}

/**
 * @brief Recursive nodes printing
 */
void
trb_print_subtree_nodes(trt_wrapper wr, struct trt_printer_ctx pc, struct trt_tree_ctx* tc)
{
    /* before root node is no linebreak printing. This must be addressed by the caller. */
    /* print root node */
    trt_node root = pc.fp.read.node(tc);
    trp_print_entire_node(root, (trt_pck_print){tc, pc.fp.print},
        (trt_pck_indent){wr, trp_default_indent_in_node(root)}, pc.max_line_length, pc.print);
    /* go to the actual node's child or stay in actual node */
    if(!trp_node_is_empty(pc.fp.modify.next_child(tc))) {
        /* print root's nodes */
        trb_print_nodes(wr, &pc, tc);
        /* get back from child node to actual node */
        pc.fp.modify.parent(tc);
    }
}


/* ----------- <Definition of the other functions> ----------- */

#define PRINT_N_TIMES_BUFFER_SIZE 16

void
trg_print_n_times(int32_t n, char c, trt_printing p)
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

ly_bool
trg_test_bit(uint64_t number, uint32_t bit)
{
    return (number >> bit) & 1U;
}

void
trg_print_linebreak(trt_printing p)
{
    trp_print(p, 1, trd_separator_linebreak);
}

const char*
trg_print_substr(const char* str, size_t len, trt_printing p)
{
    for(size_t i = 0; i < len; i++) {
        trg_print_n_times(1, str[0], p);
        str++;
    }
    return str;
}

/* ----------- <Definition of module interface> ----------- */

//LY_ERR tree_print_parsed_and_compiled_module(struct ly_out *out, const struct lys_module *module, uint32_t options, size_t line_length)
LY_ERR tree_print_parsed_and_compiled_module(struct ly_out *UNUSED(out), const struct lys_module *UNUSED(module), uint32_t UNUSED(options), size_t UNUSED(line_length))
{
    return 0;
}

//LY_ERR tree_print_submodule(struct ly_out *out, const struct lys_module *module, const struct lysp_submodule *submodp, uint32_t options, size_t line_length)
LY_ERR tree_print_submodule(struct ly_out *UNUSED(out), const struct lys_module *UNUSED(module), const struct lysp_submodule *UNUSED(submodp), uint32_t UNUSED(options), size_t UNUSED(line_length))
{
    return 0;
}

//LY_ERR tree_print_compiled_node(struct ly_out *out, const struct lysc_node *node, uint32_t options, size_t line_length)
LY_ERR tree_print_compiled_node(struct ly_out *UNUSED(out), const struct lysc_node *UNUSED(node), uint32_t UNUSED(options), size_t UNUSED(line_length))
{
    return 0;
}


/* -######-- Definitions end -#######- */
