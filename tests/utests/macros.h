/**
 * @file   macros.h
 * @author Radek Iša <isa@cesnet.cz>
 * @brief  this file contains macros for simplification test writing
 *
 * Copyright (c) 2020 CESNET, z.s.p.o.
 *
 * This source code is licensed under BSD 3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef _LY_UTEST_MACROS_H_
#define _LY_UTEST_MACROS_H_

// cmocka header files
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

//local header files
#include "libyang.h"
#include "tests/config.h"

//macros

/**
 * @brief get actual context. In test macros is context hiden. 
 */
#define CONTEXT_GET \
                        ly_context

/**
 * @brief Create actual context
 * @param[in] PATH Directory where libyang will search for the imported or included modules and submodules. If no such directory is available, NULL is accepted.
 */
#define CONTEXT_CREATE_PATH(PATH) \
                        struct ly_ctx *ly_context;\
                        assert_int_equal(LY_SUCCESS, ly_ctx_new(PATH, 0, &ly_context))\

/**
 * @brief destroy context 
 */
#define CONTEXT_DESTROY \
                        ly_ctx_destroy(ly_context, NULL)


/**
 * @brief Parse (and validate) data from the input handler as a YANG data tree.
 *
 * @param[in] INPUT The input data in the specified @p format to parse (and validate) 
 * @param[in] INPUT_FORMAT Format of the input data to be parsed. Can be 0 to try to detect format from the input handler.
 * @param[in] PARSE_OPTIONS Options for parser, see @ref dataparseroptions.
 * @param[in] VALIDATE_OPTIONS Options for the validation phase, see @ref datavalidationoptions.
 * @param[in] OUT_STATUS expected return status
 * @param[in] ERR_MSG    check error or warning message. (NOT IMPLEMENTED YET)
 * @param[out] OUT_NODE Resulting data tree built from the input data. Note that NULL can be a valid result as a representation of an empty YANG data tree.
 * The returned data are expected to be freed using LYD_NODE_DESTROY().
 */
#define LYD_NODE_CREATE_PARAM(INPUT, INPUT_FORMAT, PARSE_OPTIONS, VALIDATE_OPTIONS, OUT_STATUS, ERR_MSG, OUT_NODE) \
                        assert_int_equal(OUT_STATUS, lyd_parse_data_mem(ly_context, INPUT, INPUT_FORMAT, PARSE_OPTIONS, VALIDATE_OPTIONS, & OUT_NODE));\
                        if (OUT_STATUS ==  LY_SUCCESS) { \
                            assert_non_null(OUT_NODE);\
                        } \
                        else { \
                            assert_null(OUT_NODE);\
                        }\
                        /*assert_string_equal(ERR_MSG, );*/

/**
 * @breaf free lyd_node
 * @param[in] NODE pointer to lyd_node
 */
#define LYD_NODE_DESTROY(NODE) \
                        lyd_free_all(NODE);

/**
 * @breaf Check if lyd_node and his subnodes have correct values. Print lyd_node and his sunodes int o string in json or xml format. 
 * @param[in] NODE pointer to lyd_node
 * @param[in] TEXT expected output string in json or xml format.
 * @param[in] FORMAT format of input text. LYD_JSON, LYD_XML
 * @param[in] PARAM  options [Data printer flags](@ref dataprinterflags).
 */
#define LYD_NODE_CHECK_CHAR_PARAM(NODE, TEXT, FORMAT, PARAM) \
                         {\
                             char * test;\
                             lyd_print_mem(&test, NODE, FORMAT, PARAM);\
                             assert_string_equal(test, TEXT);\
                             free(test);\
                         }


/**
 * @breaf Compare two lyd_node structure. Macro print lyd_node structure into string and then compare string. Print function use these two parameters. LYD_PRINT_WITHSIBLINGS | LYD_PRINT_SHRINK;
 * @param[in] NODE_1 pointer to lyd_node
 * @param[in] NODE_2 pointer to lyd_node
 */
#define LYD_NODE_CHECK(NODE_1, NODE_2) \
                         {\
                             char * test_1;\
                             char * test_2;\
                             lyd_print_mem(&test_1, NODE_1, LYD_XML, LYD_PRINT_WITHSIBLINGS | LYD_PRINT_SHRINK);\
                             lyd_print_mem(&test_2, NODE_2, LYD_XML, LYD_PRINT_WITHSIBLINGS | LYD_PRINT_SHRINK);\
                             assert_string_equal(test_1, test_2);\
                             free(test_1);\
                             free(test_2);\
                         }


/**
 * @breaf change lyd_node to his root node.
 * @param[inout] NODE pointer to lyd_node
 */
#define ROOT_GET(NODE) \
                        if (NODE) { \
                            while ((NODE)->parent) { \
                                NODE = (struct lyd_node *)(NODE)->parent;\
                            }\
                            while ((NODE)->prev->next) {\
                                NODE = (NODE)->prev;\
                            }\
                        }\


/*
    SUPPORT MACROS 
*/

/**
 * @brief Macro check description in node 
 * 
 * @param[in] STRING input text 
 * @param[in] TEXT   expected text 
 */

#define assert_string(STRING, TEXT)\
                if (TEXT == NULL){\
                    assert_null(STRING);\
                }\
                else {\
                    assert_non_null(STRING);\
                    assert_string_equal(STRING, TEXT);\
                }\

/**
 * @brief Macro check if pointer 
 * 
 * @param[in] POINTER pointer to some variable 
 * @param[in] FLAG    0 -> pointer is NULL, 1 -> pointer is not null
 */
#define assert_pointer(POINTER, FLAG)\
                assert_true(FLAG == 0 ? POINTER == NULL : POINTER != NULL)

/**
 * @brief Macro check size of size array 
 * 
 * @param[in] POINTER pointer to sized array 
 * @param[in] SIZE    array size 
 */
#define assert_array(ARRAY, SIZE)\
                assert_true((SIZE == 0) ? \
                            (ARRAY == NULL) : \
                            (ARRAY != NULL && SIZE == LY_ARRAY_COUNT(ARRAY)));\

/*
    LIBYANG NODE CHECKING
*/


/**
 * @brief Macro check if lysp_action_inout value is correct
 * @param[in] NODE pointer to lysp_when variable
 * @param[in] DATA 0 -> pointer is NULL, 1 -> pointer is not null
 * @param[in] EXTS    size of list of extension array 
 * @param[in] GROUPINGS size of list of grouping
 * @param[in] MUSTS    size of list of must restriction
 * @param[in] NODETYPE node type
 * @param[in] PARENT 0 -> pointer is NULL, 1 -> pointer is not null
 * @param[in] TYPEDEFS size of list of typedefs
 */
#define LYSP_ACTION_INOUT_CHECK(NODE, DATA, EXTS, GROUPINGS, MUSTS, NODETYPE, PARENT, TYPEDEFS)\
                assert_non_null(NODE);\
                assert_pointer((NODE)->data, DATA);\
                assert_array((NODE)->exts, EXTS);\
                assert_array((NODE)->groupings, GROUPINGS);\
                assert_array((NODE)->musts, MUSTS);\
                assert_int_equal((NODE)->nodetype, NODETYPE);\
                assert_pointer((NODE)->parent, PARENT);\
                assert_array((NODE)->typedefs, TYPEDEFS);

/**
 * @brief Macro check if lysp_action_inout value is correct
 * @param[in] NODE pointer to lysp_when variable
 * @param[in] DSC  string expected description
 * @param[in] EXTS    size of list of extension array 
 * @param[in] FLAGS   flags
 * @param[in] GROUPINGS size of list of grouping
 * @param[in] IFFEATURES size of list of if-feature expressions
 * @param[in] INPUT_*    LYSP_ACTION_INOUT_CHECK
 * @param[in] NAME     string reprezenting node name
 * @param[in] NODETYPE node type
 * @param[in] OUTPUT_*    LYSP_ACTION_INOUT_CHECK
 * @param[in] PARENT 0 -> pointer is NULL, 1 -> pointer is not null
 * @param[in] REF      string reprezenting reference
 * @param[in] TYPEDEFS size of list of typedefs
 */
#define LYSP_ACTION_CHECK(NODE, DSC, EXTS, FLAGS, GROUPINGS, IFFEATURES,\
                        INPUT_DATA, INPUT_EXTS, INPUT_GROUPINGS, INPUT_MUSTS,\
                        INPUT_PARENT, INPUT_TYPEDEFS,\
                        NAME, NODETYPE, \
                        OUTPUT_DATA, OUTPUT_EXTS, OUTPUT_GROUPINGS, OUTPUT_MUSTS,\
                        OUTPUT_PARENT, OUTPUT_TYPEDEFS,\
                        PARENT, REF, TYPEDEFS)\
                assert_non_null(NODE);\
                assert_string((NODE)->dsc, DSC);\
                assert_array((NODE)->exts, EXTS);\
                assert_int_equal((NODE)->flags, FLAGS);\
                assert_array((NODE)->groupings, GROUPINGS);\
                assert_array((NODE)->iffeatures, IFFEATURES);\
                LYSP_ACTION_INOUT_CHECK(&((NODE)->input), INPUT_DATA, INPUT_EXTS, INPUT_GROUPINGS,\
                            INPUT_MUSTS, LYS_INPUT, INPUT_PARENT, INPUT_TYPEDEFS);\
                assert_string_equal((NODE)->name, NAME);\
                assert_int_equal((NODE)->flags, FLAGS);\
                LYSP_ACTION_INOUT_CHECK(&((NODE)->output), OUTPUT_DATA, OUTPUT_EXTS, OUTPUT_GROUPINGS,\
                            OUTPUT_MUSTS, LYS_OUTPUT, OUTPUT_PARENT, OUTPUT_TYPEDEFS);\
                assert_pointer((NODE)->parent, PARENT);\
                assert_string((NODE)->ref, REF);\
                assert_array((NODE)->typedefs, TYPEDEFS)\
                




/**
 * @brief Macro check if lysp_when value is correct
 * @param[in] NODE pointer to lysp_when variable
 * @param[in] COND string specifid condition
 * @param[in] DSC  description or NULL
 * @param[in] EXTS    size of list of extension array 
 * @param[in] REF     string reference
 */
#define LYSP_WHEN_CHECK(NODE, COND, DSC, EXTS, REF)\
                assert_non_null(NODE);\
                assert_string_equal((NODE)->cond, COND);\
                assert_string((NODE)->dsc, DSC);\
                assert_array((NODE)->exts, EXTS);\
                if (REF == NULL){\
                    assert_null((NODE)->ref);\
                }\
                else {\
                    assert_non_null((NODE)->ref);\
                    assert_string_equal((NODE)->ref, REF);\
                }               


/**
 * @brief Macro check if lysp_restr value is correct
 * @param[in] NODE pointer to lysp_restr variable
 * @param[in] ARG_STR string reprezenting
 * @param[in] DSC  description or NULL
 * @param[in] EAPPTAG string reprezenting error-app-tag value 
 * @param[in] EMSG    string reprezenting error message
 * @param[in] EXTS    size of list of extension array 
 * @param[in] REF     string reference
 */

#define LYSP_RESTR_CHECK(NODE, ARG_STR, DSC, EAPPTAG, EMSG, EXTS, REF)\
                assert_non_null(NODE);\
                assert_non_null((NODE)->arg.mod);\
                assert_string_equal((NODE)->arg.str, ARG_STR);\
                if (DSC == NULL){\
                    assert_null((NODE)->dsc);\
                }\
                else {\
                    assert_non_null((NODE)->dsc);\
                    assert_string_equal((NODE)->dsc, DSC);\
                }\
                if (EAPPTAG == NULL){\
                    assert_null((NODE)->eapptag);\
                }\
                else {\
                    assert_non_null((NODE)->eapptag);\
                    assert_string_equal((NODE)->eapptag, EAPPTAG);\
                }\
                if (EMSG == NULL){\
                    assert_null((NODE)->emsg);\
                }\
                else {\
                    assert_non_null((NODE)->emsg);\
                    assert_string_equal((NODE)->emsg, EMSG);\
                }\
                assert_array((NODE)->exts, EXTS);\
                if (REF == NULL){\
                    assert_null((NODE)->ref);\
                }\
                else {\
                    assert_non_null((NODE)->ref);\
                    assert_string_equal((NODE)->ref, REF);\
                }\

/**
 * @brief Macro check if lysp_import value is correct
 * @param[in] NODE pointer to lysp_import variable
 * @param[in] DSC  description or NULL
 * @param[in] EXTS size of list of extensions
 * @param[in] NAME string name of imported module
 * @param[in] PREFIX string prefix for the data from the imported schema
 * @param[in] REF    string reference
 * @prame[in] REV    string reprezenting date in format "11-10-2020"
 */
#define LYSP_IMPORT_CHECK(NODE, DSC, EXTS, NAME, PREFIX, REF, REV)\
                assert_non_null(NODE);\
                if (DSC == NULL){\
                    assert_null((NODE)->dsc);\
                }\
                else {\
                    assert_string_equal((NODE)->dsc, DSC);\
                }\
                assert_array((NODE)->exts, EXTS);\
                /*assert_non_null((NODE)->module); // ?? it is mandatory but ... */\
                assert_string_equal((NODE)->name, NAME);\
                assert_string_equal((NODE)->prefix, PREFIX);\
                if (REF == NULL){\
                    assert_null((NODE)->ref);\
                }\
                else {\
                    assert_string_equal((NODE)->ref, REF);\
                }\
                assert_string_equal((NODE)->rev, REV)


/**
 * @brief Macro check if lysp_stmt value is correct 
 * 
 * @param[in] NODE pointer to lysp_ext_instance variable
 * @param[in] ARGUMENT string optional value of the extension's argument
 * @param[in] COMPILED 0 -> compiled data dosnt exists, 1 -> compiled data exists
 * @param[in] DSC      string reprezent description
 * @param[in] EXTS     size of list of extension instances
 * @param[in] FLAGS    schema nodes flags
 * @param[in] NAME     string reprezent extension name
 * @param[in] REF      string reprezent reference statement
 */
#define LYSP_EXT_CHECK(NODE, ARGUMENT, COMPILED, DSC, EXTS, FLAGS, NAME, REF)\
                 assert_non_null(NODE);\
                if (ARGUMENT == NULL) {\
                    assert_null((NODE)->argument);\
                }\
                else {\
                    assert_non_null((NODE)->argument);\
                    assert_string_equal((NODE)->argument, ARGUMENT);\
                }\
                assert_pointer((NODE)->compiled, COMPILED);\
                if (DSC == NULL) {\
                    assert_null((NODE)->dsc);\
                }\
                else {\
                    assert_non_null((NODE)->dsc);\
                    assert_string_equal((NODE)->dsc, DSC);\
                }\
                assert_array((NODE)->exts, EXTS);\
                assert_int_equal((NODE)->flags, FLAGS);\
                assert_string_equal((NODE)->name, NAME);\
                if (REF == NULL) {\
                    assert_null((NODE)->ref);\
                }\
                else {\
                    assert_non_null((NODE)->ref);\
                    assert_string_equal((NODE)->ref, REF);\
                }

/**
 * @brief Macro check if lysp_stmt value is correct 
 * 
 * @param[in] NODE pointer to lysp_ext_instance variable
 * @param[in] ARGUMENT string optional value of the extension's argument
 * @param[in] CHILD    0 -> node doesnt have child, 1 -> node have children
 * @param[in] COMPILED 0 -> compiled data dosnt exists, 1 -> compiled data exists
 * @param[in] INSUBSTMS value identifying placement of the extension instance
 * @param[in] INSUBSTMS_INDEX indentifi index
 * @param[in] PARENT   0 -> if node is root otherwise 1
 * @param[in] PARENT_TYPE parent type. not relevat if PARENT == 0
 * @param[in] YIN         int ?? 
 */
#define LYSP_EXT_INSTANCE_CHECK(NODE, ARGUMENT, CHILD, COMPILED, INSUBSTMT, INSUBSTMT_INDEX, NAME, PARENT, PARENT_TYPE, YIN)\
                assert_non_null(NODE);\
                if (ARGUMENT == NULL) {\
                    assert_null((NODE)->argument);\
                }\
                else {\
                    assert_non_null((NODE)->argument);\
                    assert_string_equal((NODE)->argument, ARGUMENT);\
                }\
                assert_pointer((NODE)->compiled, COMPILED);\
                assert_int_equal((NODE)->insubstmt, INSUBSTMT);\
                assert_int_equal((NODE)->insubstmt_index, INSUBSTMT_INDEX);\
                assert_string_equal((NODE)->name, NAME);\
                if (PARENT != 0) { \
                    assert_non_null((NODE)->parent); \
                    assert_int_equal((NODE)->parent_type, PARENT_TYPE); \
                } \
                else { \
                    assert_null((NODE)->parent); \
                }\
                assert_int_equal((NODE)->yin, YIN)


/**
 * @brief Macro check if lysp_stmt value is correct 
 * 
 * @param[in] NODE pointer to lysp_stmt variable
 * @param[in] ARG  string statemet argumet
 * @param[in] CHILD 0 -> pointer is NULL, 1 -> pointer is not null
 * @param[in] FLAGS int statement flags
 * @param[in] NEXT  0 -> pointer is NULL, 1 -> pointer is not null 
 * @param[in] STMS  string identifier of the statement 
 */
#define LYSP_STMT_CHECK(NODE, ARG, CHILD, FLAGS, KW, NEXT, STMT)\
                assert_non_null(NODE);\
                if(ARG == NULL) {\
                    assert_null((NODE)->arg);\
                }\
                else  {\
                    assert_non_null((NODE)->arg);\
                    assert_string_equal((NODE)->arg, ARG);\
                }\
                assert_pointer((NODE)->child, CHILD);\
                assert_int_equal((NODE)->flags, FLAGS);\
                assert_int_equal((NODE)->kw, KW);\
                assert_pointer((NODE)->next, NEXT);\
                assert_string_equal((NODE)->stmt, STMT)\

/**
 * @brief Macro check if lysp_type_enum value is correct 
 * 
 * @param[in] NODE pointer to lysp_type_enum variable
 * @param[in] ARG  string statemet argumet
 * @param[in] CHILD 0 -> pointer is NULL, 1 -> pointer is not null
 * @param[in] FLAGS int statement flags
 * @param[in] NEXT  0 -> pointer is NULL, 1 -> pointer is not null 
 * @param[in] STMS  string identifier of the statement 
 */
#define LYSP_TYPE_ENUM_CHECK(NODE, DSC, EXTS, FLAGS, IFFEATURES, NAME, REF, VALUE)\
                assert_non_null(NODE);\
                if(DSC != NULL){\
                    assert_string_equal(DSC, (NODE)->dsc);\
                } else {\
                    assert_null((NODE)->dsc);\
                }\
                assert_array((NODE)->exts, EXTS);\
                assert_int_equal(FLAGS, (NODE)->flags);\
                assert_array((NODE)->iffeatures, IFFEATURES);\
                if(NAME != NULL){\
                    assert_string_equal(NAME, (NODE)->name);\
                } else {\
                    assert_null((NODE)->name);\
                }\
                if(REF != NULL){\
                    assert_string_equal(REF, (NODE)->ref);\
                } else {\
                    assert_null((NODE)->ref);\
                }\
                assert_int_equal(VALUE, (NODE)->value)


/**
 * @brief Macro check if lysp_node is correct 
 * 
 * @param[in] NODE  pointer to lysp_node variable
 * @param[in] DSC   description statement
 * @param[in] EXTS  0 pointer is null, 1 pointer is not null  
 * @param[in] FLAGS flags
 * @param[in] IFFEATURES  0 pointer is null, 1 pointer is not null
 * @param[in] NAME  string reprezenting node name 
 * @param[in] NEXT  0 pointer is null, 1 pointer is not null
 * @param[in] TYPE  node type (LYS_LEAF, ....)
 * @param[in] PARENT 0 pointer is null, 1 pointer is not null
 * @param[in] REF   string
 * @param[in] WHEN 0 pointer is null, 1 pointer is not null
 */
#define LYSP_NODE_CHECK(NODE, DSC, EXTS, FLAGS, IFFEATURES, NAME, NEXT, TYPE, PARENT, REF, WHEN) \
                assert_non_null(NODE);\
                if(DSC != NULL){\
                    assert_non_null((NODE)->dsc);\
                    assert_string_equal(DSC, (NODE)->dsc);\
                } else {\
                    assert_null((NODE)->dsc);\
                }\
                assert_array((NODE)->exts, EXTS);\
                assert_int_equal(FLAGS, (NODE)->flags);\
                assert_array((NODE)->iffeatures, IFFEATURES);\
                assert_non_null((NODE)->name);\
                assert_string_equal(NAME, (NODE)->name);\
                assert_true(NEXT == 0 ? (NODE)->next == NULL : (NODE)->next != NULL);\
                assert_int_equal(TYPE, (NODE)->nodetype);\
                assert_true(PARENT == 0 ? (NODE)->parent == NULL : (NODE)->parent != NULL);\
                if(REF != NULL){\
                    assert_non_null((NODE)->ref);\
                    assert_string_equal(REF, (NODE)->ref);\
                } else {\
                    assert_null((NODE)->ref);\
                }\
                assert_true(WHEN == 0 ? (NODE)->when == NULL : (NODE)->when != NULL)\



/**
 * @brief Macro check if lyd_notif have correct values 
 * 
 * @param[in] NODE  pointer to lysc_node variable
 * @param[in] NAME  node name 
 */
#define LYSC_NOTIF_CHECK(NODE, NAME) \
                assert_non_null(NODE);\
                assert_string_equal(NAME, (NODE)->name);\
                assert_int_equal(LYS_NOTIF,  (NODE)->nodetype);\
/**
 * @brief Macro check if lyd_action have correct values 
 * 
 * @param[in] NODE  pointer to lysc_node variable
 * @param[in] TYPE  node_type LYS_RPC or LYS_ACTION 
 * @param[in] NAME  action name
 */
#define LYSC_ACTION_CHECK(NODE, TYPE, NAME) \
                assert_non_null(NODE);\
                assert_string_equal(NAME, (NODE)->name);\
                assert_int_equal(TYPE,  (NODE)->nodetype);\

/**
 * @brief Macro check if lyd_node have correct values 
 * 
 * @param[in] NODE  pointer to lysc_node variable
 * @param[in] TYPE  node_type LYSC_LEAF, LYSC_CONTAINER, ...
 * @param[in] NAME  lysc_node name 
 */
#define LYSC_NODE_CHECK(NODE, TYPE, NAME) \
                assert_non_null(NODE);\
                assert_string_equal(NAME, (NODE)->name);\
                assert_int_equal(TYPE,  (NODE)->nodetype);\
                assert_non_null((NODE)->prev);

 /**
 * @brief Macro check if lyd_meta have correct values 
 * 
 * @param[in] NODE             pointer to lyd_meta variable
 * @param[in] PARENT           pointer to paren
 * @param[in] TYPE_VAL         value typ INT8, UINT8, STRING, ...
 * @param[in] VALUE_CANNONICAL string value
 * @param[in] ...              specific variable parameters
 */
#define LYD_META_CHECK(NODE, PARENT, NAME, TYPE_VAL, CANNONICAL_VAL, ...)\
                assert_non_null(NODE);\
                assert_ptr_equal((NODE)->parent, PARENT);\
                assert_string_equal((NODE)->name, NAME);\
                LYD_VALUE_CHECK((NODE)->value, TYPE_VAL, CANNONICAL_VAL __VA_OPT__(,) __VA_ARGS__)

 /**
 * @brief Macro check if lyd_node_term have correct values 
 * 
 * @param[in] NODE             pointer to lyd_node_term variable
 * @param[in] VALUE_TYPE value type INT8, UINT8, STRING, ...
 * @param[in] VALUE_CANNONICAL string value
 * @param[in] ...              specific variable parameters
 */
#define LYD_NODE_TERM_CHECK(NODE, VALUE_TYPE, VALUE_CANNONICAL, ...)\
                assert_non_null(NODE);\
                assert_int_equal((NODE)->flags, 0);\
                assert_non_null((NODE)->prev);\
                LYD_VALUE_CHECK((NODE)->value, VALUE_TYPE, VALUE_CANNONICAL  __VA_OPT__(,) __VA_ARGS__)

 /**
 * @brief Macro check if lyd_node_any have correct values 
 * 
 * @param[in] NODE       lyd_node_any
 * @param[in] VALUE_TYPE value type LYD_ANYDATA_VALUETYPE
 */
#define LYD_NODE_ANY_CHECK(NODE, VALUE_TYPE)\
                assert_non_null(NODE);\
                assert_int_equal((NODE)->flags, 0);\
                assert_non_null((NODE)->prev);\
                assert_int_equal((NODE)->value_type, VALUE_TYPE);\
                assert_non_null((NODE)->schema)
               
 /**
 * @brief Macro check if lyd_node_opaq have correct values 
 * 
 * @param[in] NODE     lyd_node_opaq
 * @param[in] ATTR     0,1 if attr should be null or if it is set
 * @param[in] CHILD    0,1 if node have child
 * @param[in] FORMAT   LYD_XML, LYD_JSON
 * @param[in] VAL_PREFS_COUNT count of val_prefs
 * @param[in] NAME     string name
 * @param[in] value    string value
 */ 
#define LYD_NODE_OPAQ_CHECK(NODE, ATTR, CHILD, FORMAT, VAL_PREFS_COUNT, NAME, VALUE)\
                assert_non_null(NODE);\
                assert_true((ATTR == 0) ? ((NODE)->attr == NULL) : ((NODE)->attr != NULL));\
                assert_true((CHILD == 0) ? ((NODE)->child == NULL) : ((NODE)->child != NULL));\
                assert_ptr_equal((NODE)->ctx, CONTEXT_GET);\
                assert_int_equal((NODE)->flags, 0);\
                assert_true((NODE)->format == FORMAT);\
                assert_int_equal((NODE)->hash, 0);\
                assert_string_equal((NODE)->name, NAME);\
                assert_non_null((NODE)->prev);\
                assert_null((NODE)->schema);\
                assert_array((NODE)->val_prefs, VAL_PREFS_COUNT);\
                assert_string_equal((NODE)->value, VALUE)\

/**
 * @brief Macro check if lyd_value have expected data.
 * 
 * @param[in] NODE     lyd_value
 * @param[in] TYPE_VAL value type. (INT8, UINT8, EMPTY, UNION, BITS, ...)
 *                     part of text reprezenting LY_DATA_TYPE.
 * @param[in] CANNONICAL_VAL string represents cannonical value
 * @param[in] ...      Unspecified parameters. Type and numbers of parameters are specified
 *                     by type of value. These parameters are passed to macro
 *                     LYD_VALUE_CHECK_ ## TYPE_VAL.
 */

#define LYD_VALUE_CHECK(NODE, TYPE_VAL, CANNONICAL_VAL, ...) \
                assert_non_null((NODE).canonical);\
                assert_string_equal(CANNONICAL_VAL, (NODE).canonical);\
                LYD_VALUE_CHECK_ ## TYPE_VAL (NODE __VA_OPT__(,) __VA_ARGS__)

/* 
    LYD VALUES CHECKING SPECIALIZATION, DONT USE THESE MACROS
*/

/**
 * @brief Macro check correctnes of lyd_value which is type EMPTY.
 * @brief Example LYD_VALUE_CHECK(node->value, EMPTY, ""); 
 * @param[in] NODE     lyd_value
 */
#define LYD_VALUE_CHECK_EMPTY(NODE) \
                assert_non_null((NODE).realtype); \
                assert_int_equal(LY_TYPE_EMPTY, (NODE).realtype->basetype) \

/**
 * @brief Macro check correctnes of lyd_value which is type UNION.
 * @brief Example LYD_VALUE_CHECK(node->value, UNION, "12", INT8, "12", 12);
 * @warning   type of subvalue cannot be UNION. Example of calling 
 * @param[in] NODE       lyd_value
 * @param[in] VALUE_TYPE type of suvalue. INT8, UINT8 
 * @param[in] CNNONICAL_VAL string represents cannonical value 
 */
#define LYD_VALUE_CHECK_UNION(NODE, VALUE_TYPE, CANNONICAL_VAL, ...) \
                assert_non_null((NODE).realtype); \
                assert_int_equal(LY_TYPE_UNION, (NODE).realtype->basetype); \
                assert_non_null((NODE).subvalue);\
                assert_non_null((NODE).subvalue->prefix_data);\
                assert_non_null((NODE).subvalue->value.canonical);\
                assert_string_equal(CANNONICAL_VAL, (NODE).subvalue->value.canonical);\
                LYD_VALUE_CHECK_ ## VALUE_TYPE ((NODE).subvalue->value __VA_OPT__(,) __VA_ARGS__)

/**
 * @brief Macro check correctnes of lyd_value which is type BITS.
 * @brief Example arr = {"a", "b"}; LYD_VALUE_CHECK(node->value, BITS, "a b", arr);
 * @param[in] NODE     lyd_value
 * @param[in] VALUE    array of names
 */
#define LYD_VALUE_CHECK_BITS(NODE, VALUE) \
                assert_non_null((NODE).realtype); \
                assert_int_equal(LY_TYPE_BITS, (NODE).realtype->basetype); \
                {\
                    int unsigned arr_size = sizeof(VALUE)/sizeof(VALUE[0]);\
                    assert_int_equal(arr_size, LY_ARRAY_COUNT((NODE).bits_items));\
                    for (int unsigned it = 0; it < arr_size; it++) {\
                        assert_string_equal(VALUE[it], (NODE).bits_items[it]->name);\
                    }\
                }

/**
 * @brief Macro check correctnes of lyd_value which is type INST.
 * @param[in] NODE     lyd_value
 * @param[in] VALUE    array of enum ly_path_pred_type
 * @brief Example enum arr = {0x0, 0x1}; LYD_VALUE_CHECK(node->value, INST, "test/d", arr);
 */
#define LYD_VALUE_CHECK_INST(NODE, VALUE) \
                assert_non_null((NODE).realtype); \
                assert_int_equal(LY_TYPE_INST, (NODE).realtype->basetype); \
                {\
                    int unsigned arr_size = sizeof(VALUE)/sizeof(VALUE[0]);\
                    assert_int_equal(arr_size, LY_ARRAY_COUNT((NODE).target));\
                    for (int unsigned it = 0; it < arr_size; it++) {\
                        assert_int_equal(VALUE[it], (NODE).target[it].pred_type);\
                    }\
                }

/**
 * @brief Macro check correctnes of lyd_value which is type ENUM.
 * @brief Example LYD_VALUE_CHECK(node->value, ENUM, "item_name", "item_name");
 * @param[in] NODE  lyd_value
 * @param[in] VALUE name of item in enum.
 */
#define LYD_VALUE_CHECK_ENUM(NODE, VALUE) \
                assert_non_null((NODE).realtype); \
                assert_int_equal(LY_TYPE_ENUM, (NODE).realtype->basetype); \
                assert_string_equal(VALUE, (NODE).enum_item->name)

/**
 * @brief Macro check correctnes of lyd_value which is type INT8.
 * @brief Example LYD_VALUE_CHECK(node->value, INT8, "12", 12);
 * @param[in] NODE  lyd_value
 * @param[in] VALUE inteager (-128 to 127).
 */
#define LYD_VALUE_CHECK_INT8(NODE, VALUE) \
                assert_non_null((NODE).realtype);\
                assert_int_equal(LY_TYPE_INT8, (NODE).realtype->basetype); \
                assert_int_equal(VALUE, (NODE).int8)

/**
 * @brief Macro check correctnes of lyd_value which is type UINT8.
 * @brief Example LYD_VALUE_CHECK(node->value, UINT8, "12", 12);
 * @param[in] NODE  lyd_value
 * @param[in] VALUE inteager (0 to 255).
 */
#define LYD_VALUE_CHECK_UINT8(NODE, VALUE) \
                assert_non_null((NODE).realtype);\
                assert_int_equal(LY_TYPE_UINT8, (NODE).realtype->basetype); \
                assert_int_equal(VALUE, (NODE).uint8)
/**
 * @brief Macro check correctnes of lyd_value which is type STRING.
 * @brief Example LYD_VALUE_CHECK(node->value, STRING, "text");
 * @param[in] NODE  lyd_value
 */
#define LYD_VALUE_CHECK_STRING(NODE) \
                assert_non_null((NODE).realtype);\
                assert_int_equal(LY_TYPE_STRING, (NODE).realtype->basetype)

/**
 * @brief Macro check correctnes of lyd_value which is type LEAFREF.
 * @brief Example LYD_VALUE_CHECK(node->value, LEAFREF, "");
 * @param[in] NODE  lyd_value
 */
#define LYD_VALUE_CHECK_LEAFREF(NODE) \
                assert_non_null((NODE).realtype)\
                assert_int_equal(LY_TYPE_LEAFREF, (NODE).realtype->basetype);\
                assert_non_null((NODE).ptr)

/**
 * @brief Macro check correctnes of lyd_value which is type DEC64.
 * @brief Example LYD_VALUE_CHECK(node->value, DEC64, "125", 125);
 * @param[in] NODE  lyd_value
 * @param[in] VALUE 64bit inteager
*/
#define LYD_VALUE_CHECK_DEC64(NODE, VALUE) \
                assert_non_null((NODE).realtype);\
                assert_int_equal(LY_TYPE_DEC64, (NODE).realtype->basetype); \
                assert_int_equal(VALUE, (NODE).dec64)

/**
 * @brief Macro check correctnes of lyd_value which is type BINNARY.
 * @brief Example LYD_VALUE_CHECK(node->value, BINARY, "aGVs\nbG8=");
 * @param[in] NODE  lyd_value
*/
#define LYD_VALUE_CHECK_BINARY(NODE) \
                assert_non_null((NODE).realtype);\
                assert_int_equal(LY_TYPE_BINARY, (NODE).realtype->basetype)

/**
 * @brief Macro check correctnes of lyd_value which is type BOOL.
 * @brief Example LYD_VALUE_CHECK(node->value, BOOL, "true", 1);
 * @param[in] NODE  lyd_value
 * @param[in] VALUE boolean value 0,1
*/
#define LYD_VALUE_CHECK_BOOL(NODE, VALUE) \
                assert_non_null((NODE).realtype); \
                assert_int_equal(LY_TYPE_BOOL, (NODE).realtype->basetype); \
                assert_int_equal(VALUE, (NODE).boolean)

/**
 * @brief Macro check correctnes of lyd_value which is type IDENT.
 * @brief Example LYD_VALUE_CHECK(node->value, IDENT, "types:gigabit-ethernet", "gigabit-ethernet");
 * @param[in] NODE  lyd_value
 * @param[in] VALUE ident name
*/
#define LYD_VALUE_CHECK_IDENT(NODE, VALUE) \
                assert_non_null((NODE).realtype); \
                assert_int_equal(LY_TYPE_IDENT, (NODE).realtype->basetype); \
                assert_string_equal(VALUE, (NODE).ident->name)



#endif

