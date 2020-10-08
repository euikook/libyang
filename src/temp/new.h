/**
 * @file new.h
 * @author Adam Piecek <piecek@cesnet.cz>
 * @brief tree printer
 */

/* TODO: rename to printer_tree.c */
/* TODO: merge new.c to printer_tree.c */
/* TODO: line break due to long prefix string */

#ifndef NEW_H_
#define NEW_H_

#include <stdint.h> /* uint_, int_ */
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h> /* NULL */


struct trt_tree_ctx;
struct trt_getters;
struct trt_fp_modify_ctx;
struct trt_fp_read;
struct trt_fp_crawl;
struct trt_fp_print;

/**
 * @brief General function for printing
 *
 * Variadic arguments are expected to be of the type char*.
 *
 * @param[in] out struct ly_out* or other auxiliary structure for printing
 */
typedef void (*trt_print_func)(void *out, int arg_count, va_list ap);


typedef struct
{
    void* out;
    trt_print_func pf;
} trt_printing,
  trt_injecting_strlen;

void trp_print(trt_printing, int arg_count, ...);

typedef struct 
{
    const struct trt_tree_ctx* ctx;
    void (*pf)(const struct trt_tree_ctx *, trt_printing);
} trt_cf_print_keys,
  trt_cf_print_iffeatures;

typedef uint32_t trt_printer_opts;

typedef struct
{
    uint32_t bytes;
} trt_counter;

/**
 * @brief Function that counts the characters to be printed instead of printing
 *
 * @param[in,out] out it is expected to be type trt_counter
 * @param[in] arg_count number of arguments in ...
 */
void trp_injected_strlen(void *out, int arg_count, va_list ap); 

typedef struct
{
    const char* src;
    const char* substr_start;
    size_t substr_size;
} trt_breakable_str;

trt_breakable_str trp_empty_breakable_str();
trt_breakable_str trp_set_breakable_str(const char*);
bool trp_breakable_str_is_empty(trt_breakable_str);;
bool trp_breakable_str_begin_will_be_printed(trt_breakable_str);
bool trp_breakable_str_end_will_be_printed(trt_breakable_str);
void trp_print_breakable_str(trt_breakable_str, trt_printing);

/* ======================================= */
/* ----------- <Print getters> ----------- */
/* ======================================= */

/**
 * @brief Functions that provide printing themselves
 */
struct trt_fp_print
{
    void (*print_features_names)(const struct trt_tree_ctx*, trt_printing);   /*<< print including spaces between names */
    void (*print_keys)(const struct trt_tree_ctx *, trt_printing);            /*<< print including delimiters between names */
};

typedef struct
{
    const struct trt_tree_ctx* tree_ctx;
    struct trt_fp_print fps;
} trt_pck_print;

/* ================================ */
/* ----------- <indent> ----------- */
/* ================================ */

typedef enum
{
    trd_indent_one = 1,
    trd_indent_empty = 0,               /**< If the node is a case node, there is no space before the <name> */
    trd_indent_long_line_break = 2,     /**< The new line should be indented so that it starts below <name> with a whitespace offset of at least two characters. */
    trd_indent_line_begin = 2,          /**< indent below the keyword (module, augment ...)  */
    trd_indent_btw_siblings = 2,
    trd_indent_before_keys = 1,         /**< <x>___<keys> */
    trd_indent_before_type = 4,         /**< <x>___<type>, but if mark is set then indent == 3 */
    trd_indent_before_iffeatures = 1,   /**< <x>___<iffeatures> */
} trt_cnf_indent;


typedef enum
{
    trd_indent_in_node_normal = 0,
    trd_indent_in_node_unified,
    trd_indent_in_node_divided,     /**< the node is not on one line, but on more */
    trd_indent_in_node_failed       /**< indent is not valid by RFC */

} trt_indent_in_node_type;

typedef int16_t trt_indent_btw;
const trt_indent_btw trd_linebreak = -1;

typedef struct
{
    trt_indent_in_node_type type;
    trt_indent_btw btw_name_opts;       /**< variable unified_for_all_subtrees has no influence */
    trt_indent_btw btw_opts_type;
    trt_indent_btw btw_type_iffeatures; /**< ignored if <type> missing */
} trt_indent_in_node;

bool trp_indent_in_node_are_eq(trt_indent_in_node, trt_indent_in_node);
trt_indent_in_node trp_indent_in_node_place(trt_indent_in_node);

typedef enum 
{
    trd_wrapper_top = 0,
    trd_wrapper_body
} trd_wrapper_type;

/**
 * @brief For resolving sibling symbol placement
 *
 * Bit indicates where the sibling symbol must be printed.
 * This place is in multiples of trd_indent_before_status.
 */
typedef struct
{
    trd_wrapper_type type;
    uint64_t bit_marks1;
    uint32_t actual_pos;
} trt_wrapper;

trt_wrapper trp_init_wrapper_top();

trt_wrapper trp_init_wrapper_body();

/**
 * @brief Setting mark in .bit_marks at position .actual_pos
 */
trt_wrapper trp_wrapper_set_mark(trt_wrapper);

/**
 * @brief Set shift to the right (next sibling symbol position)
 */
trt_wrapper trp_wrapper_set_shift(trt_wrapper);

bool trt_wrapper_eq(trt_wrapper, trt_wrapper);

/**
 * @brief Print "  |" sequence.
 */
void trp_print_wrapper(trt_wrapper, trt_printing);


typedef struct
{
    trt_wrapper wrapper;
    trt_indent_in_node in_node;
} trt_pck_indent;

/* ================================== */
/* ----------- <node_name> ----------- */
/* ================================== */

typedef const char* trt_node_name_prefix;
static const char trd_node_name_prefix_choice[] = "(";
static const char trd_node_name_prefix_case[] = ":(";

typedef const char* trt_node_name_suffix;
static const char trd_node_name_suffix_choice[] = ")";
static const char trd_node_name_suffix_case[] = ")";

typedef enum
{
    trd_node_else = 0,
    trd_node_case,
    trd_node_choice,
    trd_node_optional_choice,
    trd_node_optional,              /**< for an optional leaf, anydata, or anyxml */
    trd_node_container,             /**< for a presence container */
    trd_node_listLeaflist,          /**< for a leaf-list or list (without keys) */
    trd_node_keys,                  /**< for a list's keys */
    trd_node_top_level1,            /**< for a top-level data node in a mounted module */
    trd_node_top_level2             /**< for a top-level data node of a module identified in a mount point parent reference */
} trt_node_type;

typedef struct
{
    trt_node_type type;
    const char* module_prefix;            /**< prefix defined in the module where the node is defined */
    const char* str;
} trt_node_name;


trt_node_name trp_empty_node_name();
bool trp_node_name_is_empty(trt_node_name);
void trp_print_node_name(trt_node_name, trt_printing);
bool trp_mark_is_used(trt_node_name);

/* ============================== */
/* ----------- <opts> ----------- */
/* ============================== */

static const char trd_opts_optional[] = "?";   /**< for an optional leaf, choice, anydata, or anyxml */ 
static const char trd_opts_container[] = "!";       /**< for a presence container */
static const char trd_opts_list[] = "*";            /**< for a leaf-list or list */
static const char trd_opts_slash[] = "/";           /**< for a top-level data node in a mounted module */
static const char trd_opts_at_sign[] = "@";         /**< for a top-level data node of a module identified in a mount point parent reference */
static const size_t trd_opts_mark_length = 1;       /**< every opts mark has a length of one */

typedef const char* trt_opts_keys_prefix;
static const char trd_opts_keys_prefix[] = "[";
typedef const char* trt_opts_keys_suffix;
static const char trd_opts_keys_suffix[] = "]";

typedef bool trt_opts_keys;

trt_opts_keys trp_set_opts_keys();
trt_opts_keys trp_empty_opts_keys();
bool trp_opts_keys_is_empty(trt_opts_keys);
void trp_print_opts_keys(trt_opts_keys, trt_indent_btw, trt_cf_print_keys, trt_printing);

/* ============================== */
/* ----------- <type> ----------- */
/* ============================== */

typedef const char* trt_type_leafref;
static const char trd_type_leafref_keyword[] = "leafref";
typedef const char* trt_type_target_prefix;
static const char trd_type_target_prefix[] = "-> ";

typedef enum
{
    trd_type_name = 0,
    trd_type_target,
    trd_type_leafref,
    trd_type_empty
} trt_type_type;

typedef struct
{
    trt_type_type type;
    trt_breakable_str str;
} trt_type;

trt_type trp_empty_type();
bool trp_type_is_empty(trt_type);
void trp_print_type(trt_type, trt_printing);

/* ==================================== */
/* ----------- <iffeatures> ----------- */
/* ==================================== */

typedef const char* trt_iffeatures_prefix;
static const char trd_iffeatures_prefix[] = "{";
typedef const char* trt_iffeatures_suffix;
static const char trd_iffeatures_suffix[] = "}?";
typedef bool trt_iffeature;

trt_iffeature trp_set_iffeature();
trt_iffeature trp_empty_iffeature();
bool trp_iffeature_is_empty(trt_iffeature);

/**
 * @brief Print all iffeatures of node 
 *
 * @param[in] print_iffeatures added function which finds and prints all iffeatures
 */
void trp_print_iffeatures(trt_iffeature, trt_cf_print_iffeatures, trt_printing);

/* ============================== */
/* ----------- <node> ----------- */
/* ============================== */

typedef const char* trt_status;
static const char trd_status_current[] = "+";
static const char trd_status_deprecated[] = "x";
static const char trd_status_obsolete[] = "o";

typedef const char* trt_flags;
static const char trd_flags_rw[] = "rw";
static const char trd_flags_ro[] = "ro";
static const char trd_flags_rpc_input_params[] = "-w";
static const char trd_flags_uses_of_grouping[] = "-u";
static const char trd_flags_rpc[] = "-x";
static const char trd_flags_notif[] = "-n";
static const char trd_flags_mount_point[] = "mp";
static const char trd_flags_empty[] = "";              /**<  Case nodes do not have any flags */

typedef struct
{
    trt_status status;
    trt_flags flags;
    trt_node_name name;
    trt_opts_keys opts_keys;
    trt_type type;                  /**< is the name of the type for leafs and leaf-lists */
    trt_iffeature iffeatures;  
} trt_node;

trt_node trp_empty_node();
bool trp_node_is_empty(trt_node);
void trp_print_node(trt_node, trt_pck_print, trt_indent_in_node, trt_printing);

typedef struct
{
    trt_indent_in_node indent;
    trt_node node;
}trt_pair_indent_node;

trt_pair_indent_node trp_first_half_node(trt_node, trt_indent_in_node);
trt_pair_indent_node trp_second_half_node(trt_node, trt_indent_in_node);

/* =================================== */
/* ----------- <statement> ----------- */
/* =================================== */

typedef const char* trt_top_keyword;
static const char trd_top_keyword_module[] = "module";
static const char trd_top_keyword_submodule[] = "submodule";

typedef const char* trt_body_keyword;
static const char trd_body_keyword_augment[] = "augment";
static const char trd_body_keyword_rpc[] = "rpcs";
static const char trd_body_keyword_notif[] = "notifications";
static const char trd_body_keyword_grouping[] = "grouping";
static const char trd_body_keyword_yang_data[] = "yang-data";

typedef enum
{
    trd_keyword_stmt_top,
    trd_keyword_stmt_body,
} trt_keyword_stmt_type;

typedef struct
{
    trt_keyword_stmt_type type;
    trt_top_keyword keyword;
    trt_breakable_str str;
} trt_keyword_stmt;

trt_keyword_stmt trp_empty_keyword_stmt();
bool trp_keyword_stmt_is_empty(trt_keyword_stmt);
void trp_print_keyword_stmt(trt_keyword_stmt, trt_printing);

/* ======================================== */
/* ----------- <Modify getters> ----------- */
/* ======================================== */

/**
 * @brief Functions that change the state of the tree_ctx structure
 *
 * For all, if the value cannot be returned, its empty version obtained by the corresponding function returning the empty value is returned.
 */
struct trt_fp_modify_ctx
{
    trt_node (*next_sibling)(struct trt_tree_ctx*);
    trt_node (*next_child)(struct trt_tree_ctx*);
    trt_keyword_stmt (*next_augment)(struct trt_tree_ctx*);
    trt_keyword_stmt (*next_grouping)(struct trt_tree_ctx*);
    trt_keyword_stmt (*next_yang_data)(struct trt_tree_ctx*);
};

/* ====================================== */
/* ----------- <Read getters> ----------- */
/* ====================================== */

/**
 * @brief Functions providing information for the print
 *
 * For all, if the value cannot be returned, its empty version obtained by the corresponding function returning the empty value is returned.
 */
struct trt_fp_read
{
    trt_keyword_stmt (*module_name)(const struct trt_tree_ctx*);
    trt_node (*node)(const struct trt_tree_ctx*);
};

/* ===================================== */
/* ----------- <All getters> ----------- */
/* ===================================== */

/**
 * @brief A set of all necessary functions that must be provided for the printer
 */
struct trt_fp_all
{
    struct trt_fp_modify_ctx modify;
    struct trt_fp_read read;
    struct trt_fp_print print;
};

/* ========================================= */
/* ----------- <Printer context> ----------- */
/* ========================================= */

/**
 * @brief Main structure for part of the printer
 */
struct trt_printer_ctx
{
    trt_printer_opts options;
    trt_wrapper wrapper;
    trt_printing print;
    struct trt_fp_all fp;
    uint32_t max_line_length;   /**< including last character */
};

/* ====================================== */
/* ----------- <Tree context> ----------- */
/* ====================================== */

#if 0

struct lys_module;
struct lysc_node;
struct lysp_node;

typedef enum
{
    data,
    augment,
    grouping,
    yang_data,
} trt_subtree_type;

/**
 * @brief Main structure for browsing the libyang tree
 */
struct trt_tree_ctx
{
    struct ly_out *out;
    const struct lys_module *module;
    trt_subtree_type node_ctx;
    struct lysc_node *act_cnode;
    struct lysp_node *act_pnode;
};

#endif

/* ======================================== */
/* ----------- <Main functions> ----------- */
/* ======================================== */

/**
 * @brief Execute Printer - print tree
 */
void trp_main(struct trt_printer_ctx, struct trt_tree_ctx*);

/**
 * @brief Print one line
 */
void trp_print_line(trt_node, trt_pck_print, trt_pck_indent, trt_printing);

/**
 * @brief Print an entire node that can be split into multiple lines.
 */
void trp_print_entire_node(trt_node, trt_pck_print, trt_pck_indent, uint32_t mll, trt_printing);

void trp_print_divided_node(trt_node, trt_pck_print, trt_pck_indent, uint32_t mll, trt_printing);

/**
 * @brief Recursive nodes printing
 */
void trp_print_nodes(struct trt_printer_ctx, struct trt_tree_ctx*, trt_pck_indent);

/**
 * @brief Get default indent in node based on node values.
 */
trt_indent_in_node trp_default_indent_in_node(trt_node);

/**
 * @brief Get the correct alignment for the node
 *
 * @return .type == trd_indent_in_node_divided - the node does not fit in the line, some .trt_indent_btw has negative value as a line break sign.
 * @return .type == trd_indent_in_node_normal - the node fits into the line, all .trt_indent_btw values has non-negative number.
 * @return .type == trd_indent_in_node_failed - the node does not fit into the line, all .trt_indent_btw has negative or zero values, function failed.
 */
trt_pair_indent_node trp_try_normal_indent_in_node(trt_node, trt_pck_print, trt_pck_indent, uint32_t mll);

/**
 * @brief Find out if it is possible to unify the alignment in all subtrees
 *
 * The aim is to make it a little bit similar to two columns.
*/
trt_indent_in_node trp_try_unified_indent(struct trt_printer_ctx);

/* =================================== */
/* ----------- <separator> ----------- */
/* =================================== */

typedef const char* const trt_separator;
static trt_separator trd_separator_colon = ":";
static trt_separator trd_separator_space = " ";
static trt_separator trd_separator_dashes = "--";
static trt_separator trd_separator_linebreak = "\n";

void trg_print_n_times(int32_t n, char, trt_printing);

bool trg_test_bit(uint64_t number, uint32_t bit);

void trg_print_linebreak(trt_printing);

/* ================================ */
/* ----------- <symbol> ----------- */
/* ================================ */

typedef const char* const trt_symbol;
static trt_symbol trd_symbol_sibling = "|";


#endif
