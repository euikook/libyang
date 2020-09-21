/**
 * @file new.h
 * @author Adam Piecek <piecek@cesnet.cz>
 * @brief tree printer
 */

/* TODO: rename to printer_tree.c */
/* TODO: merge new.c to printer_tree.c */
/* TODO: line break? */

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
 * @param[in] print_struct struct ly_out* or other auxiliary structure for printing
 */
typedef void (*trt_print_func)(void *print_struct, ...);
typedef uint32_t trt_printer_opts;

/* ================================ */
/* ----------- <indent> ----------- */
/* ================================ */

typedef enum
{
    trd_indent_one = 1,
    trd_indent_empty = 0,                   /**< If the node is a case node, there is no space before the <name> */
    trd_indent_long_line_break_offset = 2,  /**< The new line should be indented so that it starts below <name> with a whitespace offset of at least two characters. */
    trd_indent_before_status = 2,           /**< Sequence of "--" is 2 spaces long. */
} trt_cnf_indent;

typedef int16_t trt_indent_btw;

typedef struct
{
    bool unified_for_all_subtrees;
    trt_indent_btw btw_name_opts;       /**< variable unified_for_all_subtrees has no influence */
    trt_indent_btw btw_opts_type;
    trt_indent_btw btw_type_iffeatures; /**< ignored if <type> missing */
} trt_indent_in_node;

/**
 * @brief For resolving sibling symbol placement
 *
 * Bit indicates where the sibling symbol must be printed.
 * This place is in multiples of trd_indent_before_status.
 */
typedef struct
{
    uint64_t bit_marks1;
    uint32_t actual_pos;
} trt_wrapper;

/**
 * @brief Setting mark in .bit_marks at position .actual_pos
 */
trt_wrapper trp_wrapper_set_mark(trt_wrapper);

/**
 * @brief Set shift to the right (next sibling symbol position)
 */
trt_wrapper trp_wrapper_set_shift(trt_wrapper);

/**
 * @brief how many characters the wrapper occupies from the left edge of the printout to the last <opts> position
 */
uint32_t trp_wrapper_strlen(trt_wrapper);

typedef struct
{
    trt_indent_in_node in_node;
    trt_wrapper wrapper;
} trt_indent;

/* ================================== */
/* ----------- <node_name> ----------- */
/* ================================== */

typedef const char* trt_node_name_prefix;
static const char trd_node_name_prefix_empty[] = "";
static const char trd_node_name_prefix_choice[] = "(";
static const char trd_node_name_prefix_case[] = ":(";

typedef const char* trt_node_name_suffix;
static const char trd_node_name_suffix_empty[] = "";
static const char trd_node_name_suffix_choice[] = ")";
static const char trd_node_name_suffix_case[] = ")";

typedef enum
{
    trd_node_type_else,
    trd_node_type_choice,
    trd_node_type_case,
} trt_node_type;

typedef struct
{
    trt_node_type type;
    const char* module_prefix;            /**< prefix defined in the module where the node is defined */
    const char* str;
} trt_node_name;

trt_node_name trp_empty_node_name();
bool trp_node_name_is_empty(trt_node_name);
void trp_print_node_name(trt_node_name, trt_print_func);
uint32_t trp_strlen_node_name(trt_node_name);

/* ============================== */
/* ----------- <opts> ----------- */
/* ============================== */

typedef const char* trt_opts_mark;
static const char trd_opts_question_mark[] = "?"; /**< for an optional leaf, choice, anydata, or anyxml */ 
static const char trd_opts_container[] = "!";     /**< for a presence container */
static const char trd_opts_list[] = "*";          /**< for a leaf-list or list */
static const char trd_opts_slash[] = "/";         /**< for a top-level data node in a mounted module */
static const char trd_opts_at_sign[] = "@";       /**< for a top-level data node of a module identified in a mount point parent reference */
static const char trd_opts_empty[] = "";

typedef const char* trt_opts_keys_prefix;
static const char trd_opts_keys_prefix[] = "[";
typedef const char* trt_opts_keys_suffix;
static const char trd_opts_keys_suffix[] = "]";

typedef enum
{
    trd_opts_type_empty,
    trd_opts_type_mark,
    trd_opts_type_keys,
} trt_opts_type;

typedef struct
{
    trt_opts_type id;
    const char* mark_or_keys;
} trt_opts;

trt_opts trp_empty_opts();
bool trp_opts_is_empty(trt_opts);

/**
 * @brief Print all [keys] of node 
 *
 * @param[in] print_keys added function which finds and prints all keys
 */
void trp_print_opts(trt_opts, void (*print_keys)(const struct trt_tree_ctx *), trt_print_func);
uint32_t trp_strlen_opts(trt_opts, uint32_t(*strlen_keys)(const struct trt_tree_ctx*));

/* ============================== */
/* ----------- <type> ----------- */
/* ============================== */

typedef const char* trt_type_leafref;
static const char trd_type_leafref[] = "leafref";
typedef const char* trt_type_target_prefix;
static const char trd_type_target_prefix[] = "-> ";

typedef enum
{
    trd_type_type_empty,
    trd_type_type_target,
    trd_type_type_leafref,
} trt_type_type;

typedef struct
{
    trt_type_type id;
    const char* target_or_leafref;
} trt_type;

trt_type trp_empty_type();
bool trp_type_is_empty(trt_type);
void trp_print_type(trt_type, trt_print_func);
uint32_t trp_strlen_type(trt_type);

/* ==================================== */
/* ----------- <iffeatures> ----------- */
/* ==================================== */

typedef const char* trt_iffeatures_prefix;
static const char trd_iffeatures_prefix[] = "{";
typedef const char* trt_iffeatures_suffix;
static const char trd_iffeatures_suffix[] = "}?";
typedef bool trt_iffeature;

trt_iffeature trp_empty_iffeature();
bool trp_iffeature_is_empty(trt_iffeature);

/**
 * @brief Print all iffeatures of node 
 *
 * @param[in] print_iffeatures added function which finds and prints all iffeatures
 */
void trp_print_iffeatures(void (*print_iffeatures)(const struct trt_tree_ctx *), trt_print_func);
uint32_t trp_strlen_iffeatures(uint32_t(*strlen_iffeatures)(const struct trt_tree_ctx*));

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
    trt_opts opts;
    trt_type type;                  /**< is the name of the type for leafs and leaf-lists */
    trt_iffeature iffeatures;  
} trt_node;

trt_node trp_empty_node();
bool trp_node_is_empty(trt_node);

/* NOTE: If the node is a case node, there is no space before the <name> */
void trp_print_node(trt_node, struct trt_fp_print, trt_indent_in_node, trt_print_func);

uint32_t trp_strlen_node(trt_node, struct trt_fp_crawl);

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
    top,
    body,
} trt_keyword_stmt_type;

typedef struct
{
    trt_keyword_stmt_type id;
    trt_top_keyword keyword;
    const char* name;
} trt_keyword_stmt;

trt_keyword_stmt trp_empty_keyword_stmt();
bool trp_keyword_stmt_is_empty(trt_keyword_stmt);
void trp_print_keyword_stmt(trt_keyword_stmt, trt_print_func);

/* ================================= */
/* ----------- <Getters> ----------- */
/* ================================= */

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

/**
 * @brief Functions that provide the number of characters that can be printed
 */
struct trt_fp_crawl
{
    uint32_t (*strlen_features_names)(const struct trt_tree_ctx*);  /*<< count including spaces between names */
    uint32_t (*strlen_keys_names)(const struct trt_tree_ctx*);      /*<< count including spaces between names */
};

/**
 * @brief Functions that provide printing themselves
 */
struct trt_fp_print
{
    void (*print_features_names)(const struct trt_tree_ctx*);     /*<< print including spaces between names */
    void (*print_keys_names)(const struct trt_tree_ctx*);         /*<< print including spaces between names */
};


/**
 * @brief A set of all necessary functions that must be provided for the printer
 */
struct trt_fp_all
{
    struct trt_fp_modify_ctx modify;
    struct trt_fp_read read;
    struct trt_fp_crawl crawl;
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
    trt_print_func print;
    struct trt_fp_all fp;
    uint32_t max_line_length;
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
 * @brief Recursive nodes printing
 */
void trp_print_nodes(struct trt_printer_ctx, struct trt_tree_ctx*, trt_indent);

/**
 * @brief Print one line
 */
void trp_main_print_line(trt_node, trt_indent);

/**
 * @brief Find out if it is possible to unify the alignment in all subtrees
 *
 * The aim is to make it a little bit similar to two columns.
*/
trt_indent_in_node trp_main_try_unified_indent(struct trt_printer_ctx);

/* =================================== */
/* ----------- <separator> ----------- */
/* =================================== */

typedef const char* const trt_separator;
static trt_separator trd_separator_colon = ":";
static trt_separator trd_separator_space = " ";
static trt_separator trd_separator_dashes = "--";

/* ================================ */
/* ----------- <symbol> ----------- */
/* ================================ */

typedef const char* const trt_symbol;
static trt_symbol trd_symbol_sibling = "|";


#endif
