/*
 * @file test_parser_yang.c
 * @author: Radek Krejci <rkrejci@cesnet.cz>
 * @brief unit tests for functions from parser_yang.c
 *
 * Copyright (c) 2018 CESNET, z.s.p.o.
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

#include "common.h"
#include "in_internal.h"
#include "parser_internal.h"
#include "tree_schema.h"
#include "tree_schema_internal.h"

/* originally static functions from tree_schema_free.c and parser_yang.c */
void lysp_ext_instance_free(struct ly_ctx *ctx, struct lysp_ext_instance *ext);
void lysp_deviation_free(struct ly_ctx *ctx, struct lysp_deviation *dev);
void lysp_grp_free(struct ly_ctx *ctx, struct lysp_grp *grp);
void lysp_action_free(struct ly_ctx *ctx, struct lysp_action *action);
void lysp_notif_free(struct ly_ctx *ctx, struct lysp_notif *notif);
void lysp_augment_free(struct ly_ctx *ctx, struct lysp_augment *augment);
void lysp_deviate_free(struct ly_ctx *ctx, struct lysp_deviate *d);
void lysp_node_free(struct ly_ctx *ctx, struct lysp_node *node);
void lysp_when_free(struct ly_ctx *ctx, struct lysp_when *when);

LY_ERR buf_add_char(struct ly_ctx *ctx, struct ly_in *in, size_t len, char **buf, size_t *buf_len, size_t *buf_used);
LY_ERR buf_store_char(struct lys_yang_parser_ctx *ctx, struct ly_in *in, enum yang_arg arg, char **word_p,
                      size_t *word_len, char **word_b, size_t *buf_len, uint8_t need_buf, uint8_t *prefix);
LY_ERR get_keyword(struct lys_yang_parser_ctx *ctx, struct ly_in *in, enum ly_stmt *kw, char **word_p, size_t *word_len);
LY_ERR get_argument(struct lys_yang_parser_ctx *ctx, struct ly_in *in, enum yang_arg arg,
                    uint16_t *flags, char **word_p, char **word_b, size_t *word_len);
LY_ERR skip_comment(struct lys_yang_parser_ctx *ctx, struct ly_in *in, uint8_t comment);

LY_ERR parse_action(struct lys_yang_parser_ctx *ctx, struct ly_in *in, struct lysp_node *parent, struct lysp_action **actions);
LY_ERR parse_any(struct lys_yang_parser_ctx *ctx, struct ly_in *in, enum ly_stmt kw, struct lysp_node *parent, struct lysp_node **siblings);
LY_ERR parse_augment(struct lys_yang_parser_ctx *ctx, struct ly_in *in, struct lysp_node *parent, struct lysp_augment **augments);
LY_ERR parse_case(struct lys_yang_parser_ctx *ctx, struct ly_in *in, struct lysp_node *parent, struct lysp_node **siblings);
LY_ERR parse_container(struct lys_yang_parser_ctx *ctx, struct ly_in *in, struct lysp_node *parent, struct lysp_node **siblings);
LY_ERR parse_deviate(struct lys_yang_parser_ctx *ctx, struct ly_in *in, struct lysp_deviate **deviates);
LY_ERR parse_deviation(struct lys_yang_parser_ctx *ctx, struct ly_in *in, struct lysp_deviation **deviations);
LY_ERR parse_grouping(struct lys_yang_parser_ctx *ctx, struct ly_in *in, struct lysp_node *parent, struct lysp_grp **groupings);
LY_ERR parse_choice(struct lys_yang_parser_ctx *ctx, struct ly_in *in, struct lysp_node *parent, struct lysp_node **siblings);
LY_ERR parse_leaf(struct lys_yang_parser_ctx *ctx, struct ly_in *in, struct lysp_node *parent, struct lysp_node **siblings);
LY_ERR parse_leaflist(struct lys_yang_parser_ctx *ctx, struct ly_in *in, struct lysp_node *parent, struct lysp_node **siblings);
LY_ERR parse_list(struct lys_yang_parser_ctx *ctx, struct ly_in *in, struct lysp_node *parent, struct lysp_node **siblings);
LY_ERR parse_maxelements(struct lys_yang_parser_ctx *ctx, struct ly_in *in, uint32_t *max, uint16_t *flags, struct lysp_ext_instance **exts);
LY_ERR parse_minelements(struct lys_yang_parser_ctx *ctx, struct ly_in *in, uint32_t *min, uint16_t *flags, struct lysp_ext_instance **exts);
LY_ERR parse_module(struct lys_yang_parser_ctx *ctx, struct ly_in *in, struct lysp_module *mod);
LY_ERR parse_notif(struct lys_yang_parser_ctx *ctx, struct ly_in *in, struct lysp_node *parent, struct lysp_notif **notifs);
LY_ERR parse_submodule(struct lys_yang_parser_ctx *ctx, struct ly_in *in, struct lysp_submodule *submod);
LY_ERR parse_uses(struct lys_yang_parser_ctx *ctx, struct ly_in *in, struct lysp_node *parent, struct lysp_node **siblings);
LY_ERR parse_when(struct lys_yang_parser_ctx *ctx, struct ly_in *in, struct lysp_when **when_p);
LY_ERR parse_type_enum_value_pos(struct lys_yang_parser_ctx *ctx, struct ly_in *in, enum ly_stmt val_kw, int64_t *value, uint16_t *flags, struct lysp_ext_instance **exts);



#define logbuf_assert(str)\
    {\
        const char * err_msg[]  = {str};\
        const char * err_path[] = {NULL};\
        LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);\
    }


#define PARSER_CREATE(PARSER_CTX)\
            ly_set_log_clb(logger_null, 1);\
            PARSER_CTX = calloc(1, sizeof *PARSER_CTX);\
            PARSER_CTX->format = LYS_IN_YANG;\
            PARSER_CTX->pos_type = LY_VLOG_LINE;\
            PARSER_CTX->line = 1;\
            PARSER_CTX->parsed_mod = calloc(1, sizeof *PARSER_CTX->parsed_mod);\
            PARSER_CTX->parsed_mod->mod = calloc(1, sizeof *PARSER_CTX->parsed_mod->mod);\
            PARSER_CTX->parsed_mod->mod->parsed = PARSER_CTX->parsed_mod;\
            CONTEXT_CREATE_PATH(NULL);\
            PARSER_CTX->parsed_mod->mod->ctx = CONTEXT_GET;

#define PARSER_DESTROY(PARSER_CTX)\
            lys_module_free(PARSER_CTX->parsed_mod->mod, NULL);\
            CONTEXT_DESTROY;\
            free(PARSER_CTX);


#define TEST_DUP_GENERIC(PREFIX, MEMBER, VALUE1, VALUE2, FUNC, RESULT, LINE, CLEANUP) \
    in.current = PREFIX MEMBER" "VALUE1";"MEMBER" "VALUE2";} ..."; \
    assert_int_equal(LY_EVALID, FUNC(ctx, &in, RESULT)); \
    {\
        const char *err_msg[]    = {"Duplicate keyword \""MEMBER"\"."};\
        const char *err_path[]   = {"Line number "LINE"."};\
        LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);\
    }\
    CLEANUP

static void
test_helpers(void **state)
{

    (void) state;
    struct ly_in in = {0};
    char *buf, *p;
    size_t len, size;
    struct lys_yang_parser_ctx *ctx;
    uint8_t prefix = 0;
    char *err_msg[1];
    char *err_path[1];

    PARSER_CREATE(ctx);

    /* storing into buffer */
    in.current = "abcd";
    buf = NULL;
    size = len = 0;
    assert_int_equal(LY_SUCCESS, buf_add_char(NULL, &in, 2, &buf, &size, &len));
    assert_int_not_equal(0, size);
    assert_int_equal(2, len);
    assert_string_equal("cd", in.current);
    assert_false(strncmp("ab", buf, 2));
    free(buf);
    buf = NULL;


    /* invalid first characters */
    len = 0;
    in.current = "2invalid";
    assert_int_equal(LY_EVALID, buf_store_char(ctx, &in, Y_IDENTIF_ARG, &p, &len, &buf, &size, 1, &prefix));
    in.current = ".invalid";
    assert_int_equal(LY_EVALID, buf_store_char(ctx, &in, Y_IDENTIF_ARG, &p, &len, &buf, &size, 1, &prefix));
    in.current = "-invalid";
    assert_int_equal(LY_EVALID, buf_store_char(ctx, &in, Y_IDENTIF_ARG, &p, &len, &buf, &size, 1, &prefix));
    /* invalid following characters */
    len = 3; /* number of characters read before the str content */
    in.current = "!";
    assert_int_equal(LY_EVALID, buf_store_char(ctx, &in, Y_IDENTIF_ARG, &p, &len, &buf, &size, 1, &prefix));
    in.current = ":";
    assert_int_equal(LY_EVALID, buf_store_char(ctx, &in, Y_IDENTIF_ARG, &p, &len, &buf, &size, 1, &prefix));
    /* valid colon for prefixed identifiers */
    len = size = 0;
    p = NULL;
    prefix = 0;
    in.current = "x:id";
    assert_int_equal(LY_SUCCESS, buf_store_char(ctx, &in, Y_PREF_IDENTIF_ARG, &p, &len, &buf, &size, 0, &prefix));
    assert_int_equal(1, len);
    assert_null(buf);
    assert_string_equal(":id", in.current);
    assert_int_equal('x', p[len - 1]);
    assert_int_equal(LY_SUCCESS, buf_store_char(ctx, &in, Y_PREF_IDENTIF_ARG, &p, &len, &buf, &size, 1, &prefix));
    assert_int_equal(2, len);
    assert_string_equal("id", in.current);
    assert_int_equal(':', p[len - 1]);
    free(buf);
    prefix = 0;

    /* checking identifiers */
    assert_int_equal(LY_EVALID, lysp_check_identifierchar((struct lys_parser_ctx *)ctx, ':', 0, NULL));
    err_msg[0] = "Invalid identifier character ':' (0x003a).";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    assert_int_equal(LY_EVALID, lysp_check_identifierchar((struct lys_parser_ctx *)ctx, '#', 1, NULL));
    err_msg[0] = "Invalid identifier first character '#' (0x0023).";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    assert_int_equal(LY_SUCCESS, lysp_check_identifierchar((struct lys_parser_ctx *)ctx, 'a', 1, &prefix));
    assert_int_equal(0, prefix);
    assert_int_equal(LY_SUCCESS, lysp_check_identifierchar((struct lys_parser_ctx *)ctx, ':', 0, &prefix));
    assert_int_equal(1, prefix);
    assert_int_equal(LY_EVALID, lysp_check_identifierchar((struct lys_parser_ctx *)ctx, ':', 0, &prefix));
    assert_int_equal(1, prefix);
    assert_int_equal(LY_SUCCESS, lysp_check_identifierchar((struct lys_parser_ctx *)ctx, 'b', 0, &prefix));
    assert_int_equal(2, prefix);
    /* second colon is invalid */
    assert_int_equal(LY_EVALID, lysp_check_identifierchar((struct lys_parser_ctx *)ctx, ':', 0, &prefix));
    err_msg[0] = "Invalid identifier character ':' (0x003a).";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    PARSER_DESTROY(ctx);
}


#define TEST_GET_ARGUMENT_SUCCESS(INPUT_TEXT, CTX, ARG_TYPE, EXPECT_WORD, EXPECT_LEN, EXPECT_CURRENT)\
                    {\
                        const char * text  = INPUT_TEXT;\
                        in.current = text;\
                        assert_int_equal(LY_SUCCESS, get_argument(CTX, &in, Y_MAYBE_STR_ARG, NULL, &word, &buf, &len));\
                        assert_string_equal(word, EXPECT_WORD);\
                        assert_int_equal(len, EXPECT_LEN);\
                        assert_string_equal(EXPECT_CURRENT, in.current);\
                    }

static void
test_comments(void **state)
{
    (void) state;
    struct ly_in in = {0};
    struct lys_yang_parser_ctx *ctx;
    char *word, *buf;
    size_t len;
    const char * in_text;
    char *err_msg[1];
    char *err_path[1];

    PARSER_CREATE(ctx);

    //in.current = " // this is a text of / one * line */ comment\nargument;";
    in_text = " // this is a text of / one * line */ comment\nargument;";
    TEST_GET_ARGUMENT_SUCCESS(in_text, ctx, Y_STR_ARG, "argument;", 8, ";");
    assert_null(buf);

    in_text = "/* this is a \n * text // of / block * comment */\"arg\" + \"ume\" \n + \n \"nt\";";
    TEST_GET_ARGUMENT_SUCCESS(in_text, ctx, Y_STR_ARG, "argument", 8, ";");
    assert_ptr_equal(buf, word);
    free(word);

    in.current = " this is one line comment on last line";
    assert_int_equal(LY_SUCCESS, skip_comment(ctx, &in, 1));
    assert_true(in.current[0] == '\0');

    in.current = " this is a not terminated comment x";
    assert_int_equal(LY_EVALID, skip_comment(ctx, &in, 2));
    err_msg[0] = "Unexpected end-of-input, non-terminated comment.";
    err_path[0] = "Line number 5.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    assert_true(in.current[0] == '\0');

    PARSER_DESTROY(ctx);
}


static void
test_arg(void **state)
{
    (void) state;
    struct lys_yang_parser_ctx *ctx;
    struct ly_in in = {0};
    char *word, *buf;
    size_t len;
    char *err_msg[1];
    char *err_path[1];

    PARSER_CREATE(ctx);

    /* missing argument */
    in.current = ";";
    assert_int_equal(LY_SUCCESS, get_argument(ctx, &in, Y_MAYBE_STR_ARG, NULL, &word, &buf, &len));
    assert_null(word);

    in.current = "{";
    assert_int_equal(LY_EVALID, get_argument(ctx, &in, Y_STR_ARG, NULL, &word, &buf, &len));
    err_msg[0] = "Invalid character sequence \"{\", expected an argument.";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    /* invalid escape sequence */
    in.current = "\"\\s\"";
    assert_int_equal(LY_EVALID, get_argument(ctx, &in, Y_STR_ARG, NULL, &word, &buf, &len));
    err_msg[0] = "Double-quoted string unknown special character \'\\s\'.";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    TEST_GET_ARGUMENT_SUCCESS("\'\\s\'", ctx, Y_STR_ARG, "\\s\'", 2, "");

    /* invalid character after the argument */
    in.current = "hello\"";
    assert_int_equal(LY_EVALID, get_argument(ctx, &in, Y_STR_ARG, NULL, &word, &buf, &len));
    err_msg[0] = "Invalid character sequence \"\"\", expected unquoted string character, optsep, semicolon or opening brace.";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    in.current = "hello}";
    assert_int_equal(LY_EVALID, get_argument(ctx, &in, Y_STR_ARG, NULL, &word, &buf, &len));
    err_msg[0] = "Invalid character sequence \"}\", expected unquoted string character, optsep, semicolon or opening brace.";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    /* invalid identifier-ref-arg-str */
    in.current = "pre:pre:value";
    assert_int_equal(LY_EVALID, get_argument(ctx, &in, Y_PREF_IDENTIF_ARG, NULL, &word, &buf, &len));
    err_msg[0] = "Invalid identifier character ':' (0x003a).";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    in.current = "\"\";"; /* empty identifier is not allowed */
    assert_int_equal(LY_EVALID, get_argument(ctx, &in, Y_IDENTIF_ARG, NULL, &word, &buf, &len));
    err_msg[0] = "Statement argument is required.";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    in.current = "\"\";"; /* empty reference identifier is not allowed */
    assert_int_equal(LY_EVALID, get_argument(ctx, &in, Y_PREF_IDENTIF_ARG, NULL, &word, &buf, &len));
    err_msg[0] = "Statement argument is required.";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    /* slash is not an invalid character */
    TEST_GET_ARGUMENT_SUCCESS("hello/x\t", ctx, Y_STR_ARG, "hello/x\t", 7, "\t");
    assert_null(buf);

    /* different quoting */
    TEST_GET_ARGUMENT_SUCCESS("hello/x\t", ctx, Y_STR_ARG, "hello/x\t", 7, "\t");

    TEST_GET_ARGUMENT_SUCCESS("hello ", ctx, Y_STR_ARG, "hello ", 5, " ");

    TEST_GET_ARGUMENT_SUCCESS("hello/*comment*/\n", ctx, Y_STR_ARG, "hello/*comment*/\n", 5, "\n");

    TEST_GET_ARGUMENT_SUCCESS("\"hello\\n\\t\\\"\\\\\";", ctx, Y_STR_ARG, "hello\n\t\"\\", 9, ";");
    free(buf);

    ctx->indent = 14;
    /* - space and tabs before newline are stripped out
     * - space and tabs after newline (indentation) are stripped out
     */
    TEST_GET_ARGUMENT_SUCCESS("\"hello \t\n\t\t world!\"", ctx, Y_STR_ARG, "hello\n  world!", 14, "");
    free(buf);

//    /* In contrast to previous, the backslash-escaped tabs are expanded after trimming, so they are preserved */
    ctx->indent = 14;
    TEST_GET_ARGUMENT_SUCCESS("\"hello \\t\n\t\\t world!\"", ctx, Y_STR_ARG, "hello \t\n\t world!", 16, "");
    assert_ptr_equal(word, buf);
    free(buf);

//    /* Do not handle whitespaces after backslash-escaped newline as indentation */
    ctx->indent = 14;
    TEST_GET_ARGUMENT_SUCCESS("\"hello\\n\t\t world!\"", ctx, Y_STR_ARG, "hello\n\t\t world!", 15, "");
    assert_ptr_equal(word, buf);
    free(buf);

    ctx->indent = 14;
    TEST_GET_ARGUMENT_SUCCESS("\"hello\n \tworld!\"", ctx, Y_STR_ARG, "hello\nworld!", 12, "");
    assert_ptr_equal(word, buf);
    free(buf);

    TEST_GET_ARGUMENT_SUCCESS("\'hello\'", ctx, Y_STR_ARG, "hello'", 5, "");

    TEST_GET_ARGUMENT_SUCCESS("\"hel\"  +\t\n\"lo\"", ctx, Y_STR_ARG, "hello", 5, "");
    assert_ptr_equal(word, buf);
    free(buf);

    in.current = "\"hel\"  +\t\nlo"; /* unquoted the second part */
    assert_int_equal(LY_EVALID, get_argument(ctx, &in, Y_STR_ARG, NULL, &word, &buf, &len));
    err_msg[0] = "Both string parts divided by '+' must be quoted.";
    err_path[0] = "Line number 6.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    TEST_GET_ARGUMENT_SUCCESS("\'he\'\t\n+ \"llo\"", ctx, Y_STR_ARG, "hello", 5, "");
    free(buf);

    TEST_GET_ARGUMENT_SUCCESS(" \t\n\"he\"+\'llo\'", ctx, Y_STR_ARG, "hello", 5, "");
    free(buf);


    /* missing argument */
    in.current = ";";
    assert_int_equal(LY_EVALID, get_argument(ctx, &in, Y_STR_ARG, NULL, &word, &buf, &len));
    err_msg[0] = "Invalid character sequence \";\", expected an argument.";
    err_path[0] = "Line number 8.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    PARSER_DESTROY(ctx);
}


#define TEST_STMS_SUCCESS(INPUT_TEXT, CTX, ACTION, EXPECT_WORD)\
                   in.current = INPUT_TEXT;\
                   assert_int_equal(LY_SUCCESS, get_keyword(CTX, &in, &kw, &word, &len));\
                   assert_int_equal(ACTION, kw);\
                   assert_int_equal(strlen(EXPECT_WORD), len);\
                   assert_true(0 == strncmp(EXPECT_WORD, word, len))



static void
test_stmts(void **state)
{
    (void) state;
    struct lys_yang_parser_ctx *ctx;
    struct ly_in in = {0};
    const char *p;
    enum ly_stmt kw;
    char *word;
    size_t len;
    char *err_msg[1];
    char *err_path[1];


    PARSER_CREATE(ctx);

    in.current = "\n// comment\n\tinput\t{";
    assert_int_equal(LY_SUCCESS, get_keyword(ctx, &in, &kw, &word, &len));
    assert_int_equal(LY_STMT_INPUT, kw);
    assert_int_equal(5, len);
    assert_string_equal("input\t{", word);
    assert_string_equal("\t{", in.current);

    in.current = "\t /* comment */\t output\n\t{";
    assert_int_equal(LY_SUCCESS, get_keyword(ctx, &in, &kw, &word, &len));
    assert_int_equal(LY_STMT_OUTPUT, kw);
    assert_int_equal(6, len);
    assert_string_equal("output\n\t{", word);
    assert_string_equal("\n\t{", in.current);
    assert_int_equal(LY_SUCCESS, get_keyword(ctx, &in, &kw, &word, &len));
    assert_int_equal(LY_STMT_SYNTAX_LEFT_BRACE, kw);
    assert_int_equal(1, len);
    assert_string_equal("{", word);
    assert_string_equal("", in.current);

    in.current = "/input { "; /* invalid slash */
    assert_int_equal(LY_EVALID, get_keyword(ctx, &in, &kw, &word, &len));
    err_msg[0] = "Invalid identifier first character '/'.";
    err_path[0] = "Line number 4.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    in.current = "not-a-statement-nor-extension { "; /* invalid identifier */
    assert_int_equal(LY_EVALID, get_keyword(ctx, &in, &kw, &word, &len));
    err_msg[0] = "Invalid character sequence \"not-a-statement-nor-extension\", expected a keyword.";
    err_path[0] = "Line number 4.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    in.current = "path;"; /* missing sep after the keyword */
    assert_int_equal(LY_EVALID, get_keyword(ctx, &in, &kw, &word, &len));
    err_msg[0] = "Invalid character sequence \"path;\", expected a keyword followed by a separator.";
    err_path[0] = "Line number 4.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    TEST_STMS_SUCCESS("action ", ctx, LY_STMT_ACTION, "action");

    TEST_STMS_SUCCESS("anydata ", ctx, LY_STMT_ANYDATA, "anydata");
    TEST_STMS_SUCCESS("anyxml ", ctx, LY_STMT_ANYXML, "anyxml");
    TEST_STMS_SUCCESS("argument ", ctx, LY_STMT_ARGUMENT, "argument");
    TEST_STMS_SUCCESS("augment ", ctx, LY_STMT_AUGMENT, "augment");
    TEST_STMS_SUCCESS("base ", ctx, LY_STMT_BASE, "base");
    TEST_STMS_SUCCESS("belongs-to ", ctx, LY_STMT_BELONGS_TO, "belongs-to");
    TEST_STMS_SUCCESS("bit ", ctx, LY_STMT_BIT, "bit");
    TEST_STMS_SUCCESS("case ", ctx, LY_STMT_CASE, "case");
    TEST_STMS_SUCCESS("choice ", ctx, LY_STMT_CHOICE, "choice");
    TEST_STMS_SUCCESS("config ", ctx, LY_STMT_CONFIG, "config");
    TEST_STMS_SUCCESS("contact ", ctx, LY_STMT_CONTACT, "contact");
    TEST_STMS_SUCCESS("container ", ctx, LY_STMT_CONTAINER, "container");
    TEST_STMS_SUCCESS("default ", ctx, LY_STMT_DEFAULT, "default");
    TEST_STMS_SUCCESS("description ", ctx, LY_STMT_DESCRIPTION, "description");
    TEST_STMS_SUCCESS("deviate ", ctx, LY_STMT_DEVIATE, "deviate");
    TEST_STMS_SUCCESS("deviation ", ctx, LY_STMT_DEVIATION, "deviation");
    TEST_STMS_SUCCESS("enum ", ctx, LY_STMT_ENUM, "enum");
    TEST_STMS_SUCCESS("error-app-tag ", ctx, LY_STMT_ERROR_APP_TAG, "error-app-tag");
    TEST_STMS_SUCCESS("error-message ", ctx, LY_STMT_ERROR_MESSAGE, "error-message");
    TEST_STMS_SUCCESS("extension ", ctx, LY_STMT_EXTENSION, "extension");
    TEST_STMS_SUCCESS("feature ", ctx, LY_STMT_FEATURE, "feature");
    TEST_STMS_SUCCESS("fraction-digits ", ctx, LY_STMT_FRACTION_DIGITS, "fraction-digits");
    TEST_STMS_SUCCESS("grouping ", ctx, LY_STMT_GROUPING, "grouping");
    TEST_STMS_SUCCESS("identity ", ctx, LY_STMT_IDENTITY, "identity");
    TEST_STMS_SUCCESS("if-feature ", ctx, LY_STMT_IF_FEATURE, "if-feature");
    TEST_STMS_SUCCESS("import ", ctx, LY_STMT_IMPORT, "import");
    TEST_STMS_SUCCESS("include ", ctx, LY_STMT_INCLUDE, "include");
    TEST_STMS_SUCCESS("input{", ctx, LY_STMT_INPUT, "input");
    TEST_STMS_SUCCESS("key ", ctx, LY_STMT_KEY, "key");
    TEST_STMS_SUCCESS("leaf ", ctx, LY_STMT_LEAF, "leaf");
    TEST_STMS_SUCCESS("leaf-list ", ctx, LY_STMT_LEAF_LIST, "leaf-list");
    TEST_STMS_SUCCESS("length ", ctx, LY_STMT_LENGTH, "length");
    TEST_STMS_SUCCESS("list ", ctx, LY_STMT_LIST, "list");
    TEST_STMS_SUCCESS("mandatory ", ctx, LY_STMT_MANDATORY, "mandatory");
    TEST_STMS_SUCCESS("max-elements ", ctx, LY_STMT_MAX_ELEMENTS, "max-elements");
    TEST_STMS_SUCCESS("min-elements ", ctx, LY_STMT_MIN_ELEMENTS, "min-elements");
    TEST_STMS_SUCCESS("modifier ", ctx, LY_STMT_MODIFIER, "modifier");
    TEST_STMS_SUCCESS("module ", ctx, LY_STMT_MODULE, "module");
    TEST_STMS_SUCCESS("must ", ctx, LY_STMT_MUST, "must");
    TEST_STMS_SUCCESS("namespace ", ctx, LY_STMT_NAMESPACE, "namespace");
    TEST_STMS_SUCCESS("notification ", ctx, LY_STMT_NOTIFICATION, "notification");
    TEST_STMS_SUCCESS("ordered-by ", ctx, LY_STMT_ORDERED_BY, "ordered-by");
    TEST_STMS_SUCCESS("organization ", ctx, LY_STMT_ORGANIZATION, "organization");
    TEST_STMS_SUCCESS("output ", ctx, LY_STMT_OUTPUT, "output");
    TEST_STMS_SUCCESS("path ", ctx, LY_STMT_PATH, "path");
    TEST_STMS_SUCCESS("pattern ", ctx, LY_STMT_PATTERN, "pattern");
    TEST_STMS_SUCCESS("position ", ctx, LY_STMT_POSITION, "position");
    TEST_STMS_SUCCESS("prefix ", ctx, LY_STMT_PREFIX, "prefix");
    TEST_STMS_SUCCESS("presence ", ctx, LY_STMT_PRESENCE, "presence");
    TEST_STMS_SUCCESS("range ", ctx, LY_STMT_RANGE, "range");
    TEST_STMS_SUCCESS("reference ", ctx, LY_STMT_REFERENCE, "reference");
    TEST_STMS_SUCCESS("refine ", ctx, LY_STMT_REFINE, "refine");
    TEST_STMS_SUCCESS("require-instance ", ctx, LY_STMT_REQUIRE_INSTANCE, "require-instance");
    TEST_STMS_SUCCESS("revision ", ctx, LY_STMT_REVISION, "revision");
    TEST_STMS_SUCCESS("revision-date ", ctx, LY_STMT_REVISION_DATE, "revision-date");
    TEST_STMS_SUCCESS("rpc ", ctx, LY_STMT_RPC, "rpc");
    TEST_STMS_SUCCESS("status ", ctx, LY_STMT_STATUS, "status");
    TEST_STMS_SUCCESS("submodule ", ctx, LY_STMT_SUBMODULE, "submodule");
    TEST_STMS_SUCCESS("type ", ctx, LY_STMT_TYPE, "type");
    TEST_STMS_SUCCESS("typedef ", ctx, LY_STMT_TYPEDEF, "typedef");
    TEST_STMS_SUCCESS("unique ", ctx, LY_STMT_UNIQUE, "unique");
    TEST_STMS_SUCCESS("units ", ctx, LY_STMT_UNITS, "units");
    TEST_STMS_SUCCESS("uses ", ctx, LY_STMT_USES, "uses");
    TEST_STMS_SUCCESS("value ", ctx, LY_STMT_VALUE, "value");
    TEST_STMS_SUCCESS("when ", ctx, LY_STMT_WHEN, "when");
    TEST_STMS_SUCCESS("yang-version ", ctx, LY_STMT_YANG_VERSION, "yang-version");
    TEST_STMS_SUCCESS("yin-element ", ctx, LY_STMT_YIN_ELEMENT, "yin-element");
    TEST_STMS_SUCCESS(";config false;", ctx, LY_STMT_SYNTAX_SEMICOLON, ";");
    assert_string_equal("config false;", in.current);
    TEST_STMS_SUCCESS("{ config false;", ctx, LY_STMT_SYNTAX_LEFT_BRACE, "{");
    assert_string_equal(" config false;", in.current);
    TEST_STMS_SUCCESS("}", ctx, LY_STMT_SYNTAX_RIGHT_BRACE, "}");
    assert_string_equal("", in.current);

    /* geenric extension */
    in.current = p = "nacm:default-deny-write;";
    assert_int_equal(LY_SUCCESS, get_keyword(ctx, &in, &kw, &word, &len));
    assert_int_equal(LY_STMT_EXTENSION_INSTANCE, kw);
    assert_int_equal(23, len);
    assert_ptr_equal(p, word);

    PARSER_DESTROY(ctx);
}


#define TEST_MINMAX_SUCCESS(INPUT_TEXT, CTX, TYPE, VALUE)\
                in.current = INPUT_TEXT;\
                if(TYPE == LYS_SET_MIN){\
                   assert_int_equal(LY_SUCCESS, parse_minelements(CTX, &in, &value, &flags, &ext));\
                }\
                if(TYPE == LYS_SET_MAX){\
                   assert_int_equal(LY_SUCCESS, parse_maxelements(CTX, &in, &value, &flags, &ext));\
                }\
                assert_int_equal(TYPE, flags);\
                assert_int_equal(VALUE, value)

static void
test_minmax(void **state)
{
    (void) state;
    struct lys_yang_parser_ctx *ctx;
    uint16_t flags = 0;
    uint32_t value = 0;
    struct lysp_ext_instance *ext = NULL;
    struct ly_in in = {0};
    char *err_msg[1];
    char *err_path[1];

    PARSER_CREATE(ctx);

    ctx->parsed_mod->version = 2; /* simulate YANG 1.1 */

    in.current = " 1invalid; ...";
    assert_int_equal(LY_EVALID, parse_minelements(ctx, &in, &value, &flags, &ext));
    err_msg[0] = "Invalid value \"1invalid\" of \"min-elements\".";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    flags = value = 0;
    in.current = " -1; ...";
    assert_int_equal(LY_EVALID, parse_minelements(ctx, &in, &value, &flags, &ext));
    err_msg[0] = "Invalid value \"-1\" of \"min-elements\".";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    /* implementation limit */
    flags = value = 0;
    in.current = " 4294967296; ...";
    assert_int_equal(LY_EVALID, parse_minelements(ctx, &in, &value, &flags, &ext));
    err_msg[0] = "Value \"4294967296\" is out of \"min-elements\" bounds.";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    flags = value = 0;
    TEST_MINMAX_SUCCESS(" 1; ...",ctx, LYS_SET_MIN, 1);

    flags = value = 0;
    TEST_MINMAX_SUCCESS(" 1 {m:ext;} ...",ctx, LYS_SET_MIN, 1);
    assert_non_null(ext);
    FREE_ARRAY(ctx->parsed_mod->mod->ctx, ext, lysp_ext_instance_free);
    ext = NULL;

    flags = value = 0;
    in.current = " 1 {config true;} ...";
    assert_int_equal(LY_EVALID, parse_minelements(ctx, &in, &value, &flags, &ext));
    err_msg[0] = "Invalid keyword \"config\" as a child of \"min-elements\".";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    in.current = " 1invalid; ...";
    assert_int_equal(LY_EVALID, parse_maxelements(ctx, &in, &value, &flags, &ext));
    err_msg[0] = "Invalid value \"1invalid\" of \"max-elements\".";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    flags = value = 0;
    in.current = " -1; ...";
    assert_int_equal(LY_EVALID, parse_maxelements(ctx, &in, &value, &flags, &ext));
    err_msg[0] = "Invalid value \"-1\" of \"max-elements\".";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    /* implementation limit */
    flags = value = 0;
    in.current = " 4294967296; ...";
    assert_int_equal(LY_EVALID, parse_maxelements(ctx, &in, &value, &flags, &ext));
    err_msg[0] = "Value \"4294967296\" is out of \"max-elements\" bounds.";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    flags = value = 0;
    TEST_MINMAX_SUCCESS(" 1; ...",ctx, LYS_SET_MAX, 1);

    flags = value = 0;
    TEST_MINMAX_SUCCESS(" unbounded; ...",ctx, LYS_SET_MAX, 0);

    flags = value = 0;
    TEST_MINMAX_SUCCESS(" 1 {m:ext;} ...",ctx, LYS_SET_MAX, 1);
    assert_non_null(ext);
    FREE_ARRAY(ctx->parsed_mod->mod->ctx, ext, lysp_ext_instance_free);
    ext = NULL;

    flags = value = 0;
    in.current = " 1 {config true;} ...";
    assert_int_equal(LY_EVALID, parse_maxelements(ctx, &in, &value, &flags, &ext));
    err_msg[0] = "Invalid keyword \"config\" as a child of \"max-elements\".";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    PARSER_DESTROY(ctx);
}

static struct lysp_module *
mod_renew(struct lys_yang_parser_ctx *ctx)
{
    struct ly_ctx *ly_ctx = ctx->parsed_mod->mod->ctx;

    lys_module_free(ctx->parsed_mod->mod, NULL);
    ctx->parsed_mod = calloc(1, sizeof *ctx->parsed_mod);
    ctx->parsed_mod->mod = calloc(1, sizeof *ctx->parsed_mod->mod);
    ctx->parsed_mod->mod->parsed = ctx->parsed_mod;
    ctx->parsed_mod->mod->ctx = ly_ctx;

    return ctx->parsed_mod;
}

static struct lysp_submodule *
submod_renew(struct lys_yang_parser_ctx *ctx)
{
    struct ly_ctx *ly_ctx = ctx->parsed_mod->mod->ctx;

    lys_module_free(ctx->parsed_mod->mod, NULL);
    ctx->parsed_mod = calloc(1, sizeof(struct lysp_submodule));
    ctx->parsed_mod->mod = calloc(1, sizeof *ctx->parsed_mod->mod);
    lydict_insert(ly_ctx, "name", 0, &ctx->parsed_mod->mod->name);
    ctx->parsed_mod->mod->parsed = ctx->parsed_mod;
    ctx->parsed_mod->mod->ctx = ly_ctx;

    return (struct lysp_submodule *)ctx->parsed_mod;
}

static LY_ERR test_imp_clb(const char *UNUSED(mod_name), const char *UNUSED(mod_rev), const char *UNUSED(submod_name),
                           const char *UNUSED(sub_rev), void *user_data, LYS_INFORMAT *format,
                           const char **module_data, void (**free_module_data)(void *model_data, void *user_data))
{
    *module_data = user_data;
    *format = LYS_IN_YANG;
    *free_module_data = NULL;
    return LY_SUCCESS;
}

static void
test_module(void **state)
{
    (void) state;
    struct lys_yang_parser_ctx *ctx;
    struct lysp_module *mod = NULL;
    struct lysp_submodule *submod = NULL;
    struct lys_module *m;
    struct ly_in in = {0};
    char *err_msg[1];
    char *err_path[1];

    PARSER_CREATE(ctx);

    mod = mod_renew(ctx);

    /* missing mandatory substatements */
    in.current = " name {}";
    assert_int_equal(LY_EVALID, parse_module(ctx, &in, mod));
    assert_string_equal("name", mod->mod->name);
    err_msg[0] = "Missing mandatory keyword \"namespace\" as a child of \"module\".";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    mod = mod_renew(ctx);
    in.current = " name {namespace urn:x;}";
    assert_int_equal(LY_EVALID, parse_module(ctx, &in, mod));
    assert_string_equal("urn:x", mod->mod->ns);
    err_msg[0] = "Missing mandatory keyword \"prefix\" as a child of \"module\".";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    mod = mod_renew(ctx);

    in.current = " name {namespace urn:x;prefix \"x\";}";
    assert_int_equal(LY_SUCCESS, parse_module(ctx, &in, mod));
    assert_string_equal("x", mod->mod->prefix);
    mod = mod_renew(ctx);

#define SCHEMA_BEGINNING " name {yang-version 1.1;namespace urn:x;prefix \"x\";"
#define SCHEMA_BEGINNING2 " name {namespace urn:x;prefix \"x\";"
#define TEST_NODE(NODETYPE, INPUT, NAME) \
        in.current = SCHEMA_BEGINNING INPUT; \
        assert_int_equal(LY_SUCCESS, parse_module(ctx, &in, mod)); \
        assert_non_null(mod->data); \
        assert_int_equal(NODETYPE, mod->data->nodetype); \
        assert_string_equal(NAME, mod->data->name); \
        mod = mod_renew(ctx);
#define TEST_GENERIC(INPUT, TARGET, TEST) \
        in.current = SCHEMA_BEGINNING INPUT; \
        assert_int_equal(LY_SUCCESS, parse_module(ctx, &in, mod)); \
        assert_non_null(TARGET); \
        TEST; \
        mod = mod_renew(ctx);
#define TEST_DUP(MEMBER, VALUE1, VALUE2, LINE) \
        TEST_DUP_GENERIC(SCHEMA_BEGINNING, MEMBER, VALUE1, VALUE2, \
                         parse_module, mod, LINE, mod = mod_renew(ctx))

    /* duplicated namespace, prefix */
    TEST_DUP("namespace", "y", "z", "1");
    TEST_DUP("prefix", "y", "z", "1");
    TEST_DUP("contact", "a", "b", "1");
    TEST_DUP("description", "a", "b", "1");
    TEST_DUP("organization", "a", "b", "1");
    TEST_DUP("reference", "a", "b", "1");

    /* not allowed in module (submodule-specific) */
    in.current = SCHEMA_BEGINNING "belongs-to master {prefix m;}}";
    assert_int_equal(LY_EVALID, parse_module(ctx, &in, mod));
    err_msg[0] = "Invalid keyword \"belongs-to\" as a child of \"module\".";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    mod = mod_renew(ctx);

    /* anydata */
    TEST_NODE(LYS_ANYDATA, "anydata test;}", "test");
    /* anyxml */
    TEST_NODE(LYS_ANYXML, "anyxml test;}", "test");
    /* augment */
    TEST_GENERIC("augment /somepath;}", mod->augments,
                 assert_string_equal("/somepath", mod->augments[0].nodeid));
    /* choice */
    TEST_NODE(LYS_CHOICE, "choice test;}", "test");
    /* contact 0..1 */
    TEST_GENERIC("contact \"firstname\" + \n\t\" surname\";}", mod->mod->contact,
                 assert_string_equal("firstname surname", mod->mod->contact));
    /* container */
    TEST_NODE(LYS_CONTAINER, "container test;}", "test");
    /* description 0..1 */
    TEST_GENERIC("description \'some description\';}", mod->mod->dsc,
                 assert_string_equal("some description", mod->mod->dsc));
    /* deviation */
    TEST_GENERIC("deviation /somepath {deviate not-supported;}}", mod->deviations,
                 assert_string_equal("/somepath", mod->deviations[0].nodeid));
    /* extension */
    TEST_GENERIC("extension test;}", mod->extensions,
                 assert_string_equal("test", mod->extensions[0].name));
    /* feature */
    TEST_GENERIC("feature test;}", mod->features,
                 assert_string_equal("test", mod->features[0].name));
    /* grouping */
    TEST_GENERIC("grouping grp;}", mod->groupings,
                 assert_string_equal("grp", mod->groupings[0].name));
    /* identity */
    TEST_GENERIC("identity test;}", mod->identities,
                 assert_string_equal("test", mod->identities[0].name));
    /* import */
    ly_ctx_set_module_imp_clb(ctx->parsed_mod->mod->ctx, test_imp_clb, "module zzz { namespace urn:zzz; prefix z;}");
    TEST_GENERIC("import zzz {prefix z;}}", mod->imports,
                 assert_string_equal("zzz", mod->imports[0].name));

    /* import - prefix collision */
    in.current = SCHEMA_BEGINNING "import zzz {prefix x;}}";
    assert_int_equal(LY_EVALID, parse_module(ctx, &in, mod));
    err_msg[0] = "Prefix \"x\" already used as module prefix.";
    err_path[0] = "Line number 2.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    mod = mod_renew(ctx);

    in.current = SCHEMA_BEGINNING "import zzz {prefix y;}import zzz {prefix y;}}";
    assert_int_equal(LY_EVALID, parse_module(ctx, &in, mod));
    err_msg[0] = "Prefix \"y\" already used to import \"zzz\" module.";
    err_path[0] = "Line number 2.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    mod = mod_renew(ctx);
    in.current = "module name10 {yang-version 1.1;namespace urn:x;prefix \"x\";import zzz {prefix y;}import zzz {prefix z;}}";
    assert_int_equal(lys_parse_mem(ctx->parsed_mod->mod->ctx, in.current, LYS_IN_YANG, NULL), LY_SUCCESS);
    err_msg[0] = "Single revision of the module \"zzz\" imported twice.";
    err_path[0] = NULL;
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    /* include */
    ly_ctx_set_module_imp_clb(ctx->parsed_mod->mod->ctx, test_imp_clb, "module xxx { namespace urn:xxx; prefix x;}");
    in.current = "module" SCHEMA_BEGINNING "include xxx;}";
    assert_int_equal(lys_parse_mem(ctx->parsed_mod->mod->ctx, in.current, LYS_IN_YANG, NULL), LY_EVALID);
    err_msg[0]  = "Including \"xxx\" submodule into \"name\" failed.";
    err_path[0] = NULL;
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    ly_ctx_set_module_imp_clb(ctx->parsed_mod->mod->ctx, test_imp_clb, "submodule xxx {belongs-to wrong-name {prefix w;}}");
    in.current = "module" SCHEMA_BEGINNING "include xxx;}";
    assert_int_equal(lys_parse_mem(ctx->parsed_mod->mod->ctx, in.current, LYS_IN_YANG, NULL), LY_EVALID);
    err_msg[0]  = "Including \"xxx\" submodule into \"name\" failed.";
    err_path[0] = NULL;
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    ly_ctx_set_module_imp_clb(ctx->parsed_mod->mod->ctx, test_imp_clb, "submodule xxx {belongs-to name {prefix x;}}");
    TEST_GENERIC("include xxx;}", mod->includes,
                 assert_string_equal("xxx", mod->includes[0].name));

    /* leaf */
    TEST_NODE(LYS_LEAF, "leaf test {type string;}}", "test");
    /* leaf-list */
    TEST_NODE(LYS_LEAFLIST, "leaf-list test {type string;}}", "test");
    /* list */
    TEST_NODE(LYS_LIST, "list test {key a;leaf a {type string;}}}", "test");
    /* notification */
    TEST_GENERIC("notification test;}", mod->notifs,
                 assert_string_equal("test", mod->notifs[0].name));
    /* organization 0..1 */
    TEST_GENERIC("organization \"CESNET a.l.e.\";}", mod->mod->org,
                 assert_string_equal("CESNET a.l.e.", mod->mod->org));
    /* reference 0..1 */
    TEST_GENERIC("reference RFC7950;}", mod->mod->ref,
                 assert_string_equal("RFC7950", mod->mod->ref));
    /* revision */
    TEST_GENERIC("revision 2018-10-12;}", mod->revs,
                 assert_string_equal("2018-10-12", mod->revs[0].date));
    /* rpc */
    TEST_GENERIC("rpc test;}", mod->rpcs,
                 assert_string_equal("test", mod->rpcs[0].name));
    /* typedef */
    TEST_GENERIC("typedef test{type string;}}", mod->typedefs,
                 assert_string_equal("test", mod->typedefs[0].name));
    /* uses */
    TEST_NODE(LYS_USES, "uses test;}", "test");
    /* yang-version */
    in.current = SCHEMA_BEGINNING2 "\n\tyang-version 10;}";
    assert_int_equal(LY_EVALID, parse_module(ctx, &in, mod));
    err_msg[0] = "Invalid value \"10\" of \"yang-version\".";
    err_path[0] = "Line number 3.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    mod = mod_renew(ctx);
    in.current = SCHEMA_BEGINNING2 "yang-version 1;yang-version 1.1;}";
    assert_int_equal(LY_EVALID, parse_module(ctx, &in, mod));
    err_msg[0] = "Duplicate keyword \"yang-version\".";
    err_path[0] = "Line number 3.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    mod = mod_renew(ctx);
    in.current = SCHEMA_BEGINNING2 "yang-version 1;}";
    assert_int_equal(LY_SUCCESS, parse_module(ctx, &in, mod));
    assert_int_equal(1, mod->version);
    mod = mod_renew(ctx);
    in.current = SCHEMA_BEGINNING2 "yang-version \"1.1\";}";
    assert_int_equal(LY_SUCCESS, parse_module(ctx, &in, mod));
    assert_int_equal(2, mod->version);
    mod = mod_renew(ctx);

    struct lys_yang_parser_ctx *ctx_p = NULL;
    in.current = "module " SCHEMA_BEGINNING "} module q {namespace urn:q;prefixq;}";
    m = calloc(1, sizeof *m);
    m->ctx = ctx->parsed_mod->mod->ctx;
    assert_int_equal(LY_EVALID, yang_parse_module(&ctx_p, &in, m));
    err_msg[0] = "Trailing garbage \"module q {names...\" after module, expected end-of-input.";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    yang_parser_ctx_free(ctx_p);
    lys_module_free(m, NULL);

    in.current = "prefix " SCHEMA_BEGINNING "}";
    m = calloc(1, sizeof *m);
    m->ctx = ctx->parsed_mod->mod->ctx;
    assert_int_equal(LY_EVALID, yang_parse_module(&ctx_p, &in, m));
    err_msg[0] = "Invalid keyword \"prefix\", expected \"module\" or \"submodule\".";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    yang_parser_ctx_free(ctx_p);
    lys_module_free(m, NULL);

    in.current = "module " SCHEMA_BEGINNING "leaf enum {type enumeration {enum seven { position 7;}}}}";
    m = calloc(1, sizeof *m);
    m->ctx = ctx->parsed_mod->mod->ctx;
    assert_int_equal(LY_EVALID, yang_parse_module(&ctx_p, &in, m));
    err_msg[0] = "Invalid keyword \"position\" as a child of \"enum\".";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    yang_parser_ctx_free(ctx_p);
    lys_module_free(m, NULL);

    /* extensions */
    TEST_GENERIC("prefix:test;}", mod->exts,
                 assert_string_equal("prefix:test", mod->exts[0].name);
                 assert_int_equal(LYEXT_SUBSTMT_SELF, mod->exts[0].insubstmt));
    mod = mod_renew(ctx);

    /* invalid substatement */
    in.current = SCHEMA_BEGINNING "must false;}";
    assert_int_equal(LY_EVALID, parse_module(ctx, &in, mod));
    err_msg[0] = "Invalid keyword \"must\" as a child of \"module\".";
    err_path[0] = "Line number 3.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    /* submodule */
    submod = submod_renew(ctx);

    /* missing mandatory substatements */
    in.current = " subname {}";
    assert_int_equal(LY_EVALID, parse_submodule(ctx, &in, submod));
    err_msg[0] = "Missing mandatory keyword \"belongs-to\" as a child of \"submodule\".";
    err_path[0] = "Line number 3.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    assert_string_equal("subname", submod->name);

    submod = submod_renew(ctx);

    in.current = " subname {belongs-to name {prefix x;}}";
    assert_int_equal(LY_SUCCESS, parse_submodule(ctx, &in, submod));
    assert_string_equal("name", submod->mod->name);
    submod = submod_renew(ctx);

#undef SCHEMA_BEGINNING
#define SCHEMA_BEGINNING " subname {belongs-to name {prefix x;}"

    /* duplicated namespace, prefix */
    in.current = " subname {belongs-to name {prefix x;}belongs-to module1;belongs-to module2;} ...";
    assert_int_equal(LY_EVALID, parse_submodule(ctx, &in, submod));
    err_msg[0] = "Duplicate keyword \"belongs-to\".";
    err_path[0] = "Line number 3.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    submod = submod_renew(ctx);

    /* not allowed in submodule (module-specific) */
    in.current = SCHEMA_BEGINNING "namespace \"urn:z\";}";
    assert_int_equal(LY_EVALID, parse_submodule(ctx, &in, submod));
    err_msg[0] = "Invalid keyword \"namespace\" as a child of \"submodule\".";
    err_path[0] = "Line number 3.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    submod = submod_renew(ctx);
    in.current = SCHEMA_BEGINNING "prefix m;}}";
    assert_int_equal(LY_EVALID, parse_submodule(ctx, &in, submod));
    err_msg[0] = "Invalid keyword \"prefix\" as a child of \"submodule\".";
    err_path[0] = "Line number 3.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    submod = submod_renew(ctx);

    in.current = "submodule " SCHEMA_BEGINNING "} module q {namespace urn:q;prefixq;}";
    assert_int_equal(LY_EVALID, yang_parse_submodule(&ctx_p, ctx->parsed_mod->mod->ctx, (struct lys_parser_ctx *)ctx, &in, &submod));
    err_msg[0] = "Trailing garbage \"module q {names...\" after submodule, expected end-of-input.";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    yang_parser_ctx_free(ctx_p);

    in.current = "prefix " SCHEMA_BEGINNING "}";
    assert_int_equal(LY_EVALID, yang_parse_submodule(&ctx_p, ctx->parsed_mod->mod->ctx, (struct lys_parser_ctx *)ctx, &in, &submod));
    err_msg[0] = "Invalid keyword \"prefix\", expected \"module\" or \"submodule\".";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    yang_parser_ctx_free(ctx_p);
    submod = submod_renew(ctx);


    PARSER_DESTROY(ctx);

#undef TEST_GENERIC
#undef TEST_NODE
#undef TEST_DUP
#undef SCHEMA_BEGINNING
}

static void
test_deviation(void **state)
{
    (void) state;
    struct lys_yang_parser_ctx *ctx;
    struct lysp_deviation *d = NULL;
    struct ly_in in = {0};
    char *err_msg[1];
    char *err_path[1];

    PARSER_CREATE(ctx);

    /* invalid cardinality */
#define TEST_DUP(MEMBER, VALUE1, VALUE2) \
    TEST_DUP_GENERIC(" test {deviate not-supported;", MEMBER, VALUE1, VALUE2, parse_deviation, \
                     &d, "1", FREE_ARRAY(ctx->parsed_mod->mod->ctx, d, lysp_deviation_free); d = NULL)

    TEST_DUP("description", "a", "b");
    TEST_DUP("reference", "a", "b");

    /* full content */
    in.current = " test {deviate not-supported;description text;reference \'another text\';prefix:ext;} ...";
    assert_int_equal(LY_SUCCESS, parse_deviation(ctx, &in, &d));
    assert_non_null(d);
    assert_string_equal(" ...", in.current);
    FREE_ARRAY(ctx->parsed_mod->mod->ctx, d, lysp_deviation_free);
    d = NULL;

    /* missing mandatory substatement */
    in.current = " test {description text;}";
    assert_int_equal(LY_EVALID, parse_deviation(ctx, &in, &d));
    err_msg[0] = "Missing mandatory keyword \"deviate\" as a child of \"deviation\".";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    FREE_ARRAY(ctx->parsed_mod->mod->ctx, d, lysp_deviation_free);
    d = NULL;

    /* invalid substatement */
    in.current = " test {deviate not-supported; status obsolete;}";
    assert_int_equal(LY_EVALID, parse_deviation(ctx, &in, &d));
    err_msg[0] = "Invalid keyword \"status\" as a child of \"deviation\".";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    FREE_ARRAY(ctx->parsed_mod->mod->ctx, d, lysp_deviation_free);
    d = NULL;

    PARSER_DESTROY(ctx);
#undef TEST_DUP
}


#define TEST_DEVIATE_SUCCESS(INPUT_TEXT, REMAIN_TEXT)\
                    in.current = INPUT_TEXT;\
                    assert_int_equal(LY_SUCCESS, parse_deviate(ctx, &in, &d));\
                    assert_non_null(d);\
                    assert_string_equal(REMAIN_TEXT, in.current);\
                    lysp_deviate_free(ctx->parsed_mod->mod->ctx, d); free(d); d = NULL

static void
test_deviate(void **state)
{
    (void) state;
    struct lys_yang_parser_ctx *ctx;
    struct lysp_deviate *d = NULL;
    struct ly_in in = {0};
    char *err_msg[1];
    char *err_path[1];

    PARSER_CREATE(ctx);

    /* invalid cardinality */
#define TEST_DUP(TYPE, MEMBER, VALUE1, VALUE2) \
    TEST_DUP_GENERIC(TYPE" {", MEMBER, VALUE1, VALUE2, parse_deviate, \
                     &d, "1", lysp_deviate_free(ctx->parsed_mod->mod->ctx, d); free(d); d = NULL)

    TEST_DUP("add", "config", "true", "false");
    TEST_DUP("replace", "default", "int8", "uint8");
    TEST_DUP("add", "mandatory", "true", "false");
    TEST_DUP("add", "max-elements", "1", "2");
    TEST_DUP("add", "min-elements", "1", "2");
    TEST_DUP("replace", "type", "int8", "uint8");
    TEST_DUP("add", "units", "kilometers", "miles");

    /* full contents */
    TEST_DEVIATE_SUCCESS(" not-supported {prefix:ext;} ...", " ...");
    TEST_DEVIATE_SUCCESS(" add {units meters; must 1; must 2; unique x; unique y; default a; default b; config true; mandatory true; min-elements 1; max-elements 2; prefix:ext;} ...", " ...");
    TEST_DEVIATE_SUCCESS(" delete {units meters; must 1; must 2; unique x; unique y; default a; default b; prefix:ext;} ...", " ...");
    TEST_DEVIATE_SUCCESS(" replace {type string; units meters; default a; config true; mandatory true; min-elements 1; max-elements 2; prefix:ext;} ...", " ...");

    /* invalid substatements */
#define TEST_NOT_SUP(DEV, STMT, VALUE) \
    in.current = " "DEV" {"STMT" "VALUE";}..."; \
    assert_int_equal(LY_EVALID, parse_deviate(ctx, &in, &d)); \
    err_msg[0]  = "Deviate \""DEV"\" does not support keyword \""STMT"\".";\
    err_path[0] = "Line number 1.";\
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);\
    lysp_deviate_free(ctx->parsed_mod->mod->ctx, d); free(d); d = NULL

    TEST_NOT_SUP("not-supported", "units", "meters");
    TEST_NOT_SUP("not-supported", "must", "1");
    TEST_NOT_SUP("not-supported", "unique", "x");
    TEST_NOT_SUP("not-supported", "default", "a");
    TEST_NOT_SUP("not-supported", "config", "true");
    TEST_NOT_SUP("not-supported", "mandatory", "true");
    TEST_NOT_SUP("not-supported", "min-elements", "1");
    TEST_NOT_SUP("not-supported", "max-elements", "2");
    TEST_NOT_SUP("not-supported", "type", "string");
    TEST_NOT_SUP("add", "type", "string");
    TEST_NOT_SUP("delete", "config", "true");
    TEST_NOT_SUP("delete", "mandatory", "true");
    TEST_NOT_SUP("delete", "min-elements", "1");
    TEST_NOT_SUP("delete", "max-elements", "2");
    TEST_NOT_SUP("delete", "type", "string");
    TEST_NOT_SUP("replace", "must", "1");
    TEST_NOT_SUP("replace", "unique", "a");

    in.current = " nonsence; ...";
    assert_int_equal(LY_EVALID, parse_deviate(ctx, &in, &d));
    err_msg[0]  = "Invalid value \"nonsence\" of \"deviate\".";\
    err_path[0] = "Line number 1.";\
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);\
    assert_null(d);

    PARSER_DESTROY(ctx);
#undef TEST_NOT_SUP
#undef TEST_DUP
}

static void
test_container(void **state)
{
    (void) state;
    struct lys_yang_parser_ctx *ctx;
    struct lysp_node_container *c = NULL;
    struct ly_in in = {0};
    char *err_msg[1];
    char *err_path[1];

    PARSER_CREATE(ctx);

    ctx->parsed_mod->version = 2; /* simulate YANG 1.1 */

    /* invalid cardinality */
#define TEST_DUP(MEMBER, VALUE1, VALUE2) \
    in.current = "cont {" MEMBER" "VALUE1";"MEMBER" "VALUE2";} ..."; \
    assert_int_equal(LY_EVALID, parse_container(ctx, &in, NULL, (struct lysp_node**)&c)); \
    err_msg[0] = "Duplicate keyword \""MEMBER"\".";\
    err_path[0] = "Line number 1.";\
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path); \
    lysp_node_free(ctx->parsed_mod->mod->ctx, (struct lysp_node*)c); c = NULL;

    TEST_DUP("config", "true", "false");
    TEST_DUP("description", "text1", "text2");
    TEST_DUP("presence", "true", "false");
    TEST_DUP("reference", "1", "2");
    TEST_DUP("status", "current", "obsolete");
    TEST_DUP("when", "true", "false");
#undef TEST_DUP

    /* full content */
    in.current = "cont {action x;anydata any;anyxml anyxml; choice ch;config false;container c;description test;grouping g;if-feature f; leaf l {type string;}"
          "leaf-list ll {type string;} list li;must 'expr';notification not; presence true; reference test;status current;typedef t {type int8;}uses g;when true;m:ext;} ...";
    assert_int_equal(LY_SUCCESS, parse_container(ctx, &in, NULL, (struct lysp_node**)&c));
    uint16_t flag = LYS_CONFIG_R | LYS_STATUS_CURR;
    LYSP_NODE_CHECK(c, "test", 1, flag, 1, "cont", 0, LYS_CONTAINER, 0, "test", 1);
    assert_non_null(c->actions);
    assert_non_null(c->child);
    assert_non_null(c->groupings);
    assert_non_null(c->musts);
    assert_non_null(c->notifs);
    assert_string_equal("true", c->presence);
    assert_non_null(c->typedefs);
    ly_set_erase(&ctx->tpdfs_nodes, NULL);
    lysp_node_free(ctx->parsed_mod->mod->ctx, (struct lysp_node*)c); c = NULL;

    /* invalid */
    in.current = " cont {augment /root;} ...";
    assert_int_equal(LY_EVALID, parse_container(ctx, &in, NULL, (struct lysp_node**)&c));
    err_msg[0] = "Invalid keyword \"augment\" as a child of \"container\".";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    lysp_node_free(ctx->parsed_mod->mod->ctx, (struct lysp_node*)c); c = NULL;
    in.current = " cont {nonsence true;} ...";
    assert_int_equal(LY_EVALID, parse_container(ctx, &in, NULL, (struct lysp_node**)&c));
    err_msg[0] = "Invalid character sequence \"nonsence\", expected a keyword.";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    lysp_node_free(ctx->parsed_mod->mod->ctx, (struct lysp_node*)c); c = NULL;

    ctx->parsed_mod->version = 1; /* simulate YANG 1.0 */
    in.current = " cont {action x;} ...";
    assert_int_equal(LY_EVALID, parse_container(ctx, &in, NULL, (struct lysp_node**)&c));
    err_msg[0] = "Invalid keyword \"action\" as a child of \"container\" - the statement is allowed only in YANG 1.1 modules.";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    lysp_node_free(ctx->parsed_mod->mod->ctx, (struct lysp_node*)c); c = NULL;

    PARSER_DESTROY(ctx);
}

static void
test_leaf(void **state)
{
    (void) state;
    struct lys_yang_parser_ctx *ctx;
    struct lysp_node_leaf *l = NULL;
    struct ly_in in = {0};
    char *err_msg[1];
    char *err_path[1];

    PARSER_CREATE(ctx);

    /* invalid cardinality */
#define TEST_DUP(MEMBER, VALUE1, VALUE2) \
    in.current = "l {" MEMBER" "VALUE1";"MEMBER" "VALUE2";} ..."; \
    assert_int_equal(LY_EVALID, parse_leaf(ctx, &in, NULL, (struct lysp_node**)&l)); \
    err_msg[0] = "Duplicate keyword \""MEMBER"\".";\
    err_path[0] = "Line number 1.";\
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path); \
    lysp_node_free(ctx->parsed_mod->mod->ctx, (struct lysp_node*)l); l = NULL;

    TEST_DUP("config", "true", "false");
    TEST_DUP("default", "x", "y");
    TEST_DUP("description", "text1", "text2");
    TEST_DUP("mandatory", "true", "false");
    TEST_DUP("reference", "1", "2");
    TEST_DUP("status", "current", "obsolete");
    TEST_DUP("type", "int8", "uint8");
    TEST_DUP("units", "text1", "text2");
    TEST_DUP("when", "true", "false");
#undef TEST_DUP

    /* full content - without mandatory which is mutual exclusive with default */
    in.current = "l {config false;default \"xxx\";description test;if-feature f;"
          "must 'expr';reference test;status current;type string; units yyy;when true;m:ext;} ...";
    assert_int_equal(LY_SUCCESS, parse_leaf(ctx, &in, NULL, (struct lysp_node**)&l));
    uint16_t flag = LYS_CONFIG_R | LYS_STATUS_CURR;
    LYSP_NODE_CHECK(l, "test", 1, flag, 1, "l", 0, LYS_LEAF, 0, "test", 1);
    assert_string_equal("xxx", l->dflt.str);
    assert_string_equal("yyy", l->units);
    assert_string_equal("string", l->type.name);
    assert_non_null(l->musts);
    lysp_node_free(ctx->parsed_mod->mod->ctx, (struct lysp_node*)l); l = NULL;

    /* full content - now with mandatory */
    in.current = "l {mandatory true; type string;} ...";
    assert_int_equal(LY_SUCCESS, parse_leaf(ctx, &in, NULL, (struct lysp_node**)&l));
    flag = LYS_MAND_TRUE;
    LYSP_NODE_CHECK(l, NULL, 0, flag, 0, "l", 0, LYS_LEAF, 0, NULL, 0);
    assert_string_equal("string", l->type.name);
    lysp_node_free(ctx->parsed_mod->mod->ctx, (struct lysp_node*)l); l = NULL;

    /* invalid */
    in.current = " l {description \"missing type\";} ...";
    assert_int_equal(LY_EVALID, parse_leaf(ctx, &in, NULL, (struct lysp_node**)&l));
    err_msg[0] = "Missing mandatory keyword \"type\" as a child of \"leaf\".";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    lysp_node_free(ctx->parsed_mod->mod->ctx, (struct lysp_node*)l); l = NULL;

    PARSER_DESTROY(ctx);
}

static void
test_leaflist(void **state)
{
    (void) state;
    struct lys_yang_parser_ctx *ctx;
    struct lysp_node_leaflist *ll = NULL;
    struct ly_in in = {0};
    char *err_msg[1];
    char *err_path[1];

    PARSER_CREATE(ctx);

    ctx->parsed_mod->version = 2; /* simulate YANG 1.1 */

    /* invalid cardinality */
#define TEST_DUP(MEMBER, VALUE1, VALUE2) \
    in.current = "ll {" MEMBER" "VALUE1";"MEMBER" "VALUE2";} ..."; \
    assert_int_equal(LY_EVALID, parse_leaflist(ctx, &in, NULL, (struct lysp_node**)&ll)); \
    err_msg[0] = "Duplicate keyword \""MEMBER"\".";\
    err_path[0] = "Line number 1.";\
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path); \
    lysp_node_free(ctx->parsed_mod->mod->ctx, (struct lysp_node*)ll); ll = NULL;

    TEST_DUP("config", "true", "false");
    TEST_DUP("description", "text1", "text2");
    TEST_DUP("max-elements", "10", "20");
    TEST_DUP("min-elements", "10", "20");
    TEST_DUP("ordered-by", "user", "system");
    TEST_DUP("reference", "1", "2");
    TEST_DUP("status", "current", "obsolete");
    TEST_DUP("type", "int8", "uint8");
    TEST_DUP("units", "text1", "text2");
    TEST_DUP("when", "true", "false");
#undef TEST_DUP

    /* full content - without min-elements which is mutual exclusive with default */
    in.current = "ll {config false;default \"xxx\"; default \"yyy\";description test;if-feature f;"
          "max-elements 10;must 'expr';ordered-by user;reference test;"
          "status current;type string; units zzz;when true;m:ext;} ...";
    assert_int_equal(LY_SUCCESS, parse_leaflist(ctx, &in, NULL, (struct lysp_node**)&ll));
    LYSP_NODE_CHECK(ll, "test", 1, 0x446, 1, "ll", 0, LYS_LEAFLIST, 0, "test", 1);
    assert_non_null(ll->dflts);
    assert_int_equal(2, LY_ARRAY_COUNT(ll->dflts));
    assert_string_equal("xxx", ll->dflts[0].str);
    assert_string_equal("yyy", ll->dflts[1].str);
    assert_string_equal("zzz", ll->units);
    assert_int_equal(10, ll->max);
    assert_int_equal(0, ll->min);
    assert_string_equal("string", ll->type.name);
    assert_non_null(ll->musts);
    assert_int_equal(LYS_CONFIG_R | LYS_STATUS_CURR | LYS_ORDBY_USER | LYS_SET_MAX, ll->flags);
    lysp_node_free(ctx->parsed_mod->mod->ctx, (struct lysp_node*)ll); ll = NULL;

    /* full content - now with min-elements */
    in.current = "ll {min-elements 10; type string;} ...";
    assert_int_equal(LY_SUCCESS, parse_leaflist(ctx, &in, NULL, (struct lysp_node**)&ll));
    LYSP_NODE_CHECK(ll, NULL, 0, 0x200, 0, "ll", 0, LYS_LEAFLIST, 0, NULL, 0);
    assert_string_equal("string", ll->type.name);
    assert_int_equal(0, ll->max);
    assert_int_equal(10, ll->min);
    assert_int_equal(LYS_SET_MIN, ll->flags);
    lysp_node_free(ctx->parsed_mod->mod->ctx, (struct lysp_node*)ll); ll = NULL;

    /* invalid */
    in.current = " ll {description \"missing type\";} ...";
    assert_int_equal(LY_EVALID, parse_leaflist(ctx, &in, NULL, (struct lysp_node**)&ll));
    err_msg[0] = "Missing mandatory keyword \"type\" as a child of \"leaf-list\".";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    lysp_node_free(ctx->parsed_mod->mod->ctx, (struct lysp_node*)ll); ll = NULL;

    ctx->parsed_mod->version = 1; /* simulate YANG 1.0 - default statement is not allowed */
    in.current = " ll {default xx; type string;} ...";
    assert_int_equal(LY_EVALID, parse_leaflist(ctx, &in, NULL, (struct lysp_node**)&ll));
    err_msg[0] = "Invalid keyword \"default\" as a child of \"leaf-list\" - the statement is allowed only in YANG 1.1 modules.";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    lysp_node_free(ctx->parsed_mod->mod->ctx, (struct lysp_node*)ll); ll = NULL;

    PARSER_DESTROY(ctx);
}

static void
test_list(void **state)
{
    (void) state;
    struct lys_yang_parser_ctx *ctx;
    struct lysp_node_list *l = NULL;
    struct ly_in in = {0};
    char *err_msg[1];
    char *err_path[1];

    PARSER_CREATE(ctx);

    ctx->parsed_mod->version = 2; /* simulate YANG 1.1 */

    /* invalid cardinality */
#define TEST_DUP(MEMBER, VALUE1, VALUE2) \
    in.current = "l {" MEMBER" "VALUE1";"MEMBER" "VALUE2";} ..."; \
    assert_int_equal(LY_EVALID, parse_list(ctx, &in, NULL, (struct lysp_node**)&l)); \
    err_msg[0] = "Duplicate keyword \""MEMBER"\".";\
    err_path[0] = "Line number 1.";\
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path); \
    lysp_node_free(ctx->parsed_mod->mod->ctx, (struct lysp_node*)l); l = NULL;

    TEST_DUP("config", "true", "false");
    TEST_DUP("description", "text1", "text2");
    TEST_DUP("key", "one", "two");
    TEST_DUP("max-elements", "10", "20");
    TEST_DUP("min-elements", "10", "20");
    TEST_DUP("ordered-by", "user", "system");
    TEST_DUP("reference", "1", "2");
    TEST_DUP("status", "current", "obsolete");
    TEST_DUP("when", "true", "false");
#undef TEST_DUP

    /* full content */
    in.current = "l {action x;anydata any;anyxml anyxml; choice ch;config false;container c;description test;grouping g;if-feature f; key l; leaf l {type string;}"
          "leaf-list ll {type string;} list li;max-elements 10; min-elements 1;must 'expr';notification not; ordered-by system; reference test;"
          "status current;typedef t {type int8;}unique xxx;unique yyy;uses g;when true;m:ext;} ...";
    assert_int_equal(LY_SUCCESS, parse_list(ctx, &in, NULL, (struct lysp_node**)&l));
    uint16_t flags = LYS_CONFIG_R | LYS_STATUS_CURR | LYS_ORDBY_SYSTEM | LYS_SET_MAX | LYS_SET_MIN;
    LYSP_NODE_CHECK(l, "test", 1, flags, 1, "l", 0, LYS_LIST, 0, "test", 1);
    assert_string_equal("l", l->key);
    assert_non_null(l->uniques);
    assert_int_equal(2, LY_ARRAY_COUNT(l->uniques));
    assert_string_equal("xxx", l->uniques[0].str);
    assert_string_equal("yyy", l->uniques[1].str);
    assert_int_equal(10, l->max);
    assert_int_equal(1, l->min);
    assert_non_null(l->musts);
    ly_set_erase(&ctx->tpdfs_nodes, NULL);
    lysp_node_free(ctx->parsed_mod->mod->ctx, (struct lysp_node*)l); l = NULL;

    /* invalid content */
    ctx->parsed_mod->version = 1; /* simulate YANG 1.0 */
    in.current = "l {action x;} ...";
    assert_int_equal(LY_EVALID, parse_list(ctx, &in, NULL, (struct lysp_node**)&l));
    err_msg[0] = "Invalid keyword \"action\" as a child of \"list\" - the statement is allowed only in YANG 1.1 modules.";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    lysp_node_free(ctx->parsed_mod->mod->ctx, (struct lysp_node*)l); l = NULL;

    PARSER_DESTROY(ctx);
}

static void
test_choice(void **state)
{
    (void) state;
    struct lys_yang_parser_ctx *ctx;
    struct lysp_node_choice *ch = NULL;
    struct ly_in in = {0};
    char *err_msg[1];
    char *err_path[1];

    PARSER_CREATE(ctx);

    ctx->parsed_mod->version = 2; /* simulate YANG 1.1 */

    /* invalid cardinality */
#define TEST_DUP(MEMBER, VALUE1, VALUE2) \
    in.current = "ch {" MEMBER" "VALUE1";"MEMBER" "VALUE2";} ..."; \
    assert_int_equal(LY_EVALID, parse_choice(ctx, &in, NULL, (struct lysp_node**)&ch)); \
    err_msg[0] = "Duplicate keyword \""MEMBER"\".";\
    err_path[0] = "Line number 1.";\
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path); \
    lysp_node_free(ctx->parsed_mod->mod->ctx, (struct lysp_node*)ch); ch = NULL;

    TEST_DUP("config", "true", "false");
    TEST_DUP("default", "a", "b");
    TEST_DUP("description", "text1", "text2");
    TEST_DUP("mandatory", "true", "false");
    TEST_DUP("reference", "1", "2");
    TEST_DUP("status", "current", "obsolete");
    TEST_DUP("when", "true", "false");
#undef TEST_DUP

    /* full content - without default due to a collision with mandatory */
    in.current = "ch {anydata any;anyxml anyxml; case c;choice ch;config false;container c;description test;if-feature f;leaf l {type string;}"
          "leaf-list ll {type string;} list li;mandatory true;reference test;status current;when true;m:ext;} ...";
    assert_int_equal(LY_SUCCESS, parse_choice(ctx, &in, NULL, (struct lysp_node**)&ch));
    uint16_t flags = LYS_CONFIG_R | LYS_STATUS_CURR | LYS_MAND_TRUE;
    LYSP_NODE_CHECK(ch, "test", 1, flags, 1, "ch", 0, LYS_CHOICE, 0, "test", 1);
    lysp_node_free(ctx->parsed_mod->mod->ctx, (struct lysp_node*)ch); ch = NULL;

    /* full content - the default missing from the previous node */
    in.current = "ch {default c;case c;} ...";
    assert_int_equal(LY_SUCCESS, parse_choice(ctx, &in, NULL, (struct lysp_node**)&ch));
    LYSP_NODE_CHECK(ch, NULL, 0, 0, 0, "ch", 0, LYS_CHOICE, 0, NULL, 0);
    assert_string_equal("c", ch->dflt.str);
    lysp_node_free(ctx->parsed_mod->mod->ctx, (struct lysp_node*)ch); ch = NULL;

    PARSER_DESTROY(ctx);
}

static void
test_case(void **state)
{
    (void) state;
    struct lys_yang_parser_ctx *ctx;
    struct lysp_node_case *cs = NULL;
    struct ly_in in = {0};
    char *err_msg[1];
    char *err_path[1];

    PARSER_CREATE(ctx);

    ctx->parsed_mod->version = 2; /* simulate YANG 1.1 */

    /* invalid cardinality */
#define TEST_DUP(MEMBER, VALUE1, VALUE2) \
    in.current = "cs {" MEMBER" "VALUE1";"MEMBER" "VALUE2";} ..."; \
    assert_int_equal(LY_EVALID, parse_case(ctx, &in, NULL, (struct lysp_node**)&cs)); \
    err_msg[0] = "Duplicate keyword \""MEMBER"\".";\
    err_path[0] = "Line number 1.";\
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path); \
    lysp_node_free(ctx->parsed_mod->mod->ctx, (struct lysp_node*)cs); cs = NULL;

    TEST_DUP("description", "text1", "text2");
    TEST_DUP("reference", "1", "2");
    TEST_DUP("status", "current", "obsolete");
    TEST_DUP("when", "true", "false");
#undef TEST_DUP

    /* full content */
    in.current = "cs {anydata any;anyxml anyxml; choice ch;container c;description test;if-feature f;leaf l {type string;}"
          "leaf-list ll {type string;} list li;reference test;status current;uses grp;when true;m:ext;} ...";
    assert_int_equal(LY_SUCCESS, parse_case(ctx, &in, NULL, (struct lysp_node**)&cs));
    LYSP_NODE_CHECK(cs, "test", 1, LYS_STATUS_CURR, 1, "cs", 0, LYS_CASE, 0, "test", 1);
    lysp_node_free(ctx->parsed_mod->mod->ctx, (struct lysp_node*)cs); cs = NULL;

    /* invalid content */
    in.current = "cs {config true} ...";
    assert_int_equal(LY_EVALID, parse_case(ctx, &in, NULL, (struct lysp_node**)&cs));
    err_msg[0] = "Invalid keyword \"config\" as a child of \"case\".";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    lysp_node_free(ctx->parsed_mod->mod->ctx, (struct lysp_node*)cs); cs = NULL;

    PARSER_DESTROY(ctx);
}

static void
test_any(enum ly_stmt kw)
{
    struct lys_yang_parser_ctx *ctx;
    struct lysp_node_anydata *any = NULL;
    struct ly_in in = {0};
    char *err_msg[1];
    char *err_path[1];

    PARSER_CREATE(ctx);

    if (kw == LY_STMT_ANYDATA) {
        ctx->parsed_mod->version = 2; /* simulate YANG 1.1 */
    } else {
        ctx->parsed_mod->version = 1; /* simulate YANG 1.0 */
    }

    /* invalid cardinality */
#define TEST_DUP(MEMBER, VALUE1, VALUE2) \
    in.current = "l {" MEMBER" "VALUE1";"MEMBER" "VALUE2";} ..."; \
    assert_int_equal(LY_EVALID, parse_any(ctx, &in, kw, NULL, (struct lysp_node**)&any)); \
    err_msg[0] = "Duplicate keyword \""MEMBER"\".";\
    err_path[0] = "Line number 1.";\
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path); \
    lysp_node_free(ctx->parsed_mod->mod->ctx, (struct lysp_node*)any); any = NULL;

    TEST_DUP("config", "true", "false");
    TEST_DUP("description", "text1", "text2");
    TEST_DUP("mandatory", "true", "false");
    TEST_DUP("reference", "1", "2");
    TEST_DUP("status", "current", "obsolete");
    TEST_DUP("when", "true", "false");
#undef TEST_DUP

    /* full content */
    in.current = "any {config true;description test;if-feature f;mandatory true;must 'expr';reference test;status current;when true;m:ext;} ...";
    assert_int_equal(LY_SUCCESS, parse_any(ctx, &in, kw, NULL, (struct lysp_node**)&any));
    //LYSP_NODE_CHECK(NODE, DSC, EXTS, FLAGS, IFFEATURES, NAME, NEXT, TYPE, PARENT, REF, WHEN)
    uint16_t node_type = kw == LY_STMT_ANYDATA ? LYS_ANYDATA : LYS_ANYXML;
    uint16_t flags     = LYS_CONFIG_W | LYS_STATUS_CURR | LYS_MAND_TRUE;
    LYSP_NODE_CHECK(any, "test", 1, flags, 1, "any", 0, node_type, 0, "test", 1);
    assert_non_null(any->musts);
    lysp_node_free(ctx->parsed_mod->mod->ctx, (struct lysp_node*)any); any = NULL;

    PARSER_DESTROY(ctx);
}

static void
test_anydata(void **state)
{
    (void) state;
    return test_any(LY_STMT_ANYDATA);
}

static void
test_anyxml(void **state)
{
    (void) state;
    return test_any(LY_STMT_ANYXML);
}

static void
test_grouping(void **state)
{
    (void) state;
    struct lys_yang_parser_ctx *ctx = *state;
    struct lysp_grp *grp = NULL;
    struct ly_in in = {0};
    char *err_msg[1];
    char *err_path[1];

    PARSER_CREATE(ctx);

    ctx->parsed_mod->version = 2; /* simulate YANG 1.1 */

    /* invalid cardinality */
#define TEST_DUP(MEMBER, VALUE1, VALUE2) \
    in.current = "l {" MEMBER" "VALUE1";"MEMBER" "VALUE2";} ..."; \
    assert_int_equal(LY_EVALID, parse_grouping(ctx, &in, NULL, &grp)); \
    err_msg[0] = "Duplicate keyword \""MEMBER"\".";\
    err_path[0] = "Line number 1.";\
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path); \
    FREE_ARRAY(ctx->parsed_mod->mod->ctx, grp, lysp_grp_free); grp = NULL;

    TEST_DUP("description", "text1", "text2");
    TEST_DUP("reference", "1", "2");
    TEST_DUP("status", "current", "obsolete");
#undef TEST_DUP

    /* full content */
    in.current = "grp {action x;anydata any;anyxml anyxml; choice ch;container c;description test;grouping g;leaf l {type string;}"
          "leaf-list ll {type string;} list li;notification not;reference test;status current;typedef t {type int8;}uses g;m:ext;} ...";
    assert_int_equal(LY_SUCCESS, parse_grouping(ctx, &in, NULL, &grp));
    assert_non_null(grp);
    assert_int_equal(LYS_GROUPING, grp->nodetype);
    assert_string_equal("grp", grp->name);
    assert_string_equal("test", grp->dsc);
    assert_non_null(grp->exts);
    assert_string_equal("test", grp->ref);
    assert_null(grp->parent);
    assert_int_equal( LYS_STATUS_CURR, grp->flags);
    ly_set_erase(&ctx->tpdfs_nodes, NULL);
    FREE_ARRAY(ctx->parsed_mod->mod->ctx, grp, lysp_grp_free); grp = NULL;

    /* invalid content */
    in.current = "grp {config true} ...";
    assert_int_equal(LY_EVALID, parse_grouping(ctx, &in, NULL, &grp));
    err_msg[0] = "Invalid keyword \"config\" as a child of \"grouping\".";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    FREE_ARRAY(ctx->parsed_mod->mod->ctx, grp, lysp_grp_free); grp = NULL;

    in.current = "grp {must 'expr'} ...";
    assert_int_equal(LY_EVALID, parse_grouping(ctx, &in, NULL, &grp));
    err_msg[0] = "Invalid keyword \"must\" as a child of \"grouping\".";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    FREE_ARRAY(ctx->parsed_mod->mod->ctx, grp, lysp_grp_free); grp = NULL;

    PARSER_DESTROY(ctx);
}

static void
test_action(void **state)
{
    (void) state;
    struct lys_yang_parser_ctx *ctx;
    struct lysp_action *rpcs = NULL;
    struct lysp_node_container *c = NULL;
    struct ly_in in = {0};
    char *err_msg[1];
    char *err_path[1];

    PARSER_CREATE(ctx);

    ctx->parsed_mod->version = 2; /* simulate YANG 1.1 */

    /* invalid cardinality */
#define TEST_DUP(MEMBER, VALUE1, VALUE2) \
    in.current = "func {" MEMBER" "VALUE1";"MEMBER" "VALUE2";} ..."; \
    assert_int_equal(LY_EVALID, parse_action(ctx, &in, NULL, &rpcs)); \
    err_msg[0] = "Duplicate keyword \""MEMBER"\".";\
    err_path[0] = "Line number 1.";\
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path); \
    FREE_ARRAY(ctx->parsed_mod->mod->ctx, rpcs, lysp_action_free); rpcs = NULL;

    TEST_DUP("description", "text1", "text2");
    TEST_DUP("input", "{leaf l1 {type empty;}} description a", "{leaf l2 {type empty;}} description a");
    TEST_DUP("output", "{leaf l1 {type empty;}} description a", "{leaf l2 {type empty;}} description a");
    TEST_DUP("reference", "1", "2");
    TEST_DUP("status", "current", "obsolete");
#undef TEST_DUP

    /* full content */
    in.current = "top;";
    assert_int_equal(LY_SUCCESS, parse_container(ctx, &in, NULL, (struct lysp_node**)&c));
    in.current = "func {description test;grouping grp;if-feature f;reference test;status current;typedef mytype {type int8;} m:ext;"
          "input {anydata a1; anyxml a2; choice ch; container c; grouping grp; leaf l {type int8;} leaf-list ll {type int8;}"
          " list li; must 1; typedef mytypei {type int8;} uses grp; m:ext;}"
          "output {anydata a1; anyxml a2; choice ch; container c; grouping grp; leaf l {type int8;} leaf-list ll {type int8;}"
          " list li; must 1; typedef mytypeo {type int8;} uses grp; m:ext;}} ...";
    assert_int_equal(LY_SUCCESS, parse_action(ctx, &in, (struct lysp_node*)c, &rpcs));
    assert_non_null(rpcs);
    assert_int_equal(LYS_ACTION, rpcs->nodetype);
    assert_string_equal("func", rpcs->name);
    assert_string_equal("test", rpcs->dsc);
    assert_non_null(rpcs->exts);
    assert_non_null(rpcs->iffeatures);
    assert_string_equal("test", rpcs->ref);
    assert_non_null(rpcs->groupings);
    assert_non_null(rpcs->typedefs);
    assert_int_equal(LYS_STATUS_CURR, rpcs->flags);
    /* input */
    assert_int_equal(rpcs->input.nodetype, LYS_INPUT);
    assert_non_null(rpcs->input.groupings);
    assert_non_null(rpcs->input.exts);
    assert_non_null(rpcs->input.musts);
    assert_non_null(rpcs->input.typedefs);
    assert_non_null(rpcs->input.data);
    /* output */
    assert_int_equal(rpcs->output.nodetype, LYS_OUTPUT);
    assert_non_null(rpcs->output.groupings);
    assert_non_null(rpcs->output.exts);
    assert_non_null(rpcs->output.musts);
    assert_non_null(rpcs->output.typedefs);
    assert_non_null(rpcs->output.data);

    ly_set_erase(&ctx->tpdfs_nodes, NULL);
    FREE_ARRAY(ctx->parsed_mod->mod->ctx, rpcs, lysp_action_free); rpcs = NULL;

    /* invalid content */
    in.current = "func {config true} ...";
    assert_int_equal(LY_EVALID, parse_action(ctx, &in, NULL, &rpcs));
    err_msg[0] = "Invalid keyword \"config\" as a child of \"rpc\".";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    FREE_ARRAY(ctx->parsed_mod->mod->ctx, rpcs, lysp_action_free); rpcs = NULL;

    lysp_node_free(ctx->parsed_mod->mod->ctx, (struct lysp_node*)c);

    PARSER_DESTROY(ctx);
}

static void
test_notification(void **state)
{
    (void) state;
    struct lys_yang_parser_ctx *ctx;
    struct lysp_notif *notifs = NULL;
    struct lysp_node_container *c = NULL;
    struct ly_in in = {0};
    char *err_msg[1];
    char *err_path[1];

    PARSER_CREATE(ctx);

    ctx->parsed_mod->version = 2; /* simulate YANG 1.1 */

    /* invalid cardinality */
#define TEST_DUP(MEMBER, VALUE1, VALUE2) \
    in.current = "func {" MEMBER" "VALUE1";"MEMBER" "VALUE2";} ..."; \
    assert_int_equal(LY_EVALID, parse_notif(ctx, &in, NULL, &notifs)); \
    err_msg[0] = "Duplicate keyword \""MEMBER"\".";\
    err_path[0] = "Line number 1.";\
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path); \
    FREE_ARRAY(ctx->parsed_mod->mod->ctx, notifs, lysp_notif_free); notifs = NULL;

    TEST_DUP("description", "text1", "text2");
    TEST_DUP("reference", "1", "2");
    TEST_DUP("status", "current", "obsolete");
#undef TEST_DUP

    /* full content */
    in.current = "top;";
    assert_int_equal(LY_SUCCESS, parse_container(ctx, &in, NULL, (struct lysp_node**)&c));
    in.current = "ntf {anydata a1; anyxml a2; choice ch; container c; description test; grouping grp; if-feature f; leaf l {type int8;}"
          "leaf-list ll {type int8;} list li; must 1; reference test; status current; typedef mytype {type int8;} uses grp; m:ext;}";
    assert_int_equal(LY_SUCCESS, parse_notif(ctx, &in, (struct lysp_node*)c, &notifs));
    assert_non_null(notifs);
    assert_int_equal(LYS_NOTIF, notifs->nodetype);
    assert_string_equal("ntf", notifs->name);
    assert_string_equal("test", notifs->dsc);
    assert_non_null(notifs->exts);
    assert_non_null(notifs->iffeatures);
    assert_string_equal("test", notifs->ref);
    assert_non_null(notifs->groupings);
    assert_non_null(notifs->typedefs);
    assert_non_null(notifs->musts);
    assert_non_null(notifs->data);
    assert_int_equal(LYS_STATUS_CURR, notifs->flags);

    ly_set_erase(&ctx->tpdfs_nodes, NULL);
    FREE_ARRAY(ctx->parsed_mod->mod->ctx, notifs, lysp_notif_free); notifs = NULL;

    /* invalid content */
    in.current = "ntf {config true} ...";
    assert_int_equal(LY_EVALID, parse_notif(ctx, &in, NULL, &notifs));
    err_msg[0] = "Invalid keyword \"config\" as a child of \"notification\".";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);
    FREE_ARRAY(ctx->parsed_mod->mod->ctx, notifs, lysp_notif_free); notifs = NULL;

    lysp_node_free(ctx->parsed_mod->mod->ctx, (struct lysp_node*)c);

    PARSER_DESTROY(ctx);
}

static void
test_uses(void **state)
{
    (void) state;
    struct lys_yang_parser_ctx *ctx;
    struct lysp_node_uses *u = NULL;
    struct ly_in in = {0};
    char *err_msg[1];
    char *err_path[1];

    PARSER_CREATE(ctx);

    ctx->parsed_mod->version = 2; /* simulate YANG 1.1 */

    /* invalid cardinality */
#define TEST_DUP(MEMBER, VALUE1, VALUE2) \
    in.current = "l {" MEMBER" "VALUE1";"MEMBER" "VALUE2";} ..."; \
    assert_int_equal(LY_EVALID, parse_uses(ctx, &in, NULL, (struct lysp_node**)&u)); \
    err_msg[0] = "Duplicate keyword \""MEMBER"\".";\
    err_path[0] = "Line number 1.";\
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path); \
    lysp_node_free(ctx->parsed_mod->mod->ctx, (struct lysp_node*)u); u = NULL;

    TEST_DUP("description", "text1", "text2");
    TEST_DUP("reference", "1", "2");
    TEST_DUP("status", "current", "obsolete");
    TEST_DUP("when", "true", "false");
#undef TEST_DUP

    /* full content */
    in.current = "grpref {augment some/node;description test;if-feature f;reference test;refine some/other/node;status current;when true;m:ext;} ...";
    assert_int_equal(LY_SUCCESS, parse_uses(ctx, &in, NULL, (struct lysp_node**)&u));
    LYSP_NODE_CHECK(u, "test", 1, LYS_STATUS_CURR, 1, "grpref", 0, LYS_USES, 0, "test", 1);
    assert_non_null(u->augments);
    assert_non_null(u->refines);
    lysp_node_free(ctx->parsed_mod->mod->ctx, (struct lysp_node*)u); u = NULL;

    PARSER_DESTROY(ctx);
}

static void
test_augment(void **state)
{
    (void) state;
    struct lys_yang_parser_ctx *ctx;
    struct lysp_augment *a = NULL;
    struct ly_in in = {0};
    char *err_msg[1];
    char *err_path[1];

    PARSER_CREATE(ctx);

    ctx->parsed_mod->version = 2; /* simulate YANG 1.1 */

    /* invalid cardinality */
#define TEST_DUP(MEMBER, VALUE1, VALUE2) \
    in.current = "l {" MEMBER" "VALUE1";"MEMBER" "VALUE2";} ..."; \
    assert_int_equal(LY_EVALID, parse_augment(ctx, &in, NULL, &a)); \
    err_msg[0] = "Duplicate keyword \""MEMBER"\".";\
    err_path[0] = "Line number 1.";\
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path); \
    FREE_ARRAY(ctx->parsed_mod->mod->ctx, a, lysp_augment_free); a = NULL;

    TEST_DUP("description", "text1", "text2");
    TEST_DUP("reference", "1", "2");
    TEST_DUP("status", "current", "obsolete");
    TEST_DUP("when", "true", "false");
#undef TEST_DUP

    /* full content */
    in.current = "/target/nodeid {action x; anydata any;anyxml anyxml; case cs; choice ch;container c;description test;if-feature f;leaf l {type string;}"
          "leaf-list ll {type string;} list li;notification not;reference test;status current;uses g;when true;m:ext;} ...";
    assert_int_equal(LY_SUCCESS, parse_augment(ctx, &in, NULL, &a));
    assert_non_null(a);
    assert_int_equal(LYS_AUGMENT, a->nodetype);
    assert_string_equal("/target/nodeid", a->nodeid);
    assert_string_equal("test", a->dsc);
    assert_non_null(a->exts);
    assert_non_null(a->iffeatures);
    assert_string_equal("test", a->ref);
    assert_non_null(a->when);
    assert_null(a->parent);
    assert_int_equal(LYS_STATUS_CURR, a->flags);
    FREE_ARRAY(ctx->parsed_mod->mod->ctx, a, lysp_augment_free); a = NULL;

    PARSER_DESTROY(ctx);
}

static void
test_when(void **state)
{
    (void) state;
    struct lys_yang_parser_ctx *ctx;
    struct lysp_when *w = NULL;
    struct ly_in in = {0};
    char *err_msg[1];
    char *err_path[1];

    PARSER_CREATE(ctx);

    ctx->parsed_mod->version = 2; /* simulate YANG 1.1 */

    /* invalid cardinality */
#define TEST_DUP(MEMBER, VALUE1, VALUE2) \
    in.current = "l {" MEMBER" "VALUE1";"MEMBER" "VALUE2";} ..."; \
    assert_int_equal(LY_EVALID, parse_when(ctx, &in, &w)); \
    err_msg[0] = "Duplicate keyword \""MEMBER"\".";\
    err_path[0] = "Line number 1.";\
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path); \
    FREE_MEMBER(ctx->parsed_mod->mod->ctx, w, lysp_when_free); w = NULL;

    TEST_DUP("description", "text1", "text2");
    TEST_DUP("reference", "1", "2");
#undef TEST_DUP

    /* full content */
    in.current = "expression {description test;reference test;m:ext;} ...";
    assert_int_equal(LY_SUCCESS, parse_when(ctx, &in, &w));
    assert_non_null(w);
    assert_string_equal("expression", w->cond);
    assert_string_equal("test", w->dsc);
    assert_string_equal("test", w->ref);
    assert_non_null(w->exts);
    FREE_MEMBER(ctx->parsed_mod->mod->ctx, w, lysp_when_free); w = NULL;

    /* empty condition */
    in.current = "\"\";";
    assert_int_equal(LY_SUCCESS, parse_when(ctx, &in, &w));
    logbuf_assert("Empty argument of when statement does not make sense.");
    assert_non_null(w);
    assert_string_equal("", w->cond);
    FREE_MEMBER(ctx->parsed_mod->mod->ctx, w, lysp_when_free); w = NULL;

    PARSER_DESTROY(ctx);
}

static void
test_value(void **state)
{
    (void) state;
    struct lys_yang_parser_ctx *ctx;
    struct ly_in in = {0};
    int64_t val = 0;
    uint16_t flags = 0;
    char *err_msg[1];
    char *err_path[1];

    PARSER_CREATE(ctx);

    in.current = "-0;";
    assert_int_equal(parse_type_enum_value_pos(ctx, &in, LY_STMT_VALUE, &val, &flags, NULL), LY_SUCCESS);
    assert_int_equal(val, 0);

    in.current = "-0;";
    flags = 0;
    assert_int_equal(parse_type_enum_value_pos(ctx, &in, LY_STMT_POSITION, &val, &flags, NULL), LY_EVALID);
    err_msg[0] = "Invalid value \"-0\" of \"position\".";
    err_path[0] = "Line number 1.";
    LY_ERROR_CHECK(CONTEXT_GET, err_msg, err_path);

    PARSER_DESTROY(ctx);
}

int main(void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_helpers),
        cmocka_unit_test(test_comments),
        cmocka_unit_test(test_arg),
        cmocka_unit_test(test_stmts),
        cmocka_unit_test(test_minmax),
        cmocka_unit_test(test_module),
        cmocka_unit_test(test_deviation),
        cmocka_unit_test(test_deviate),
        cmocka_unit_test(test_container),
        cmocka_unit_test(test_leaf),
        cmocka_unit_test(test_leaflist),
        cmocka_unit_test(test_list),
        cmocka_unit_test(test_choice),
        cmocka_unit_test(test_case),
        cmocka_unit_test(test_anydata),
        cmocka_unit_test(test_anyxml),
        cmocka_unit_test(test_action),
        cmocka_unit_test(test_notification),
        cmocka_unit_test(test_grouping),
        cmocka_unit_test(test_uses),
        cmocka_unit_test(test_augment),
        cmocka_unit_test(test_when),
        cmocka_unit_test(test_value),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
