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
bool trp_indent_in_node_are_eq(trt_indent_in_node, trt_indent_in_node);

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
bool trt_wrapper_eq(trt_wrapper, trt_wrapper);

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
bool trp_node_name_is_empty(trt_node_name);
/** Print entire trt_node_name structure. */
void trp_print_node_name(trt_node_name, trt_printing);
/** Check if mark (?, !, *, /, @) is implicitly contained in trt_node_name. */
bool trp_mark_is_used(trt_node_name);

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
 * Opts keys is just bool because printing is not provided by the printer component (trp).
 */
typedef bool trt_opts_keys;

/** Create trt_opts_keys and note the presence of keys. */
trt_opts_keys trp_set_opts_keys();
/** Create empty trt_opts_keys and note the absence of keys. */
trt_opts_keys trp_empty_opts_keys();
/** Check if trt_opts_keys is empty. */
bool trp_opts_keys_is_empty(trt_opts_keys);

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
bool trp_type_is_empty(trt_type);
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
 * The iffeature is just bool because printing is not provided by the printer component (trp).
 */
typedef bool trt_iffeature;

/** Create trt_iffeature and note the presence of features. */
trt_iffeature trp_set_iffeature();
/** Create empty trt_iffeature and note the absence of features. */
trt_iffeature trp_empty_iffeature();
/** Check if trt_iffeature is empty. */
bool trp_iffeature_is_empty(trt_iffeature);

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

/** 
 * @brief <node> data for printing.
 *
 * <status>--<flags> <name><opts> <type> <if-features>.
 * Item <opts> is divided and moved to part trt_node_name (mark) and part trt_opts_keys (keys).
 * For printing trt_opts_keys and trt_iffeature is required special functions which prints them.
 */
typedef struct
{
    trt_status status;          /**< <status>. */
    trt_flags flags;            /**< <flags>. */
    trt_node_name name;         /**< <node> with <opts> mark. */
    trt_opts_keys opts_keys;    /**< <opts> list's keys. Printing function required. */
    trt_type type;              /**< <type> is the name of the type for leafs and leaf-lists. */
    trt_iffeature iffeatures;   /**< <if-features>. Printing function required. */
} trt_node;

/** Create trt_node as empty. */
trt_node trp_empty_node();
/** Check if trt_node is empty. */
bool trp_node_is_empty(trt_node);
/** Check if opts_keys, type and iffeatures are empty. */
bool trp_node_body_is_empty(trt_node);
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
bool trp_leafref_target_is_too_long(trt_node n, trt_wrapper wr, uint32_t mll);

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

typedef const char* trt_top_keyword;
static const char trd_top_keyword_module[] = "module";
static const char trd_top_keyword_submodule[] = "submodule";

typedef const char* trt_body_keyword;
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
    trd_keyword_stmt_top,   /**< Indicates the section with the keyword module. */
    trd_keyword_stmt_body,  /**< Indicates the section with the keyword e.g. augment, grouping.*/
} trt_keyword_stmt_type;

/**
 * @brief Main sign of the tree nodes.
 */
typedef struct
{
    trt_keyword_stmt_type type; /**< Type of the keyword_stmt. */
    trt_top_keyword keyword;    /**< String containing some of the top or body keyword. */
    const char* str;            /**< Name or path, it determines the type. */
} trt_keyword_stmt;

trt_keyword_stmt trp_empty_keyword_stmt();
bool trp_keyword_stmt_is_empty(trt_keyword_stmt);
void trt_print_keyword_stmt_begin(trt_keyword_stmt, trt_printing);
void trt_print_keyword_stmt_str(trt_keyword_stmt, uint32_t mll, trt_printing);
void trt_print_keyword_stmt_end(trt_keyword_stmt, trt_printing);
void trp_print_keyword_stmt(trt_keyword_stmt, uint32_t mll, trt_printing);

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

bool trg_test_bit(uint64_t number, uint32_t bit);

void trg_print_linebreak(trt_printing);

const char* trg_print_substr(const char*, size_t len, trt_printing);


/* ================================ */
/* ----------- <symbol> ----------- */
/* ================================ */

typedef const char* const trt_symbol;
static trt_symbol trd_symbol_sibling = "|";


#endif
