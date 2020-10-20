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
    LIBYANG NODE CHECKING
*/

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
                assert_true((VAL_PREFS_COUNT == 0) ? ((NODE)->val_prefs == NULL)\
                        : (VAL_PREFS_COUNT == LY_ARRAY_COUNT((NODE)->val_prefs)));\
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

