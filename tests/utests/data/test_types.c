/*
 * @file test_types.c
 * @author: Radek Krejci <rkrejci@cesnet.cz>
 * @brief unit tests for support of YANG data types
 *
 * Copyright (c) 2019 CESNET, z.s.p.o.
 *
 * This source code is licensed under BSD 3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://opensource.org/licenses/BSD-3-Clause
 */

#include "../macros.h"

#include <stdio.h>
#include <string.h>

#include "libyang.h"
#include "plugins_types.h"
#include "path.h"

const char *schema_a = "module defs {namespace urn:tests:defs;prefix d;yang-version 1.1;"
        "identity crypto-alg; identity interface-type; identity ethernet {base interface-type;} identity fast-ethernet {base ethernet;}"
        "typedef iref {type identityref {base interface-type;}}}";
const char *schema_b = "module types {namespace urn:tests:types;prefix t;yang-version 1.1; import defs {prefix defs;}"
        "feature f; identity gigabit-ethernet { base defs:ethernet;}"
        "typedef tboolean {type boolean;}"
        "typedef tempty {type empty;}"
        "container cont {leaf leaftarget {type empty;}"
                        "list listtarget {key id; max-elements 5;leaf id {type uint8;} leaf value {type string;}}"
                        "leaf-list leaflisttarget {type uint8; max-elements 5;}}"
        "list list {key id; leaf id {type string;} leaf value {type string;} leaf-list targets {type string;}}"
        "list list2 {key \"id value\"; leaf id {type string;} leaf value {type string;}}"
        "list list_inst {key id; leaf id {type instance-identifier {require-instance true;}} leaf value {type string;}}"
        "list list_ident {key id; leaf id {type identityref {base defs:interface-type;}} leaf value {type string;}}"
        "list list_keyless {config \"false\"; leaf id {type string;} leaf value {type string;}}"
        "leaf-list leaflisttarget {type string;}"
        "leaf binary {type binary {length 5 {error-message \"This base64 value must be of length 5.\";}}}"
        "leaf binary-norestr {type binary;}"
        "leaf int8 {type int8 {range 10..20;}}"
        "leaf uint8 {type uint8 {range 150..200;}}"
        "leaf int16 {type int16 {range -20..-10;}}"
        "leaf uint16 {type uint16 {range 150..200;}}"
        "leaf int32 {type int32;}"
        "leaf uint32 {type uint32;}"
        "leaf int64 {type int64;}"
        "leaf uint64 {type uint64;}"
        "leaf bits {type bits {bit zero; bit one {if-feature f;} bit two;}}"
        "leaf enums {type enumeration {enum white; enum yellow {if-feature f;}}}"
        "leaf dec64 {type decimal64 {fraction-digits 1; range 1.5..10;}}"
        "leaf dec64-norestr {type decimal64 {fraction-digits 18;}}"
        "leaf str {type string {length 8..10; pattern '[a-z ]*';}}"
        "leaf str-norestr {type string;}"
        "leaf str-utf8 {type string{length 2..5; pattern '€*';}}"
        "leaf bool {type boolean;}"
        "leaf tbool {type tboolean;}"
        "leaf empty {type empty;}"
        "leaf tempty {type tempty;}"
        "leaf ident {type identityref {base defs:interface-type;}}"
        "leaf iref {type defs:iref;}"
        "leaf inst {type instance-identifier {require-instance true;}}"
        "leaf inst-noreq {type instance-identifier {require-instance false;}}"
        "leaf lref {type leafref {path /leaflisttarget; require-instance true;}}"
        "leaf lref2 {type leafref {path \"../list[id = current()/../str-norestr]/targets\"; require-instance true;}}"
        "leaf un1 {type union {"
          "type leafref {path /int8; require-instance true;}"
          "type union { type identityref {base defs:interface-type;} type instance-identifier {require-instance true;} }"
          "type string {length 1..20;}}}}";

#define CONTEXT_CREATE(LYD_NODE_1, LYD_NODE_2) \
                ly_set_log_clb(logger_null, 1);\
                CONTEXT_CREATE_PATH(NULL);\
                assert_int_equal(LY_SUCCESS, lys_parse_mem(CONTEXT_GET, schema_a, LYS_IN_YANG, &(LYD_NODE_1)));  \
                assert_int_equal(LY_SUCCESS, lys_parse_mem(CONTEXT_GET, schema_b, LYS_IN_YANG, &(LYD_NODE_2)))


#define LYD_NODE_CREATE(INPUT, MODEL) \
                LYD_NODE_CREATE_PARAM(INPUT, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_SUCCESS, MODEL)


#define LYD_NODE_CHECK_CHAR(IN_MODEL, TEXT) \
                LYD_NODE_CHECK_CHAR_PARAM(IN_MODEL, TEXT, LYD_XML, LYD_PRINT_SHRINK | LYD_PRINT_WITHSIBLINGS | LYD_PRINT_SHRINK)


#define TEST_PATTERN_1(INPUT, SCHEMA_NAME, VALUE_TYPE, VALUE_CANNONICAL, ...) \
                { \
                    struct lyd_node_term *leaf;\
                    struct lyd_value value = {0};\
                    /*create model*/\
                    LYSC_NODE_CHECK((INPUT)->schema, LYS_LEAF, SCHEMA_NAME);\
                    leaf = (struct lyd_node_term*)(INPUT);\
                    LYD_NODE_TERM_CHECK(leaf, VALUE_TYPE, VALUE_CANNONICAL __VA_OPT__(,) __VA_ARGS__);\
                    /* copy value */ \
                    assert_int_equal(LY_SUCCESS, leaf->value.realtype->plugin->duplicate(CONTEXT_GET, &leaf->value, &value));\
                    LYD_VALUE_CHECK(value, VALUE_TYPE, VALUE_CANNONICAL __VA_OPT__(,) __VA_ARGS__);\
                    if(LY_TYPE_INST == LY_TYPE_ ## VALUE_TYPE) {\
                        int unsigned arr_size = LY_ARRAY_COUNT(leaf->value.target);\
                        for (int unsigned it = 0; it < arr_size; it++) {\
                            assert_ptr_equal(value.target[it].node, leaf->value.target[it].node);\
                            int unsigned arr_size_predicates = 0;\
                            if(value.target[it].pred_type == LY_PATH_PREDTYPE_NONE){\
                                assert_null(value.target[it].predicates);\
                            }\
                            else {\
                                assert_non_null(value.target[it].predicates);\
                                arr_size_predicates =  LY_ARRAY_COUNT(value.target[it].predicates);\
                                assert_int_equal(LY_ARRAY_COUNT(value.target[it].predicates), LY_ARRAY_COUNT(leaf->value.target[it].predicates));\
                            }\
                            for (int unsigned jt = 0; jt < arr_size_predicates; jt++) {\
                                if (value.target[it].pred_type == LY_PATH_PREDTYPE_POSITION) {\
                                    assert_int_equal(value.target[it].predicates[jt].position, leaf->value.target[it].predicates[jt].position);\
                                }\
                                else{\
                                    assert_true(LY_SUCCESS == value.realtype->plugin->compare(&value, &leaf->value));\
                                }\
                            }\
                        }\
                    }\
                    value.realtype->plugin->free(CONTEXT_GET, &value);\
                }


#define logbuf_assert(str)\
    {\
        const char * err_msg[]  = {str};\
        const char * err_path[] = {NULL};\
        LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);\
    }


#define TEST_TYPE_ERROR(TYPE, VALUE, ERROR_MSG) \
			{\
				const char *data    = "<" TYPE " xmlns=\"urn:tests:types\">" VALUE "</" TYPE">";\
    			LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, tree);\
 	    		const char *err_msg[]  = {ERROR_MSG};\
    			const char *err_path[] = {"/types:" TYPE};\
   				LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);\
			}


static void
test_int(void **state)
{
    (void) state;

    struct lyd_node *tree;
    const char *data;
	char *error_msg;

    const struct lys_module *mod_types;
    const struct lys_module *mod_defs;

    CONTEXT_CREATE(mod_defs, mod_types);

    /* valid data */
    data = "<int8 xmlns=\"urn:tests:types\">\n 15 \t\n  </int8>";
    LYD_NODE_CREATE(data,tree);
    TEST_PATTERN_1(tree,"int8", INT8, "15", 15);
    LYD_NODE_DESTROY(tree);

    /* invalid range */
	error_msg = "Value \"1\" does not satisfy the range constraint.";
	TEST_TYPE_ERROR("int8", "1", error_msg);
	error_msg = "Value \"100\" does not satisfy the range constraint.";
   	TEST_TYPE_ERROR("int16", "100", error_msg);


    /* invalid value */
    error_msg =  "Invalid int32 value \"0x01\".";;
    TEST_TYPE_ERROR("int32", "0x01", error_msg);
    error_msg =  "Invalid empty int64 value.";;
    TEST_TYPE_ERROR("int64", "", error_msg);
    error_msg =  "Invalid empty int64 value.";;
    TEST_TYPE_ERROR("int64", "   ", error_msg);
    error_msg =  "Invalid int64 value \"-10  xxx\".";;
    TEST_TYPE_ERROR("int64", "-10  xxx", error_msg);

    CONTEXT_DESTROY;
}

static void
test_uint(void **state)
{
    (void) state;

    struct lyd_node *tree;
    char *err_msg[1];
    char *err_path[1];
    const char *data, *error_msg;

    const struct lys_module *mod_types;
    const struct lys_module *mod_defs;

    CONTEXT_CREATE(mod_defs, mod_types);
    /* valid data */
    data    = "<uint8 xmlns=\"urn:tests:types\">\n 150 \t\n  </uint8>";
    LYD_NODE_CREATE(data,tree);
    TEST_PATTERN_1(tree,"uint8", UINT8, "150", 150);
    LYD_NODE_DESTROY(tree);

    /* invalid range */
    error_msg =  "Value \"15\" does not satisfy the range constraint.";;
    TEST_TYPE_ERROR("uint8", "\n 15 \t\n  ", error_msg);
    error_msg =  "Value \"1500\" does not satisfy the range constraint.";;
    TEST_TYPE_ERROR("uint16", "\n 1500 \t\n  ", error_msg);

    /* invalid value */
    error_msg =  "Value \"-10\" is out of uint32's min/max bounds.";;
    TEST_TYPE_ERROR("uint32", "-10", error_msg);
    data    = "<uint64 xmlns=\"urn:tests:types\"/>";
    err_msg[0] = "Invalid empty uint64 value.";
    err_path[0] = "/types:uint64";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    error_msg =  "Invalid empty uint64 value.";;
    TEST_TYPE_ERROR("uint64", "   ", error_msg);
    error_msg =  "Invalid uint64 value \"10  xxx\".";;
    TEST_TYPE_ERROR("uint64", "10  xxx", error_msg);

    CONTEXT_DESTROY;
}

static void
test_dec64(void **state)
{
    (void) state;

    struct lyd_node *tree;
    char *err_msg[1];
    char *err_path[1];
    const char *data, *error_msg;

    const struct lys_module *mod_types;
    const struct lys_module *mod_defs;

    CONTEXT_CREATE(mod_defs, mod_types);

    /* valid data */
    data = "<dec64 xmlns=\"urn:tests:types\">\n +8 \t\n  </dec64>";
    LYD_NODE_CREATE(data,tree);
    TEST_PATTERN_1(tree,"dec64", DEC64, "8.0", 80);
    LYD_NODE_DESTROY(tree);

    data = "<dec64 xmlns=\"urn:tests:types\">8.00</dec64>";
    LYD_NODE_CREATE(data,tree);
    TEST_PATTERN_1(tree,"dec64", DEC64, "8.0", 80);
    LYD_NODE_DESTROY(tree);

    data = "<dec64-norestr xmlns=\"urn:tests:types\">-9.223372036854775808</dec64-norestr>";
    LYD_NODE_CREATE(data,tree);
    TEST_PATTERN_1(tree, "dec64-norestr", DEC64, "-9.223372036854775808", INT64_C(-9223372036854775807) - INT64_C(1));
    LYD_NODE_DESTROY(tree);

    data = "<dec64-norestr xmlns=\"urn:tests:types\">9.223372036854775807</dec64-norestr>";
    LYD_NODE_CREATE(data,tree);
    TEST_PATTERN_1(tree, "dec64-norestr", DEC64, "9.223372036854775807", INT64_C(9223372036854775807));
    LYD_NODE_DESTROY(tree);

    /* invalid range */
    error_msg =  "Value \"15.0\" does not satisfy the range constraint.";;
    TEST_TYPE_ERROR("dec64", "\n 15 \t\n  ", error_msg);
    error_msg =  "Value \"0.0\" does not satisfy the range constraint.";;
    TEST_TYPE_ERROR("dec64", "\n 0 \t\n  ", error_msg);

    /* invalid value */
    error_msg =  "Invalid 1. character of decimal64 value \"xxx\".";;
    TEST_TYPE_ERROR("dec64", "xxx", error_msg);
    data    = "<dec64 xmlns=\"urn:tests:types\"/>";
    err_msg[0] = "Invalid empty decimal64 value.";
    err_path[0] = "/types:dec64";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    error_msg =  "Invalid empty decimal64 value.";;
    TEST_TYPE_ERROR("dec64", "   ", error_msg);
    error_msg =  "Invalid 6. character of decimal64 value \"8.5  xxx\".";;
    TEST_TYPE_ERROR("dec64", "8.5  xxx", error_msg);
    error_msg =  "Value \"8.55\" of decimal64 type exceeds defined number (1) of fraction digits.";;
    TEST_TYPE_ERROR("dec64", "8.55  xxx", error_msg);

    CONTEXT_DESTROY;
}

static void
test_string(void **state)
{
    (void) state;

    struct lyd_node *tree;
    char *err_msg[1];
    char *err_path[1];
    const char *data, *error_msg;

    const struct lys_module *mod_types;
    const struct lys_module *mod_defs;

    CONTEXT_CREATE(mod_defs, mod_types);

    /* valid data */
    data = "<str xmlns=\"urn:tests:types\">teststring</str>";
    LYD_NODE_CREATE(data,tree);
    TEST_PATTERN_1(tree,"str", STRING, "teststring");
    LYD_NODE_DESTROY(tree);

    /* multibyte characters (€ encodes as 3-byte UTF8 character, length restriction is 2-5) */
    data = "<str-utf8 xmlns=\"urn:tests:types\">€€</str-utf8>";
    LYD_NODE_CREATE(data,tree);
    TEST_PATTERN_1(tree, "str-utf8", STRING, "€€");
    LYD_NODE_DESTROY(tree);

    /*error */
    error_msg =  "Length \"1\" does not satisfy the length constraint.";;
    TEST_TYPE_ERROR("str-utf8", "€", error_msg);
    error_msg =  "Length \"6\" does not satisfy the length constraint.";;
    TEST_TYPE_ERROR("str-utf8", "€€€€€€", error_msg);
    error_msg =  "String \"€€x\" does not conform to the pattern \"€*\".";;
    TEST_TYPE_ERROR("str-utf8", "€€x", error_msg);

    /* invalid length */
    error_msg =  "Length \"5\" does not satisfy the length constraint.";;
    TEST_TYPE_ERROR("str", "short", error_msg);
    error_msg =  "Length \"11\" does not satisfy the length constraint.";;
    TEST_TYPE_ERROR("str", "tooooo long", error_msg);

    /* invalid pattern */
    error_msg =  "String \"string15\" does not conform to the pattern \"[a-z ]*\".";;
    TEST_TYPE_ERROR("str", "string15", error_msg);

    CONTEXT_DESTROY;
}

static void
test_bits(void **state)
{

    (void) state;;

    struct lyd_node *tree;

    const char *data, *error_msg;

    const struct lys_module *mod_types;
    const struct lys_module *mod_defs;

    CONTEXT_CREATE(mod_defs, mod_types);

    /* valid data */
    data = "<bits xmlns=\"urn:tests:types\">\n two    \t\nzero\n  </bits>";
    const char *bits_array[] = {"zero", "two"};
    LYD_NODE_CREATE(data,tree);
    TEST_PATTERN_1(tree,"bits", BITS, "zero two", bits_array);
    LYD_NODE_DESTROY(tree);

    data = "<bits xmlns=\"urn:tests:types\">zero  two</bits>";
    LYD_NODE_CREATE(data,tree);
    TEST_PATTERN_1(tree,"bits", BITS, "zero two", bits_array);
    LYD_NODE_DESTROY(tree);

    /* disabled feature */
    error_msg =  "Bit \"one\" is disabled by its 1. if-feature condition.";;
    TEST_TYPE_ERROR("bits", " \t one \n\t ", error_msg);

    /* enable that feature */
    assert_int_equal(LY_SUCCESS, lys_feature_enable(ly_ctx_get_module(CONTEXT_GET, "types", NULL), "f"));
    const char *bits_array_2[] = {"one"};
    data = "<bits xmlns=\"urn:tests:types\"> \t one \n\t </bits>";
    LYD_NODE_CREATE(data,tree);
    TEST_PATTERN_1(tree,"bits", BITS, "one", bits_array_2);
    LYD_NODE_DESTROY(tree);

    /* disabled feature */
    TEST_DATA("<bits xmlns=\"urn:tests:types\"> \t one \n\t </bits>", LY_EVALID, "Bit \"one\" is disabled by its 1. if-feature condition. /types:bits");

    /* multiple instances of the bit */
    error_msg =  "Bit \"one\" used multiple times.";;
    TEST_TYPE_ERROR("bits", "one zero one", error_msg);

    /* invalid bit value */
    error_msg =  "Invalid bit value \"xero\".";;
    TEST_TYPE_ERROR("bits", "one xero one", error_msg);

    CONTEXT_DESTROY;
}

static void
test_enums(void **state)
{
    (void) state;

    struct lyd_node *tree;
    const char *data, *error_msg;

    const struct lys_module *mod_types;
    const struct lys_module *mod_defs;

    CONTEXT_CREATE(mod_defs, mod_types);

    /* valid data */
    data = "<enums xmlns=\"urn:tests:types\">white</enums>";
    LYD_NODE_CREATE(data,tree);
    TEST_PATTERN_1(tree,"enums", ENUM, "white", "white");
    LYD_NODE_DESTROY(tree);

    /* disabled feature */
    error_msg =  "Enumeration \"yellow\" is disabled by its 1. if-feature condition.";;
    TEST_TYPE_ERROR("enums", "yellow", error_msg);

    /* enable that feature */
    assert_int_equal(LY_SUCCESS, lys_feature_enable(ly_ctx_get_module(CONTEXT_GET, "types", NULL), "f"));
    data = "<enums xmlns=\"urn:tests:types\">yellow</enums>";
    LYD_NODE_CREATE(data,tree);
    TEST_PATTERN_1(tree,"enums", ENUM, "yellow", "yellow");
    LYD_NODE_DESTROY(tree);

    /* leading/trailing whitespaces are not valid */
    error_msg =  "Invalid enumeration value \" white\".";;
    TEST_TYPE_ERROR("enums", " white", error_msg);
    error_msg =  "Invalid enumeration value \"white\n\".";;
    TEST_TYPE_ERROR("enums", "white\n", error_msg);

    /* invalid enumeration value */
    error_msg =  "Invalid enumeration value \"black\".";;
    TEST_TYPE_ERROR("enums", "black", error_msg);

    CONTEXT_DESTROY;
}

static void
test_binary(void **state)
{
    (void) state;

    struct lyd_node *tree;
    const char *data, *error_msg;

    const struct lys_module *mod_types;
    const struct lys_module *mod_defs;

    CONTEXT_CREATE(mod_defs, mod_types);

    /* valid data (hello) */
    data = "<binary xmlns=\"urn:tests:types\">\n   aGVs\nbG8=  \t\n  </binary><binary-norestr xmlns=\"urn:tests:types\">TQ==</binary-norestr>";
    LYD_NODE_CREATE(data,tree);
    TEST_PATTERN_1(tree, "binary", BINARY, "aGVs\nbG8=");
    assert_non_null(tree = tree->next);
    TEST_PATTERN_1(tree, "binary-norestr", BINARY, "TQ==");
    LYD_NODE_DESTROY(tree);

    /* no data */
    data = "<binary-norestr xmlns=\"urn:tests:types\">\n    \t\n  </binary-norestr>";
    LYD_NODE_CREATE(data,tree);
    TEST_PATTERN_1(tree, "binary-norestr", BINARY, "</binary-norestr>");
    LYD_NODE_DESTROY(tree);

    data = "<binary-norestr xmlns=\"urn:tests:types\"></binary-norestr>";
    LYD_NODE_CREATE(data,tree);
    TEST_PATTERN_1(tree, "binary-norestr", BINARY, "");
    LYD_NODE_DESTROY(tree);

    data = "<binary-norestr xmlns=\"urn:tests:types\"/>";
    LYD_NODE_CREATE(data,tree);
    TEST_PATTERN_1(tree, "binary-norestr", BINARY, "");
    LYD_NODE_DESTROY(tree);

    /* invalid base64 character */
    error_msg =  "Invalid Base64 character (@).";;
    TEST_TYPE_ERROR("binary-norestr", "a@bcd=", error_msg);

    /* missing data */
    error_msg =  "Base64 encoded value length must be divisible by 4.";;
    TEST_TYPE_ERROR("binary-norestr", "aGVsbG8", error_msg);

    error_msg =  "Base64 encoded value length must be divisible by 4.";;
    TEST_TYPE_ERROR("binary-norestr", "VsbG8=", error_msg);

    /* invalid binary length */
    /* helloworld */
    error_msg =  "This base64 value must be of length 5.";;
    TEST_TYPE_ERROR("binary", "aGVsbG93b3JsZA==", error_msg);

    /* M */
    error_msg =  "This base64 value must be of length 5.";;
    TEST_TYPE_ERROR("binary", "TQ==", error_msg);

    CONTEXT_DESTROY;
}

static void
test_boolean(void **state)
{
    (void) state;
    struct lyd_node *tree;
    const char *data, *error_msg;

    const struct lys_module *mod_types;
    const struct lys_module *mod_defs;

    CONTEXT_CREATE(mod_defs, mod_types);

    /* valid data */
    data = "<bool xmlns=\"urn:tests:types\">true</bool>";
    LYD_NODE_CREATE(data,tree);
    TEST_PATTERN_1(tree, "bool", BOOL, "true", 1);
    LYD_NODE_DESTROY(tree);

    data = "<bool xmlns=\"urn:tests:types\">false</bool>";
    LYD_NODE_CREATE(data,tree);
    TEST_PATTERN_1(tree, "bool", BOOL, "false", 0);
    LYD_NODE_DESTROY(tree);

    data = "<tbool xmlns=\"urn:tests:types\">false</tbool>";
    LYD_NODE_CREATE(data,tree);
    TEST_PATTERN_1(tree, "tbool", BOOL, "false", 0);
    LYD_NODE_DESTROY(tree);

    /* invalid value */
    error_msg =  "Invalid boolean value \"unsure\".";;
    TEST_TYPE_ERROR("bool", "unsure", error_msg);

    error_msg =  "Invalid boolean value \" true\".";;
    TEST_TYPE_ERROR("bool", " true", error_msg);

    CONTEXT_DESTROY;
}

static void
test_empty(void **state)
{
    (void) state;

    struct lyd_node *tree;
    const char *data, *error_msg;
    const struct lys_module *mod_types;
    const struct lys_module *mod_defs;

    CONTEXT_CREATE(mod_defs, mod_types);

    /* valid data */
    data = "<empty xmlns=\"urn:tests:types\"></empty>";
    LYD_NODE_CREATE(data,tree);
    TEST_PATTERN_1(tree, "empty", EMPTY, "");
    LYD_NODE_DESTROY(tree);

    data = "<empty xmlns=\"urn:tests:types\"/>";
    LYD_NODE_CREATE(data,tree);
    TEST_PATTERN_1(tree, "empty", EMPTY, "");
    LYD_NODE_DESTROY(tree);

    data = "<tempty xmlns=\"urn:tests:types\"/>";
    LYD_NODE_CREATE(data,tree);
    TEST_PATTERN_1(tree, "tempty", EMPTY, "");
    LYD_NODE_DESTROY(tree);

    /* invalid value */
    error_msg =  "Invalid empty value \"x\".";;
    TEST_TYPE_ERROR("empty", "x", error_msg);

    error_msg =  "Invalid empty value \" \".";;
    TEST_TYPE_ERROR("empty", " ", error_msg);

    CONTEXT_DESTROY;
}

static void
test_printed_value(const struct lyd_value *value, const char *expected_value, LY_PREFIX_FORMAT format,
        const void *prefix_data)
{
    const char *str;
    uint8_t dynamic;

    assert_non_null(str = value->realtype->plugin->print(value, format, (void *)prefix_data, &dynamic));
    assert_string_equal(expected_value, str);
    if (dynamic) {
        free((char*)str);
    }
}


static void
test_identityref(void **state)
{
    (void) state;

    struct lyd_node *tree;
    struct lyd_node_term *leaf;
    char *err_msg[1];
    char *err_path[1];
    const char *data, *error_msg;

    const struct lys_module *mod_types;
    const struct lys_module *mod_defs;

    CONTEXT_CREATE(mod_defs, mod_types);

    /* valid data */
    data = "<ident xmlns=\"urn:tests:types\">gigabit-ethernet</ident>";
    LYD_NODE_CREATE(data,tree);
    TEST_PATTERN_1(tree, "ident", IDENT, "types:gigabit-ethernet", "gigabit-ethernet");
    leaf = (struct lyd_node_term*)tree;
    test_printed_value(&leaf->value, "t:gigabit-ethernet", LY_PREF_SCHEMA, mod_types->parsed);
    LYD_NODE_DESTROY(tree);

    data = "<ident xmlns=\"urn:tests:types\" xmlns:x=\"urn:tests:defs\">x:fast-ethernet</ident>";
    LYD_NODE_CREATE(data,tree);
    TEST_PATTERN_1(tree, "ident", IDENT, "defs:fast-ethernet", "fast-ethernet");
    leaf = (struct lyd_node_term*)tree;
    test_printed_value(&leaf->value, "d:fast-ethernet", LY_PREF_SCHEMA, mod_defs->parsed);
    LYD_NODE_DESTROY(tree);

    /* invalid value */
    error_msg =  "Invalid identityref \"fast-ethernet\" value - identity not found.";;
    TEST_TYPE_ERROR("ident", "fast-ethernet", error_msg);

    data    = "<ident xmlns=\"urn:tests:types\" xmlns:x=\"urn:tests:defs\">x:slow-ethernet</ident>";
    err_msg[0] = "Invalid identityref \"x:slow-ethernet\" value - identity not found.";
    err_path[0] = "/types:ident";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    data    = "<ident xmlns=\"urn:tests:types\" xmlns:x=\"urn:tests:defs\">x:crypto-alg</ident>";
    err_msg[0] = "Invalid identityref \"x:crypto-alg\" value - identity not accepted by the type specification.";
    err_path[0] = "/types:ident";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    data    = "<ident xmlns=\"urn:tests:types\" xmlns:x=\"urn:tests:unknown\">x:fast-ethernet</ident>";
    err_msg[0] = "Invalid identityref \"x:fast-ethernet\" value - unable to map prefix to YANG schema.";
    err_path[0] = "/types:ident";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    CONTEXT_DESTROY;
}

/* dummy get_prefix callback for test_instanceid() */
const struct lys_module *
test_instanceid_getprefix(const struct ly_ctx *ctx, const char *prefix, size_t prefix_len, void *private)
{
    (void)ctx;
    (void)prefix;
    (void)prefix_len;

    return private;
}

static void
test_instanceid(void **state)
{
    (void) state;

    char *err_msg[1];
    char *err_path[1];
    struct lyd_node *tree;
    struct lyd_node_term *leaf;
    const char *data, *error_msg;

    const struct lys_module *mod_types;
    const struct lys_module *mod_defs;

    CONTEXT_CREATE(mod_defs, mod_types);

    /* valid data */
    data = "<cont xmlns=\"urn:tests:types\"><leaftarget/></cont>"
              "<xdf:inst xmlns:xdf=\"urn:tests:types\">/xdf:cont/xdf:leaftarget</xdf:inst>";
    LYD_NODE_CREATE(data,tree);
    tree = tree->prev;
    const enum ly_path_pred_type result_1[] = {LY_PATH_PREDTYPE_NONE, LY_PATH_PREDTYPE_NONE};
    TEST_PATTERN_1(tree, "inst", INST, "/types:cont/leaftarget", result_1);
    leaf = (struct lyd_node_term*)tree;
    for(int unsigned it = 0; it < LY_ARRAY_COUNT(leaf->value.target); it++){
        const char      *node_name[] = {"cont", "leaftarget"};
        const uint16_t  node_type[] = {LYS_CONTAINER, LYS_LEAF};
        LYSC_NODE_CHECK(leaf->value.target[it].node, node_type[it], node_name[it]);
    }
    LYD_NODE_DESTROY(tree);


    data = "<list xmlns=\"urn:tests:types\"><id>a</id></list><list xmlns=\"urn:tests:types\"><id>b</id></list>"
           "<xdf:inst xmlns:xdf=\"urn:tests:types\">/xdf:list[xdf:id='b']/xdf:id</xdf:inst>";
    LYD_NODE_CREATE(data, tree);
    tree = tree->prev->prev;
    const enum ly_path_pred_type result_2[] = {LY_PATH_PREDTYPE_LIST, LY_PATH_PREDTYPE_NONE};
    TEST_PATTERN_1(tree, "inst", INST, "/types:list[id='b']/id", result_2);
    leaf = (struct lyd_node_term*)tree;
    for(int unsigned it = 0; it < LY_ARRAY_COUNT(leaf->value.target); it++){
        const char      *node_name[] = {"list", "id"};
        const uint16_t  node_type[] = {LYS_LIST, LYS_LEAF};
        LYSC_NODE_CHECK(leaf->value.target[it].node, node_type[it], node_name[it]);
    }
    LYD_NODE_DESTROY(tree);


    data = "<leaflisttarget xmlns=\"urn:tests:types\">1</leaflisttarget><leaflisttarget xmlns=\"urn:tests:types\">2</leaflisttarget>"
           "<xdf:inst xmlns:xdf=\"urn:tests:types\">/xdf:leaflisttarget[.='1']</xdf:inst>";
    LYD_NODE_CREATE(data, tree);
    tree = tree->prev->prev;
    const enum ly_path_pred_type result_3[] = {LY_PATH_PREDTYPE_LEAFLIST};
    TEST_PATTERN_1(tree, "inst", INST, "/types:leaflisttarget[.='1']", result_3);
    leaf = (struct lyd_node_term*)tree;
    for(int unsigned it = 0; it < LY_ARRAY_COUNT(leaf->value.target); it++){
        const char      *node_name[] = {"leaflisttarget"};
        const uint16_t  node_type[] = {LYS_LEAFLIST};
        LYSC_NODE_CHECK(leaf->value.target[it].node, node_type[it], node_name[it]);
    }
    LYD_NODE_DESTROY(tree);


    data = "<list_inst xmlns=\"urn:tests:types\"><id xmlns:b=\"urn:tests:types\">/b:leaflisttarget[.='a']</id><value>x</value></list_inst>"
           "<list_inst xmlns=\"urn:tests:types\"><id xmlns:b=\"urn:tests:types\">/b:leaflisttarget[.='b']</id><value>y</value></list_inst>"
           "<leaflisttarget xmlns=\"urn:tests:types\">a</leaflisttarget><leaflisttarget xmlns=\"urn:tests:types\">b</leaflisttarget>"
           "<a:inst xmlns:a=\"urn:tests:types\">/a:list_inst[a:id=\"/a:leaflisttarget[.='b']\"]/a:value</a:inst>";
    LYD_NODE_CREATE(data, tree);
    tree = tree->prev->prev;
    const enum ly_path_pred_type result_4[] = {LY_PATH_PREDTYPE_LIST, LY_PATH_PREDTYPE_NONE};
    TEST_PATTERN_1(tree, "inst", INST, "/types:list_inst[id=\"/types:leaflisttarget[.='b']\"]/value", result_4);
    leaf = (struct lyd_node_term*)tree;
    for(int unsigned it = 0; it < LY_ARRAY_COUNT(leaf->value.target); it++){
        const char      *node_name[] = {"list_inst", "value"};
        const uint16_t  node_type[] = {LYS_LIST, LYS_LEAF};
        LYSC_NODE_CHECK(leaf->value.target[it].node, node_type[it], node_name[it]);
    }
    assert_int_equal(1, LY_ARRAY_COUNT(leaf->value.target[0].predicates));
    assert_null(leaf->value.target[1].predicates);
    test_printed_value(&leaf->value, "/t:list_inst[t:id=\"/t:leaflisttarget[.='b']\"]/t:value", LY_PREF_SCHEMA, mod_types->parsed);
    test_printed_value(&leaf->value, "/types:list_inst[id=\"/types:leaflisttarget[.='b']\"]/value", LY_PREF_JSON, NULL);
    LYD_NODE_DESTROY(tree);


    data = "<list xmlns=\"urn:tests:types\"><id>a</id></list><list xmlns=\"urn:tests:types\"><id>b</id><value>x</value></list>"
           "<xdf:inst xmlns:xdf=\"urn:tests:types\">/xdf:list[xdf:id='b']/xdf:value</xdf:inst>";
    LYD_NODE_CREATE(data, tree);
    tree = tree->prev->prev;
    const enum ly_path_pred_type result_5[] = {LY_PATH_PREDTYPE_LIST, LY_PATH_PREDTYPE_NONE};
    TEST_PATTERN_1(tree, "inst", INST, "/types:list[id='b']/value", result_5);
    leaf = (struct lyd_node_term*)tree;
    for(int unsigned it = 0; it < LY_ARRAY_COUNT(leaf->value.target); it++){
        const char      *node_name[] = {"list", "value"};
        const uint16_t  node_type[] = {LYS_LIST, LYS_LEAF};
        LYSC_NODE_CHECK(leaf->value.target[it].node, node_type[it], node_name[it]);
    }
    LYD_NODE_DESTROY(tree);


    data = "<list_inst xmlns=\"urn:tests:types\"><id xmlns:b=\"urn:tests:types\">/b:leaflisttarget[.='a']</id><value>x</value></list_inst>"
           "<list_inst xmlns=\"urn:tests:types\"><id xmlns:b=\"urn:tests:types\">/b:leaflisttarget[.='b']</id><value>y</value></list_inst>"
           "<leaflisttarget xmlns=\"urn:tests:types\">a</leaflisttarget><leaflisttarget xmlns=\"urn:tests:types\">b</leaflisttarget>"
           "<a:inst xmlns:a=\"urn:tests:types\">/a:list_inst[a:id=\"/a:leaflisttarget[.='a']\"]/a:value</a:inst>";
    LYD_NODE_CREATE(data, tree);
    tree = tree->prev->prev;
    const enum ly_path_pred_type result_6[] = {LY_PATH_PREDTYPE_LIST, LY_PATH_PREDTYPE_NONE};
    TEST_PATTERN_1(tree, "inst", INST, "/types:list_inst[id=\"/types:leaflisttarget[.='a']\"]/value", result_6);
    leaf = (struct lyd_node_term*)tree;
    for(int unsigned it = 0; it < LY_ARRAY_COUNT(leaf->value.target); it++){
        const char      *node_name[] = {"list_inst", "value"};
        const uint16_t  node_type[] = {LYS_LIST, LYS_LEAF};
        LYSC_NODE_CHECK(leaf->value.target[it].node, node_type[it], node_name[it]);
    }
    LYD_NODE_DESTROY(tree);

    data = "<list_ident xmlns=\"urn:tests:types\"><id xmlns:dfs=\"urn:tests:defs\">dfs:ethernet</id><value>x</value></list_ident>"
           "<list_ident xmlns=\"urn:tests:types\"><id xmlns:dfs=\"urn:tests:defs\">dfs:fast-ethernet</id><value>y</value></list_ident>"
           "<a:inst xmlns:a=\"urn:tests:types\" xmlns:d=\"urn:tests:defs\">/a:list_ident[a:id='d:fast-ethernet']/a:value</a:inst>";
    LYD_NODE_CREATE(data, tree);
    tree = tree->prev->prev;
    const enum ly_path_pred_type result_7[] = {LY_PATH_PREDTYPE_LIST, LY_PATH_PREDTYPE_NONE};
    TEST_PATTERN_1(tree, "inst", INST, "/types:list_ident[id='defs:fast-ethernet']/value", result_7);
    leaf = (struct lyd_node_term*)tree;
    for(int unsigned it = 0; it < LY_ARRAY_COUNT(leaf->value.target); it++){
        const char      *node_name[] = {"list_ident", "value"};
        const uint16_t  node_type[] = {LYS_LIST, LYS_LEAF};
        LYSC_NODE_CHECK(leaf->value.target[it].node, node_type[it], node_name[it]);
    }
    LYD_NODE_DESTROY(tree);


    data = "<list2 xmlns=\"urn:tests:types\"><id>types:xxx</id><value>x</value></list2>"
           "<list2 xmlns=\"urn:tests:types\"><id>a:xxx</id><value>y</value></list2>"
           "<a:inst xmlns:a=\"urn:tests:types\">/a:list2[a:id='a:xxx'][a:value='y']/a:value</a:inst>";
    LYD_NODE_CREATE(data, tree);
    tree = tree->prev->prev;
    const enum ly_path_pred_type result_8[] = {LY_PATH_PREDTYPE_LIST, LY_PATH_PREDTYPE_NONE};
    TEST_PATTERN_1(tree, "inst", INST, "/types:list2[id='a:xxx'][value='y']/value", result_8);
    leaf = (struct lyd_node_term*)tree;
    for(int unsigned it = 0; it < LY_ARRAY_COUNT(leaf->value.target); it++){
        const char      *node_name[] = {"list2", "value"};
        const uint16_t  node_type[] = {LYS_LIST, LYS_LEAF};
        LYSC_NODE_CHECK(leaf->value.target[it].node, node_type[it], node_name[it]);
    }
    LYD_NODE_DESTROY(tree);

    data = "<list xmlns=\"urn:tests:types\"><id>types:xxx</id><value>x</value></list>"
           "<list xmlns=\"urn:tests:types\"><id>a:xxx</id><value>y</value></list>"
           "<a:inst xmlns:a=\"urn:tests:types\">/a:list[a:id='a:xxx']/a:value</a:inst>";
    LYD_NODE_CREATE(data, tree);
    tree = tree->prev->prev;
    const enum ly_path_pred_type result_9[] = {LY_PATH_PREDTYPE_LIST, LY_PATH_PREDTYPE_NONE};
    TEST_PATTERN_1(tree, "inst", INST, "/types:list[id='a:xxx']/value", result_9);
    leaf = (struct lyd_node_term*)tree;
    for(int unsigned it = 0; it < LY_ARRAY_COUNT(leaf->value.target); it++){
        const char      *node_name[] = {"list", "value"};
        const uint16_t  node_type[] = {LYS_LIST, LYS_LEAF};
        LYSC_NODE_CHECK(leaf->value.target[it].node, node_type[it], node_name[it]);
    }
    LYD_NODE_DESTROY(tree);

    data = "<list2 xmlns=\"urn:tests:types\"><id>a</id><value>a</value></list2>"
           "<list2 xmlns=\"urn:tests:types\"><id>c</id><value>b</value></list2>"
           "<list2 xmlns=\"urn:tests:types\"><id>a</id><value>b</value></list2>"
           "<a:inst xmlns:a=\"urn:tests:types\">/a:list2[a:id='a'][a:value='b']/a:id</a:inst>";
    LYD_NODE_CREATE(data, tree);
    tree = tree->prev->prev;
    const enum ly_path_pred_type result_10[] = {LY_PATH_PREDTYPE_LIST, LY_PATH_PREDTYPE_NONE};
    TEST_PATTERN_1(tree, "inst", INST, "/types:list2[id='a'][value='b']/id", result_10);
    leaf = (struct lyd_node_term*)tree;
    for(int unsigned it = 0; it < LY_ARRAY_COUNT(leaf->value.target); it++){
        const char      *node_name[] = {"list2", "id"};
        const uint16_t  node_type[] = {LYS_LIST, LYS_LEAF};
        LYSC_NODE_CHECK(leaf->value.target[it].node, node_type[it], node_name[it]);
    }
    assert_non_null(leaf = lyd_target(leaf->value.target, tree));
    assert_string_equal("a", leaf->value.canonical);
    assert_string_equal("b", ((struct lyd_node_term*)leaf->next)->value.canonical);
    LYD_NODE_DESTROY(tree);


    /* invalid value */
    data    = "<list xmlns=\"urn:tests:types\"><id>a</id></list><list xmlns=\"urn:tests:types\"><id>b</id><value>x</value></list>"
              "<xdf:inst xmlns:xdf=\"urn:tests:types\">/xdf:list[2]/xdf:value</xdf:inst>";
    err_msg[0] = "Invalid instance-identifier \"/xdf:list[2]/xdf:value\" value - semantic error.";
    err_path[0] = "/types:inst";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    data    = "<t:inst xmlns:t=\"urn:tests:types\">/t:cont/t:1leaftarget</t:inst>";
    err_msg[0] = "Invalid instance-identifier \"/t:cont/t:1leaftarget\" value - syntax error.";
    err_path[0] = "/types:inst";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    data    = "<t:inst xmlns:t=\"urn:tests:types\">/t:cont:t:1leaftarget</t:inst>";
    err_msg[0] = "Invalid instance-identifier \"/t:cont:t:1leaftarget\" value - syntax error.";
    err_path[0] = "/types:inst";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    data    = "<t:inst xmlns:t=\"urn:tests:types\">/t:cont/t:invalid/t:path</t:inst>";
    err_msg[0] = "Invalid instance-identifier \"/t:cont/t:invalid/t:path\" value - semantic error.";
    err_path[0] = "/types:inst";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    data    = "<inst xmlns=\"urn:tests:types\" xmlns:t=\"urn:tests:invalid\">/t:cont/t:leaftarget</inst>";
    err_msg[0] = "Invalid instance-identifier \"/t:cont/t:leaftarget\" value - semantic error.";
    err_path[0] = "/types:inst";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    error_msg =  "Invalid instance-identifier \"/cont/leaftarget\" value - syntax error.";;
    TEST_TYPE_ERROR("inst", "/cont/leaftarget", error_msg);

//    /* instance-identifier is here in JSON format because it is already in internal representation without canonical prefixes */
    data    = "<cont xmlns=\"urn:tests:types\"/><t:inst xmlns:t=\"urn:tests:types\">/t:cont/t:leaftarget</t:inst>";
    err_msg[0] = "Invalid instance-identifier \"/types:cont/leaftarget\" value - required instance not found.";
    err_path[0] = "/types:inst";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_ENOTFOUND, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    /* instance-identifier is here in JSON format because it is already in internal representation without canonical prefixes */

    data    = "<t:inst xmlns:t=\"urn:tests:types\">/t:cont/t:leaftarget</t:inst>";
    err_msg[0] = "Invalid instance-identifier \"/types:cont/leaftarget\" value - required instance not found.";
    err_path[0] = "/types:inst";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_ENOTFOUND, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    data    = "<leaflisttarget xmlns=\"urn:tests:types\">x</leaflisttarget><t:inst xmlns:t=\"urn:tests:types\">/t:leaflisttarget[1</t:inst>";
    err_msg[0] = "Invalid instance-identifier \"/t:leaflisttarget[1\" value - syntax error.";
    err_path[0] = "/types:inst";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    data    = "<cont xmlns=\"urn:tests:types\"/><t:inst xmlns:t=\"urn:tests:types\">/t:cont[1]</t:inst>";
    err_msg[0] = "Invalid instance-identifier \"/t:cont[1]\" value - semantic error.";
    err_path[0] = "/types:inst";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    data    = "<cont xmlns=\"urn:tests:types\"/><t:inst xmlns:t=\"urn:tests:types\">[1]</t:inst>";
    err_msg[0] = "Invalid instance-identifier \"[1]\" value - syntax error.";
    err_path[0] = "/types:inst";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    data    = "<cont xmlns=\"urn:tests:types\"><leaflisttarget>1</leaflisttarget></cont><t:inst xmlns:t=\"urn:tests:types\">/t:cont/t:leaflisttarget[id='1']</t:inst>";
    err_msg[0] = "Invalid instance-identifier \"/t:cont/t:leaflisttarget[id='1']\" value - syntax error.";
    err_path[0] = "/types:inst";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    data    = "<cont xmlns=\"urn:tests:types\"><leaflisttarget>1</leaflisttarget></cont>"
              "<t:inst xmlns:t=\"urn:tests:types\">/t:cont/t:leaflisttarget[t:id='1']</t:inst>";
    err_msg[0] = "Invalid instance-identifier \"/t:cont/t:leaflisttarget[t:id='1']\" value - semantic error.";
    err_path[0] = "/types:inst";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    data    = "<cont xmlns=\"urn:tests:types\"><leaflisttarget>1</leaflisttarget><leaflisttarget>2</leaflisttarget></cont>"
              "<t:inst xmlns:t=\"urn:tests:types\">/t:cont/t:leaflisttarget[4]</t:inst>";
    err_msg[0] = "Invalid instance-identifier \"/t:cont/t:leaflisttarget[4]\" value - semantic error.";
    err_path[0] = "/types:inst";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    data    = "<t:inst-noreq xmlns:t=\"urn:tests:types\">/t:cont/t:leaflisttarget[6]</t:inst-noreq>";
    err_msg[0] = "Invalid instance-identifier \"/t:cont/t:leaflisttarget[6]\" value - semantic error.";
    err_path[0] = "/types:inst-noreq";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    data    = "<cont xmlns=\"urn:tests:types\"><listtarget><id>1</id><value>x</value></listtarget></cont>"
              "<t:inst xmlns:t=\"urn:tests:types\">/t:cont/t:listtarget[t:value='x']</t:inst>";
    err_msg[0] = "Invalid instance-identifier \"/t:cont/t:listtarget[t:value='x']\" value - semantic error.";
    err_path[0] = "/types:inst";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    data    = "<t:inst-noreq xmlns:t=\"urn:tests:types\">/t:cont/t:listtarget[t:value='x']</t:inst-noreq>";
    err_msg[0] = "Invalid instance-identifier \"/t:cont/t:listtarget[t:value='x']\" value - semantic error.";
    err_path[0] = "/types:inst-noreq";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    data    = "<t:inst-noreq xmlns:t=\"urn:tests:types\">/t:cont/t:listtarget[t:x='x']</t:inst-noreq>";
    err_msg[0] = "Invalid instance-identifier \"/t:cont/t:listtarget[t:x='x']\" value - semantic error.";
    err_path[0] = "/types:inst-noreq";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_ENOTFOUND, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    data    = "<cont xmlns=\"urn:tests:types\"><listtarget><id>1</id><value>x</value></listtarget></cont>"
              "<t:inst xmlns:t=\"urn:tests:types\">/t:cont/t:listtarget[.='x']</t:inst>";
    err_msg[0] = "Invalid instance-identifier \"/t:cont/t:listtarget[.='x']\" value - semantic error.";
    err_path[0] = "/types:inst";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    /* instance-identifier is here in JSON format because it is already in internal representation without canonical prefixes */
    data    = "<cont xmlns=\"urn:tests:types\"><leaflisttarget>1</leaflisttarget></cont>"
              "<t:inst xmlns:t=\"urn:tests:types\">/t:cont/t:leaflisttarget[.='2']</t:inst>";
    err_msg[0] = "Invalid instance-identifier \"/types:cont/leaflisttarget[.='2']\" value - required instance not found.";
    err_path[0] = "/types:inst";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_ENOTFOUND, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    data    = "<cont xmlns=\"urn:tests:types\"><leaflisttarget>1</leaflisttarget></cont>"
              "<t:inst xmlns:t=\"urn:tests:types\">/t:cont/t:leaflisttarget[.='x']</t:inst>";
    err_msg[0] = "Invalid instance-identifier \"/t:cont/t:leaflisttarget[.='x']\" value - semantic error.";
    err_path[0] = "/types:inst";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    data    = "<cont xmlns=\"urn:tests:types\"><listtarget><id>1</id><value>x</value></listtarget></cont>"
              "<t:inst xmlns:t=\"urn:tests:types\">/t:cont/t:listtarget[t:id='x']</t:inst>";
    err_msg[0] = "Invalid instance-identifier \"/t:cont/t:listtarget[t:id='x']\" value - semantic error.";
    err_path[0] = "/types:inst";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    /* instance-identifier is here in JSON format because it is already in internal representation without canonical prefixes */
    data    = "<cont xmlns=\"urn:tests:types\"><listtarget><id>1</id><value>x</value></listtarget></cont>"
              "<t:inst xmlns:t=\"urn:tests:types\">/t:cont/t:listtarget[t:id='2']</t:inst>";
    err_msg[0] = "Invalid instance-identifier \"/types:cont/listtarget[id='2']\" value - required instance not found.";
    err_path[0] = "/types:inst";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_ENOTFOUND, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    data    = "<leaflisttarget xmlns=\"urn:tests:types\">a</leaflisttarget>"
              "<leaflisttarget xmlns=\"urn:tests:types\">b</leaflisttarget>"
              "<a:inst xmlns:a=\"urn:tests:types\">/a:leaflisttarget[1][2]</a:inst>";
    err_msg[0] = "Invalid instance-identifier \"/a:leaflisttarget[1][2]\" value - syntax error.";
    err_path[0] = "/types:inst";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    data    = "<leaflisttarget xmlns=\"urn:tests:types\">a</leaflisttarget>"
              "<leaflisttarget xmlns=\"urn:tests:types\">b</leaflisttarget>"
              "<a:inst xmlns:a=\"urn:tests:types\">/a:leaflisttarget[.='a'][.='b']</a:inst>";
    err_msg[0] = "Invalid instance-identifier \"/a:leaflisttarget[.='a'][.='b']\" value - syntax error.";
    err_path[0] = "/types:inst";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    data    = "<list xmlns=\"urn:tests:types\"><id>a</id><value>x</value></list>"
              "<list xmlns=\"urn:tests:types\"><id>b</id><value>y</value></list>"
              "<a:inst xmlns:a=\"urn:tests:types\">/a:list[a:id='a'][a:id='b']/a:value</a:inst>";
    err_msg[0] = "Invalid instance-identifier \"/a:list[a:id='a'][a:id='b']/a:value\" value - syntax error.";
    err_path[0] = "/types:inst";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    data    = "<list2 xmlns=\"urn:tests:types\"><id>a</id><value>x</value></list2>"
              "<list2 xmlns=\"urn:tests:types\"><id>b</id><value>y</value></list2>"
              "<a:inst xmlns:a=\"urn:tests:types\">/a:list2[a:id='a']/a:value</a:inst>";
    err_msg[0] = "Invalid instance-identifier \"/a:list2[a:id='a']/a:value\" value - semantic error.";
    err_path[0] = "/types:inst";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

//    /* check for validting instance-identifier with a complete data tree */
    data    = "<list2 xmlns=\"urn:tests:types\"><id>a</id><value>a</value></list2>"
              "<list2 xmlns=\"urn:tests:types\"><id>c</id><value>b</value></list2>"
              "<leaflisttarget xmlns=\"urn:tests:types\">a</leaflisttarget>"
              "<leaflisttarget xmlns=\"urn:tests:types\">b</leaflisttarget>"
              "<a:inst xmlns:a=\"urn:tests:types\">/a:list2[a:id='a'][a:value='a']/a:id</a:inst>";
    LYD_NODE_CREATE(data, tree);
    /* key-predicate */
    data = "/types:list2[id='a'][value='b']/id";
    assert_int_equal(LY_ENOTFOUND, lyd_value_validate(CONTEXT_GET, (const struct lyd_node_term*)tree->prev->prev, data, strlen(data),
                                                   tree, NULL));
    err_msg[0] = "Invalid instance-identifier \"/types:list2[id='a'][value='b']/id\" value - required instance not found.";
    err_path[0] = "/types:inst";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    /* leaf-list-predicate */
    data = "/types:leaflisttarget[.='c']";
    assert_int_equal(LY_ENOTFOUND, lyd_value_validate(CONTEXT_GET, (const struct lyd_node_term*)tree->prev->prev, data, strlen(data),
                                                   tree, NULL));
    err_msg[0] = "Invalid instance-identifier \"/types:leaflisttarget[.='c']\" value - required instance not found.";
    err_path[0] = "/types:inst";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    /* position predicate */
    data = "/types:list_keyless[4]";
    assert_int_equal(LY_ENOTFOUND, lyd_value_validate(CONTEXT_GET, (const struct lyd_node_term*)tree->prev->prev, data, strlen(data),
                                                   tree, NULL));
    err_msg[0] = "Invalid instance-identifier \"/types:list_keyless[4]\" value - required instance not found.";
    err_path[0] = "/types:inst";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    LYD_NODE_DESTROY(tree);

    data    = "<leaflisttarget xmlns=\"urn:tests:types\">b</leaflisttarget>"
            "<inst xmlns=\"urn:tests:types\">/a:leaflisttarget[1]</inst>";
    err_msg[0] = "Invalid instance-identifier \"/a:leaflisttarget[1]\" value - semantic error.";
    err_path[0] = "/types:inst";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    CONTEXT_DESTROY;
}

static void
test_leafref(void **state)
{

    (void) state;
    struct lyd_node *tree;
    struct lyd_node_term *leaf;

    const char *data;
    char *err_msg[1];
    char *err_path[1];

    const struct lys_module *mod_types;
    const struct lys_module *mod_defs;

    CONTEXT_CREATE(mod_defs, mod_types);

    /* types:lref: /leaflisttarget */
    /* types:lref2: ../list[id = current()/../str-norestr]/targets */

    const char *schema = "module leafrefs {yang-version 1.1; namespace urn:tests:leafrefs; prefix lr; import types {prefix t;}"
            "container c { container x {leaf x {type string;}} list l {key \"id value\"; leaf id {type string;} leaf value {type string;}"
                "leaf lr1 {type leafref {path \"../../../t:str-norestr\"; require-instance true;}}"
                "leaf lr2 {type leafref {path \"../../l[id=current()/../../../t:str-norestr][value=current()/../../../t:str-norestr]/value\"; require-instance true;}}"
                "leaf lr3 {type leafref {path \"/t:list[t:id=current ( )/../../x/x]/t:targets\";}}"
            "}}}";

    /* additional schema */
    assert_int_equal(LY_SUCCESS, lys_parse_mem(CONTEXT_GET, schema, LYS_IN_YANG, NULL));

    /* valid data */
    data =  "<leaflisttarget xmlns=\"urn:tests:types\">x</leaflisttarget><leaflisttarget xmlns=\"urn:tests:types\">y</leaflisttarget><lref xmlns=\"urn:tests:types\">y</lref>";
    LYD_NODE_CREATE(data, tree);
    tree = tree->prev->prev;
    LYSC_NODE_CHECK(tree->schema, LYS_LEAF, "lref");
    leaf = (struct lyd_node_term*)tree;
    LYD_NODE_TERM_CHECK(leaf, STRING, "y");
    LYD_NODE_DESTROY(tree);

    data = "<list xmlns=\"urn:tests:types\"><id>x</id><targets>a</targets><targets>b</targets></list>"
              "<list xmlns=\"urn:tests:types\"><id>y</id><targets>x</targets><targets>y</targets></list>"
              "<str-norestr xmlns=\"urn:tests:types\">y</str-norestr><lref2 xmlns=\"urn:tests:types\">y</lref2>";
    LYD_NODE_CREATE(data, tree);
    tree = tree->prev->prev;
    LYSC_NODE_CHECK(tree->schema, LYS_LEAF, "lref2");
    leaf = (struct lyd_node_term*)tree;
    LYD_NODE_TERM_CHECK(leaf, STRING, "y");
    LYD_NODE_DESTROY(tree);

    data = "<str-norestr xmlns=\"urn:tests:types\">y</str-norestr>"
           "<c xmlns=\"urn:tests:leafrefs\"><l><id>x</id><value>x</value><lr1>y</lr1></l></c>";
    LYD_NODE_CREATE(data, tree);
    LYSC_NODE_CHECK(tree->schema, LYS_CONTAINER, "c");
    leaf = (struct lyd_node_term*)(lyd_child(lyd_child(tree)->next)->prev);
    LYSC_NODE_CHECK(leaf->schema, LYS_LEAF, "lr1");
    LYD_NODE_TERM_CHECK(leaf, STRING, "y");
    LYD_NODE_DESTROY(tree);

    data = "<str-norestr xmlns=\"urn:tests:types\">y</str-norestr>"
           "<c xmlns=\"urn:tests:leafrefs\"><l><id>y</id><value>y</value></l>"
           "<l><id>x</id><value>x</value><lr2>y</lr2></l></c>";
    LYD_NODE_CREATE(data, tree);
    LYSC_NODE_CHECK(tree->schema, LYS_CONTAINER, "c");
    leaf = (struct lyd_node_term*)(lyd_child(lyd_child(tree)->prev)->prev);
    LYSC_NODE_CHECK(leaf->schema, LYS_LEAF, "lr2");
    LYD_NODE_TERM_CHECK(leaf, STRING, "y");
    LYD_NODE_DESTROY(tree);


    data = "<list xmlns=\"urn:tests:types\"><id>x</id><targets>a</targets><targets>b</targets></list>"
           "<list xmlns=\"urn:tests:types\"><id>y</id><targets>c</targets><targets>d</targets></list>"
           "<c xmlns=\"urn:tests:leafrefs\"><x><x>y</x></x>"
           "<l><id>x</id><value>x</value><lr3>c</lr3></l></c>";
    LYD_NODE_CREATE(data, tree);
    LYSC_NODE_CHECK(tree->schema, LYS_CONTAINER, "c");
    leaf = (struct lyd_node_term*)(lyd_child(lyd_child(tree)->prev)->prev);
    LYSC_NODE_CHECK(leaf->schema, LYS_LEAF, "lr3");
    LYD_NODE_TERM_CHECK(leaf, STRING, "c");
    LYD_NODE_DESTROY(tree);

//    /* invalid value */
    data    = "<leaflisttarget xmlns=\"urn:tests:types\">x</leaflisttarget><lref xmlns=\"urn:tests:types\">y</lref>";
    err_msg[0] = "Invalid leafref value \"y\" - no target instance \"/leaflisttarget\" with the same value.";
    err_path[0] = "/types:lref";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    data    = "<list xmlns=\"urn:tests:types\"><id>x</id><targets>a</targets><targets>b</targets></list>"
              "<list xmlns=\"urn:tests:types\"><id>y</id><targets>x</targets><targets>y</targets></list>"
              "<str-norestr xmlns=\"urn:tests:types\">y</str-norestr><lref2 xmlns=\"urn:tests:types\">b</lref2>";
    err_msg[0] = "Invalid leafref value \"b\" - no target instance \"../list[id = current()/../str-norestr]/targets\" with the same value.";
    err_path[0] = "/types:lref2";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    data    = "<list xmlns=\"urn:tests:types\"><id>x</id><targets>a</targets><targets>b</targets></list>"
              "<list xmlns=\"urn:tests:types\"><id>y</id><targets>x</targets><targets>y</targets></list>"
              "<lref2 xmlns=\"urn:tests:types\">b</lref2>";
    err_msg[0] = "Invalid leafref value \"b\" - no target instance \"../list[id = current()/../str-norestr]/targets\" with the same value.";
    err_path[0] = "/types:lref2";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    data    = "<str-norestr xmlns=\"urn:tests:types\">y</str-norestr><lref2 xmlns=\"urn:tests:types\">b</lref2>";
    err_msg[0] = "Invalid leafref value \"b\" - no target instance \"../list[id = current()/../str-norestr]/targets\" with the same value.";
    err_path[0] = "/types:lref2";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    data    = "<str-norestr xmlns=\"urn:tests:types\">y</str-norestr>"
              "<c xmlns=\"urn:tests:leafrefs\"><l><id>x</id><value>x</value><lr1>a</lr1></l></c>";
    err_msg[0] = "Invalid leafref value \"a\" - no target instance \"../../../t:str-norestr\" with the same value.";
    err_path[0] = "/leafrefs:c/l[id='x'][value='x']/lr1";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    data    = "<str-norestr xmlns=\"urn:tests:types\">z</str-norestr>"
              "<c xmlns=\"urn:tests:leafrefs\"><l><id>y</id><value>y</value></l>"
              "<l><id>x</id><value>x</value><lr2>z</lr2></l></c>";
    err_msg[0] = "Invalid leafref value \"z\" - no target instance \"../../l[id=current()/../../../t:str-norestr]"
              "[value=current()/../../../t:str-norestr]/value\" with the same value.";
    err_path[0] = "/leafrefs:c/l[id='x'][value='x']/lr2";
    LYD_NODE_CREATE_PARAM(data, LYD_XML, 0, LYD_VALIDATE_PRESENT, LY_EVALID, tree);
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);


    CONTEXT_DESTROY;
}

static void
test_union(void **state)
{
    (void) state;

    struct lyd_node *tree;
    struct lyd_node_term *leaf;

    const char *data;
    char *error_msg;
    const struct lys_module *mod_types;
    const struct lys_module *mod_defs;

    CONTEXT_CREATE(mod_defs, mod_types);

    /*
     * leaf un1 {type union {
     *             type leafref {path /int8; require-instance true;}
     *             type union {
     *               type identityref {base defs:interface-type;}
     *               type instance-identifier {require-instance true;}
     *             }
     *             type string {range 1..20;};
     *           }
     * }
     */

    /* valid data */
    data = "<int8 xmlns=\"urn:tests:types\">12</int8><un1 xmlns=\"urn:tests:types\">12</un1>";
    LYD_NODE_CREATE(data,tree);
    tree = tree->next;
    TEST_PATTERN_1(tree,"un1", UNION, "12", INT8, "12", 12);
    leaf = (struct lyd_node_term*)tree;
    assert_int_equal(((struct ly_set *)leaf->value.subvalue->prefix_data)->count, 0);
    test_printed_value(&leaf->value, "12", LY_PREF_SCHEMA, NULL);
    LYD_NODE_DESTROY(tree);


    data = "<int8 xmlns=\"urn:tests:types\">12</int8><un1 xmlns=\"urn:tests:types\">2</un1>";
    LYD_NODE_CREATE(data,tree);
    tree = tree->next;
    TEST_PATTERN_1(tree,"un1", UNION, "2", STRING, "2");
    leaf = (struct lyd_node_term*)tree;
    assert_int_equal(((struct ly_set *)leaf->value.subvalue->prefix_data)->count, 0);
    test_printed_value(&leaf->value, "2", LY_PREF_SCHEMA, NULL);
    LYD_NODE_DESTROY(tree);

    data = "<un1 xmlns=\"urn:tests:types\" xmlns:x=\"urn:tests:defs\">x:fast-ethernet</un1>";
    LYD_NODE_CREATE(data,tree);
    TEST_PATTERN_1(tree,"un1", UNION, "defs:fast-ethernet", IDENT, "defs:fast-ethernet", "fast-ethernet");
    leaf = (struct lyd_node_term*)tree;
    assert_int_equal(((struct ly_set *)leaf->value.subvalue->prefix_data)->count, 1);
    test_printed_value(&leaf->value, "d:fast-ethernet", LY_PREF_SCHEMA, mod_defs->parsed);
    test_printed_value(&leaf->value.subvalue->value, "d:fast-ethernet", LY_PREF_SCHEMA, mod_defs->parsed);
    LYD_NODE_DESTROY(tree);

    data = "<un1 xmlns=\"urn:tests:types\" xmlns:d=\"urn:tests:defs\">d:superfast-ethernet</un1>";
    LYD_NODE_CREATE(data,tree);
    TEST_PATTERN_1(tree,"un1", UNION, "d:superfast-ethernet", STRING, "d:superfast-ethernet");
    leaf = (struct lyd_node_term*)tree;
    assert_int_equal(((struct ly_set *)leaf->value.subvalue->prefix_data)->count, 1);
    LYD_NODE_DESTROY(tree);

    data = "<leaflisttarget xmlns=\"urn:tests:types\">x</leaflisttarget><leaflisttarget xmlns=\"urn:tests:types\">y</leaflisttarget>"
           "<un1 xmlns=\"urn:tests:types\" xmlns:a=\"urn:tests:types\">/a:leaflisttarget[.='y']</un1>";
    LYD_NODE_CREATE(data,tree);
    tree = tree->prev->prev;
    const enum ly_path_pred_type result_1[] = {LY_PATH_PREDTYPE_LEAFLIST};
    TEST_PATTERN_1(tree, "un1", UNION, "/types:leaflisttarget[.='y']", INST, "/types:leaflisttarget[.='y']", result_1);
    leaf = (struct lyd_node_term*)tree;

    assert_int_equal(((struct ly_set *)leaf->value.subvalue->prefix_data)->count, 1);
    LYD_NODE_DESTROY(tree);

    data = "<leaflisttarget xmlns=\"urn:tests:types\">x</leaflisttarget><leaflisttarget xmlns=\"urn:tests:types\">y</leaflisttarget>"
           "<un1 xmlns=\"urn:tests:types\" xmlns:a=\"urn:tests:types\">/a:leaflisttarget[3]</un1>";
    LYD_NODE_CREATE(data,tree);
    tree = tree->prev->prev;
    TEST_PATTERN_1(tree, "un1", UNION, "/a:leaflisttarget[3]", STRING, "/a:leaflisttarget[3]");
    leaf = (struct lyd_node_term*)tree;
    assert_int_equal(((struct ly_set *)leaf->value.subvalue->prefix_data)->count, 1);
    LYD_NODE_DESTROY(tree);

    error_msg =  "Invalid union value \"123456789012345678901\" - no matching subtype found.";;
    TEST_TYPE_ERROR("un1", "123456789012345678901", error_msg);

    CONTEXT_DESTROY;
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_int),
        cmocka_unit_test(test_uint),
        cmocka_unit_test(test_dec64),
        cmocka_unit_test(test_string),
        cmocka_unit_test(test_bits),
        cmocka_unit_test(test_enums),
        cmocka_unit_test(test_binary),
        cmocka_unit_test(test_boolean),
        cmocka_unit_test(test_empty),
        cmocka_unit_test(test_identityref),
        cmocka_unit_test(test_instanceid),
        cmocka_unit_test(test_leafref),
        cmocka_unit_test(test_union),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
