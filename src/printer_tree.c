
/**
 * @file printer_tree.c
 * @author Adam Piecek <piecek@cesnet.cz>
 * @brief RFC tree printer for libyang data structure
 *
 * Copyright (c) 2015 - 2020 CESNET, z.s.p.o.
 *
 * This source code is licensed under BSD 3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://opensource.org/licenses/BSD-3-Clause
 */
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "common.h"
#include "context.h"
#include "dict.h"
#include "log.h"
#include "parser_data.h"
#include "plugins_types.h"
#include "printer_internal.h"
#include "tree.h"
#include "tree_schema.h"
#include "tree_schema_internal.h"


/* -######-- Declarations start -#######- */

/*
 *          +---------+    +---------+    +---------+
 *   output |         |    |         |    |         |
 *      <---+   trp   +<---+   trb   +<-->+   tro   |
 *          |         |    |         |    |         |
 *          +---------+    +----+----+    +---------+
 *                              ^
 *                              |
 *                         +----+----+
 *                         |         |
 *                         |   trm   |
 *                         |         |
 *                         +----+----+
 *                              ^
 *                              | input
 *                              +
 */   


/* Glossary:
 * trt - type
 * trp - functions for Printing
 * trb - functions for Browse the tree
 * tro - functions for Obtaining information from libyang 
 * trm - Main functions
 * trg - General functions
 */


struct trt_tree_ctx;
struct trt_getters;
struct trt_fp_modify_ctx;
struct trt_fp_read;
struct trt_fp_crawl;
struct trt_fp_print;

/**
 * @brief Last layer for printing.
 *
 * Variadic arguments are expected to be of the type char*.
 *
 * @param[in,out] out struct ly_out* or other auxiliary structure for printing.
 * @param[in] arg_count number of arguments in va_list.
 * @param[in] ap variadic argument list from <stdarg.h>.
 */
typedef void (*trt_print_func)(void *out, int arg_count, va_list ap);


/**
 * @brief Customizable structure for printing on any output.
 *
 * Structures trt_printing causes printing and trt_injecting_strlen does not print but is used for character counting.
 */
typedef struct
{
    void* out;          /**< Pointer to output data. Typical ly_out* or any c++ container in the case of testing. */
    trt_print_func pf;  /**< Pointer to function which takes void* out and do typically printing actions. */
} trt_printing,
  trt_injecting_strlen;

/**
 * @brief Print variadic number of char* pointers.
 *
 * @param[in] p struct ly_out* or other auxiliary structure for printing.
 * @param[in] arg_count number of arguments in va_list.
 */
void trp_print(trt_printing p, int arg_count, ...);

/**
 * @brief Callback functions that print themselves without printer overhead
 *
 * This includes strings that cannot be printed using the char * pointer alone.
 * Instead, pieces of string are distributed in multiple locations in memory, so a function is needed to find and print them.
 *
 * Structures trt_cf_print_keys is for print a list's keys and trt_cf_print_iffeatures for list of features.
 */
typedef struct 
{
    const struct trt_tree_ctx* ctx;                         /**< Context of libyang tree. */
    void (*pf)(const struct trt_tree_ctx *, trt_printing);  /**< Pointing to definition of printing e.g. keys or features. */
} trt_cf_print_keys,
  trt_cf_print_iffeatures;

typedef uint32_t trt_printer_opts;

/**
 * @brief Used for counting characters. Used in trt_injecting_strlen as void* out.
 */
typedef struct
{
    uint32_t bytes;
} trt_counter;

/**
 * @brief Counts the characters to be printed instead of printing.
 *
 * Used in trt_injecting_strlen as trt_print_func.
 *
 * @param[in,out] out it is expected to be type of trt_counter.
 * @param[in] arg_count number of arguments in va_list.
 */
void trp_injected_strlen(void *out, int arg_count, va_list ap); 

/* ======================================= */
/* ----------- <Print getters> ----------- */
/* ======================================= */

/**
 * @brief Functions that provide printing themselves
 *
 * Functions must print including spaces or delimiters between names.
 */
struct trt_fp_print
{
    void (*print_features_names)(const struct trt_tree_ctx*, trt_printing);   /**< Print list of features. */
    void (*print_keys)(const struct trt_tree_ctx *, trt_printing);            /**< Print  list's keys. */
};

/**
 * @brief Package which only groups getter function.
 */
typedef struct
{
    const struct trt_tree_ctx* tree_ctx;    /**< Context of libyang tree. */
    struct trt_fp_print fps;                /**< Print function. */
} trt_pck_print;

/* ================================ */
/* ----------- <indent> ----------- */
/* ================================ */

/**
 * @brief Constants which are defined in the RFC or are observable from the pyang tool.
 */
typedef enum
{
    trd_indent_empty = 0,               /**< If the node is a case node, there is no space before the <name>. */
    trd_indent_long_line_break = 2,     /**< The new line should be indented so that it starts below <name> with a whitespace offset of at least two characters. */
    trd_indent_line_begin = 2,          /**< Indent below the keyword (module, augment ...).  */
    trd_indent_btw_siblings = 2,        /**< Between | and | characters. */
    trd_indent_before_keys = 1,         /**< <x>___<keys>. */
    trd_indent_before_type = 4,         /**< <x>___<type>, but if mark is set then indent == 3. */
    trd_indent_before_iffeatures = 1,   /**< <x>___<iffeatures>. */
} trt_cnf_indent;


/**
 * @brief Type of indent in node.
 */
typedef enum
{
    trd_indent_in_node_normal = 0,  /**< Node fits on one line. */
    trd_indent_in_node_unified,     /**< Alignment for sibling nodes is common. */
    trd_indent_in_node_divided,     /**< The node must be split into multiple rows. */
    trd_indent_in_node_failed       /**< Cannot be crammed into one line. The condition for the maximum line length is violated. */
} trt_indent_in_node_type;

/** Recording the number of gaps. */
typedef int16_t trt_indent_btw;

/** Constant to indicate the need to break a line. */
const trt_indent_btw trd_linebreak = -1;

/**
 * @brief Records the alignment between the individual elements of the node.
 */
typedef struct
{
    trt_indent_in_node_type type;       /**< Type of indent in node. */
    trt_indent_btw btw_name_opts;       /**< Indent between node name and opts. */
    trt_indent_btw btw_opts_type;       /**< Indent between opts and type. */
    trt_indent_btw btw_type_iffeatures; /**< Indent between type and features. Ignored if <type> missing. */
} trt_indent_in_node;

/** Create trt_indent_in_node as empty. */
trt_indent_in_node trp_empty_indent_in_node();
/** Check that they can be considered equivalent. */
ly_bool trp_indent_in_node_are_eq(trt_indent_in_node, trt_indent_in_node);

/**
 * @brief Writing a linebreak constant.
 *
 * The order where the linebreak tag can be placed is from the end.
 *
 * @param[in] item containing alignment lengths or already line break marks
 * @return with a newly placed linebreak tag.
 * @return .type = trd_indent_in_node_failed if it is not possible to place a more line breaks.
 */
trt_indent_in_node trp_indent_in_node_place_break(trt_indent_in_node item);

/**
 * @brief Type of wrappers to be printed.
 */
typedef enum 
{
    trd_wrapper_top = 0,    /**< Related to the module. */
    trd_wrapper_body        /**< Related to e.g. Augmentations or Groupings */
} trd_wrapper_type;

/**
 * @brief For resolving sibling symbol placement.
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

/** Get wrapper related to the module. */
trt_wrapper trp_init_wrapper_top();
/** Get wrapper related to e.g. Augmenations or Groupings. */
trt_wrapper trp_init_wrapper_body();

/** Setting mark in .bit_marks at position .actual_pos */
trt_wrapper trp_wrapper_set_mark(trt_wrapper);

/** Set shift to the right (next sibling symbol position). */
trt_wrapper trp_wrapper_set_shift(trt_wrapper);

/** Test if they are equivalent. */
ly_bool trt_wrapper_eq(trt_wrapper, trt_wrapper);

/** Print "  |" sequence. */
void trp_print_wrapper(trt_wrapper, trt_printing);


/**
 * @brief Package which only groups wrapper and indent in node.
 */
typedef struct
{
    trt_wrapper wrapper;            /**< "  |  |" sequence. */
    trt_indent_in_node in_node;     /**< Indent in node. */
} trt_pck_indent;

/* ================================== */
/* ------------ <status> ------------ */
/* ================================== */

static const char trd_status_current[] = "+";
static const char trd_status_deprecated[] = "x";
static const char trd_status_obsolete[] = "o";

typedef enum
{
    trd_status_type_empty = 0,
    trd_status_type_current,
    trd_status_type_deprecated,
    trd_status_type_obsolete,
} trt_status_type;

void trp_print_status(trt_status_type, trt_printing);

/* ================================== */
/* ------------ <flags> ------------- */
/* ================================== */

static const char trd_flags_rw[] = "rw";
static const char trd_flags_ro[] = "ro";
static const char trd_flags_rpc_input_params[] = "-w";
static const char trd_flags_uses_of_grouping[] = "-u";
static const char trd_flags_rpc[] = "-x";
static const char trd_flags_notif[] = "-n";
static const char trd_flags_mount_point[] = "mp";

typedef enum
{
    trd_flags_type_empty = 0,
    trd_flags_type_rw,
    trd_flags_type_ro,
    trd_flags_type_rpc_input_params,
    trd_flags_type_uses_of_grouping,
    trd_flags_type_rpc,
    trd_flags_type_notif,
    trd_flags_type_mount_point,
} trt_flags_type;

void trp_print_flags(trt_flags_type, trt_printing);
size_t trp_print_flags_strlen(trt_flags_type);

/* ================================== */
/* ----------- <node_name> ----------- */
/* ================================== */

typedef const char* trt_node_name_prefix;
static const char trd_node_name_prefix_choice[] = "(";
static const char trd_node_name_prefix_case[] = ":(";

typedef const char* trt_node_name_suffix;
static const char trd_node_name_suffix_choice[] = ")";
static const char trd_node_name_suffix_case[] = ")";

/**
 * @brief Type of the node.
 */
typedef enum
{
    trd_node_else = 0,              /**< For some node which does not require special treatment. */
    trd_node_case,                  /**< For case node. */
    trd_node_choice,                /**< For choice node. */
    trd_node_optional_choice,       /**< For choice node with optional mark (?). */
    trd_node_optional,              /**< For an optional leaf, anydata, or anyxml. */
    trd_node_container,             /**< For a presence container. */
    trd_node_listLeaflist,          /**< For a leaf-list or list (without keys). */
    trd_node_keys,                  /**< For a list's keys. */
    trd_node_top_level1,            /**< For a top-level data node in a mounted module. */
    trd_node_top_level2             /**< For a top-level data node of a module identified in a mount point parent reference. */
} trt_node_type;


/**
 * @brief Type of node and his name.
 */
typedef struct
{
    trt_node_type type;         /**< Type of the node relevant for printing. */
    const char* module_prefix;  /**< Prefix defined in the module where the node is defined. */
    const char* str;            /**< Name of the node. */
} trt_node_name;


/** Create trt_node_name as empty. */
trt_node_name trp_empty_node_name();
/** Check if trt_node_name is empty. */
ly_bool trp_node_name_is_empty(trt_node_name);
/** Print entire trt_node_name structure. */
void trp_print_node_name(trt_node_name, trt_printing);
/** Check if mark (?, !, *, /, @) is implicitly contained in trt_node_name. */
ly_bool trp_mark_is_used(trt_node_name);

/* ============================== */
/* ----------- <opts> ----------- */
/* ============================== */

static const char trd_opts_optional[] = "?";        /**< For an optional leaf, choice, anydata, or anyxml. */
static const char trd_opts_container[] = "!";       /**< For a presence container. */
static const char trd_opts_list[] = "*";            /**< For a leaf-list or list. */
static const char trd_opts_slash[] = "/";           /**< For a top-level data node in a mounted module. */
static const char trd_opts_at_sign[] = "@";         /**< For a top-level data node of a module identified in a mount point parent reference. */
static const size_t trd_opts_mark_length = 1;       /**< Every opts mark has a length of one. */

typedef const char* trt_opts_keys_prefix;
static const char trd_opts_keys_prefix[] = "[";
typedef const char* trt_opts_keys_suffix;
static const char trd_opts_keys_suffix[] = "]";

/** 
 * @brief Opts keys in node.
 *
 * Opts keys is just ly_bool because printing is not provided by the printer component (trp).
 */
typedef ly_bool trt_opts_keys;

/** Create trt_opts_keys and note the presence of keys. */
trt_opts_keys trp_set_opts_keys();
/** Create empty trt_opts_keys and note the absence of keys. */
trt_opts_keys trp_empty_opts_keys();
/** Check if trt_opts_keys is empty. */
ly_bool trp_opts_keys_is_empty(trt_opts_keys);

/** 
 * @brief Print opts keys.
 *
 * @param[in] k flag if keys is present.
 * @param[in] ind number of spaces between name and [keys].
 * @param[in] pf basically a pointer to the function that prints the keys.
 * @param[in] p basically a pointer to a function that handles the printing itself.
 */
void trp_print_opts_keys(trt_opts_keys k, trt_indent_btw ind, trt_cf_print_keys pf, trt_printing p);

/* ============================== */
/* ----------- <type> ----------- */
/* ============================== */

typedef const char* trt_type_leafref;
static const char trd_type_leafref_keyword[] = "leafref";
typedef const char* trt_type_target_prefix;
static const char trd_type_target_prefix[] = "-> ";

/** 
 * @brief Type of the <type>
 */
typedef enum
{
    trd_type_name = 0,  /**< Type is just a name that does not require special treatment. */
    trd_type_target,    /**< Should have a form "-> TARGET", where TARGET is the leafref path. */
    trd_type_leafref,   /**< This type is set automatically by the algorithm. So set type as trd_type_target. */
    trd_type_empty      /**< Type is not used at all. */
} trt_type_type;


/** 
 * @brief <type> in the <node>.
 */
typedef struct
{
    trt_type_type type; /**< Type of the <type>. */
    const char* str;    /**< Path or name of the type. */
} trt_type;

/** Create empty trt_type. */
trt_type trp_empty_type();
/** Check if trt_type is empty. */
ly_bool trp_type_is_empty(trt_type);
/** Print entire trt_type structure. */
void trp_print_type(trt_type, trt_printing);

/* ==================================== */
/* ----------- <iffeatures> ----------- */
/* ==================================== */

typedef const char* trt_iffeatures_prefix;
static const char trd_iffeatures_prefix[] = "{";
typedef const char* trt_iffeatures_suffix;
static const char trd_iffeatures_suffix[] = "}?";

/** 
 * @brief List of features in node.
 *
 * The iffeature is just ly_bool because printing is not provided by the printer component (trp).
 */
typedef ly_bool trt_iffeature;

/** Create trt_iffeature and note the presence of features. */
trt_iffeature trp_set_iffeature();
/** Create empty trt_iffeature and note the absence of features. */
trt_iffeature trp_empty_iffeature();
/** Check if trt_iffeature is empty. */
ly_bool trp_iffeature_is_empty(trt_iffeature);

/**
 * @brief Print all iffeatures of node 
 *
 * @param[in] i flag if keys is present.
 * @param[in] pf basically a pointer to the function that prints the list of features.
 * @param[in] p basically a pointer to a function that handles the printing itself.
 */
void trp_print_iffeatures(trt_iffeature i, trt_cf_print_iffeatures pf, trt_printing p);

/* ============================== */
/* ----------- <node> ----------- */
/* ============================== */

/** 
 * @brief <node> data for printing.
 *
 * <status>--<flags> <name><opts> <type> <if-features>.
 * Item <opts> is divided and moved to part trt_node_name (mark) and part trt_opts_keys (keys).
 * For printing trt_opts_keys and trt_iffeature is required special functions which prints them.
 */
typedef struct
{
    trt_status_type status;          /**< <status>. */
    trt_flags_type flags;            /**< <flags>. */
    trt_node_name name;         /**< <node> with <opts> mark. */
    trt_opts_keys opts_keys;    /**< <opts> list's keys. Printing function required. */
    trt_type type;              /**< <type> is the name of the type for leafs and leaf-lists. */
    trt_iffeature iffeatures;   /**< <if-features>. Printing function required. */
} trt_node;

/** Create trt_node as empty. */
trt_node trp_empty_node();
/** Check if trt_node is empty. */
ly_bool trp_node_is_empty(trt_node);
/** Check if opts_keys, type and iffeatures are empty. */
ly_bool trp_node_body_is_empty(trt_node);
/** Print just <status>--<flags> <name> with opts mark. */
void trp_print_node_up_to_name(trt_node, trt_printing);
/** Print alignment (spaces) instead of <status>--<flags> <name> for divided node. */
void trp_print_divided_node_up_to_name(trt_node, trt_printing);

/**
 * @brief Print trt_node structure.
 *
 * @param[in] n node structure for printing.
 * @param[in] ppck package of functions for printing opts_keys and iffeatures.
 * @param[in] ind indent in node.
 * @param[in] p basically a pointer to a function that handles the printing itself.
 */
void trp_print_node(trt_node n, trt_pck_print ppck, trt_indent_in_node ind, trt_printing p);

/**
 * @brief Check if leafref target must be change to string 'leafref' because his target string is too long.
 *
 * @param[in] n node containing leafref target.
 * @param[in] wr for node immersion depth.
 * @param[in] mll max line length border.
 * @return true if must be change to string 'leafref'.
 */
ly_bool trp_leafref_target_is_too_long(trt_node n, trt_wrapper wr, uint32_t mll);

/**
 * @brief Package which only groups indent and node.
 */
typedef struct
{
    trt_indent_in_node indent;
    trt_node node;
}trt_pair_indent_node;

/**
 * @brief Get the first half of the node based on the linebreak mark.
 *
 * Items in the second half of the node will be empty.
 *
 * @param[in] node the whole <node> to be split.
 * @param[in] ind contains information in which part of the <node> the first half ends.
 * @return first half of the node, indent is unchanged.
 */
trt_pair_indent_node trp_first_half_node(trt_node node, trt_indent_in_node ind);

/**
 * @brief Get the second half of the node based on the linebreak mark.
 *
 * Items in the first half of the node will be empty.
 * Indentations belonging to the first node will be reset to zero.
 *
 * @param[in] node the whole <node> to be split.
 * @param[in] ind contains information in which part of the <node> the second half starts.
 * @return second half of the node, indent is newly set.
 */
trt_pair_indent_node trp_second_half_node(trt_node node, trt_indent_in_node ind);

/* =================================== */
/* ----------- <statement> ----------- */
/* =================================== */

static const char trd_top_keyword_module[] = "module";
static const char trd_top_keyword_submodule[] = "submodule";

static const char trd_body_keyword_augment[] = "augment";
static const char trd_body_keyword_rpc[] = "rpcs";
static const char trd_body_keyword_notif[] = "notifications";
static const char trd_body_keyword_grouping[] = "grouping";
static const char trd_body_keyword_yang_data[] = "yang-data";

/**
 * @brief Type of the trt_keyword_stmt.
 */
typedef enum
{
    trd_keyword_stmt_top = 0,   /**< Indicates the section with the keyword module. */
    trd_keyword_stmt_body,      /**< Indicates the section with the keyword e.g. augment, grouping.*/
} trt_keyword_stmt_type;

/**
 * @brief Type of the trt_keyword.
 */
typedef enum 
{
    trd_keyword_module = 0,     /**< Used when trd_keyword_stmt_top is set. */
    trd_keyword_submodule,      /**< Used when trd_keyword_stmt_top is set. */
    trd_keyword_augment,        /**< Used when trd_keyword_stmt_body is set. */
    trd_keyword_rpc,            /**< Used when trd_keyword_stmt_body is set. */
    trd_keyword_notif,          /**< Used when trd_keyword_stmt_body is set. */
    trd_keyword_grouping,       /**< Used when trd_keyword_stmt_body is set. */
    trd_keyword_yang_data       /**< Used when trd_keyword_stmt_body is set. */
} trt_keyword_type;

/**
 * @brief Main sign of the tree nodes.
 */
typedef struct
{
    trt_keyword_stmt_type type; /**< Type of the keyword_stmt. */
    trt_keyword_type keyword;    /**< String containing some of the top or body keyword. */
    const char* str;            /**< Name or path, it determines the type. */
} trt_keyword_stmt;

trt_keyword_stmt trp_empty_keyword_stmt();
ly_bool trp_keyword_stmt_is_empty(trt_keyword_stmt);
void trt_print_keyword_stmt_begin(trt_keyword_stmt, trt_printing);
void trt_print_keyword_stmt_str(trt_keyword_stmt, uint32_t mll, trt_printing);
void trt_print_keyword_stmt_end(trt_keyword_stmt, trt_printing);
void trp_print_keyword_stmt(trt_keyword_stmt, uint32_t mll, trt_printing);
size_t trp_keyword_type_strlen(trt_keyword_type);

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
/* --------- <Main trp functions> --------- */
/* ======================================== */

/**
 * @brief Execute Printer - print tree
 */
void trp_main(struct trt_printer_ctx, struct trt_tree_ctx*);

/**
 * @brief Print one line
 */
void trp_print_line(trt_node, trt_pck_print, trt_pck_indent, trt_printing);

void trp_print_line_up_to_node_name(trt_node, trt_wrapper, trt_printing);

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
static trt_separator trd_separator_slash = "/";
static trt_separator trd_separator_linebreak = "\n";

void trg_print_n_times(int32_t n, char, trt_printing);

ly_bool trg_test_bit(uint64_t number, uint32_t bit);

void trg_print_linebreak(trt_printing);

const char* trg_print_substr(const char*, size_t len, trt_printing);


/* ================================ */
/* ----------- <symbol> ----------- */
/* ================================ */

typedef const char* const trt_symbol;
static trt_symbol trd_symbol_sibling = "|";

/* ======================================== */
/* ---------- <Module interface> ---------- */
/* ======================================== */


LY_ERR tree_print_parsed_and_compiled_module(struct ly_out *out, const struct lys_module *module, uint32_t options, size_t line_length);

LY_ERR tree_print_submodule(struct ly_out *out, const struct lys_module *module, const struct lysp_submodule *submodp, uint32_t options, size_t line_length);

LY_ERR tree_print_compiled_node(struct ly_out *out, const struct lysc_node *node, uint32_t options, size_t line_length);

/* -######-- Declarations end -#######- */
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
    return wr;
}

trt_wrapper
trp_wrapper_set_shift(trt_wrapper wr)
{
    /* +--<node>
     * |  +--<node>
     */
    wr.actual_pos++;
    return wr;
}

ly_bool
trt_wrapper_eq(trt_wrapper f, trt_wrapper s)
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

    if(trt_wrapper_eq(wr, trp_init_wrapper_top()))
        return;

    {
        uint32_t lb;
        if (wr.type == trd_wrapper_top) {
          lb = trd_indent_line_begin;
        } else if (wr.type == trd_wrapper_body) {
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
    trg_print_n_times(trd_indent_btw_siblings, trd_separator_space[0], p); 
    trp_print_node(node, pck, ind.in_node, p);
}

void
trp_print_line_up_to_node_name(trt_node node, trt_wrapper wr, trt_printing p)
{
    trp_print_wrapper(wr, p);
    trg_print_n_times(trd_indent_btw_siblings, trd_separator_space[0], p); 
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
