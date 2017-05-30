
#include "leptjson.h"
#include <stdio.h>

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <string.h>


static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

#define EXPECT_EQ_BASE(equality, expect, actual, format) \
	do {\
		test_count++;\
		if (equality)\
			test_pass++;\
		else {\
            fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual);\
            main_ret = 1;\
		}\
	} while (0)

#define EXPECT_EQ_INT( expect, actual) EXPECT_EQ_BASE((expect==actual), expect, actual, "%d")


#define TEST_ERROR(error, json)\
    do {\
        lept_value v;\
        v.type = LEPT_FALSE;\
        EXPECT_EQ_INT(error, lept_parse(&v, json));\
        EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));\
    } while(0)

#define EXPECT_EQ_DOUBLE( expect, actual) EXPECT_EQ_BASE((expect==actual), expect, actual, "%2g")

#define TEST_NUMBER(expect, json)\
	do\
	{\
		lept_value v;\
        EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, json));\
        EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(&v));\
        EXPECT_EQ_DOUBLE(expect, lept_get_number(&v));\
	}while (0)

#define EXPECT_TRUE(actual) EXPECT_EQ_BASE((actual) != 0, "true", "false", "%s")
#define EXPECT_FALSE(actual) EXPECT_EQ_BASE((actual) == 0, "false", "true", "%s")
#define EXPECT_EQ_STRING(expect, actual, alength) \
    EXPECT_EQ_BASE(sizeof(expect) - 1 == alength && memcmp(expect, actual, alength) == 0, expect, actual, "%s")


static void test_parse_null() {
	lept_value v;
	v.type = LEPT_TRUE;
	EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "null"));
	EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
}

static void test_parse_true() {
	lept_value v;
	v.type = LEPT_FALSE;
	EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "true"));
	EXPECT_EQ_INT(LEPT_TRUE, lept_get_type(&v));
}

static void test_parse_false() {
	lept_value v;
	v.type = LEPT_TRUE;
	EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "false"));
	EXPECT_EQ_INT(LEPT_FALSE, lept_get_type(&v));
}

static void test_parse_expect_value() {
	TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, "");
	TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, " ");
}

/*----------------------Êý×Ö-----------------------------*/

static void test_parse_number() {
	TEST_NUMBER(0.0, "0");
	TEST_NUMBER(0.0, "-0");
	TEST_NUMBER(0.0, "-0.0");
	TEST_NUMBER(1.0, "1");
	TEST_NUMBER(-1.0, "-1");
	TEST_NUMBER(1.5, "1.5");
	TEST_NUMBER(-1.5, "-1.5");
	TEST_NUMBER(3.1416, "3.1416");
	TEST_NUMBER(1E10, "1E10");
	TEST_NUMBER(1e10, "1e10");
	TEST_NUMBER(1E+10, "1E+10");
	TEST_NUMBER(1E-10, "1E-10");
	TEST_NUMBER(-1E10, "-1E10");
	TEST_NUMBER(-1e10, "-1e10");
	TEST_NUMBER(-1E+10, "-1E+10");
	TEST_NUMBER(-1E-10, "-1E-10");
	TEST_NUMBER(1.234E+10, "1.234E+10");
	TEST_NUMBER(1.234E-10, "1.234E-10");
	TEST_NUMBER(0.0, "1e-10000"); /* must underflow */
}

static void test_parse_root_not_singular() {
	TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "null x");

#if 1
	/* invalid number */
	TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0123"); /* after zero should be '.' or nothing */
	TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0x0");
	TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0x123");
#endif
}

static void test_parse_number_too_big() {
#if 1
	TEST_ERROR(LEPT_PARSE_NUMBER_TOO_BIG, "1e309");
	TEST_ERROR(LEPT_PARSE_NUMBER_TOO_BIG, "-1e309");
#endif
}
/*---------------------×Ö·û´®-----------------------------*/
#define TEST_STRING(expect, json)\
    do {\
        lept_value v;\
        lept_init(&v);\
        EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, json));\
        EXPECT_EQ_INT(LEPT_STRING, lept_get_type(&v));\
        EXPECT_EQ_STRING(expect, lept_get_string(&v), lept_get_string_length(&v));\
        lept_free(&v);\
    } while(0)

static void test_parse_string() {
	TEST_STRING("", "\"\"");
	TEST_STRING("Hello", "\"Hello\"");
#if 1
	TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
	TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
#endif
}


static void test_parse_missing_quotation_mark() {
	TEST_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK, "\"");
	TEST_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK, "\"abc");
}

static void test_parse_invalid_string_escape() {
#if 1
	TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\v\"");
	TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\'\"");
	TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\0\"");
	TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\x12\"");
#endif
}

static void test_parse_invalid_string_char() {
#if 1
	TEST_ERROR(LEPT_PARSE_INVALID_STRING_CHAR, "\"\x01\"");
	TEST_ERROR(LEPT_PARSE_INVALID_STRING_CHAR, "\"\x1F\"");
#endif
}
/*---------------------²âÊÔÊý×é--------------------------*/
#if defined(_MSC_VER)
#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE((expect) == (actual), (size_t)expect, (size_t)actual, "%Iu")
#else
#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE((expect) == (actual), (size_t)expect, (size_t)actual, "%zu")
#endif

static void test_parse_array() {
	lept_value v;

	lept_init(&v);
	EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "[ ]"));
	EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(&v));
	EXPECT_EQ_SIZE_T(0, lept_get_array_size(&v));

	lept_value v1;

	lept_init(&v1);
	EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v1, "[ null , false , true , 123 , \"abc\" ]"));
	EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(&v1));
	EXPECT_EQ_SIZE_T(5, lept_get_array_size(&v1));

	lept_value v2;

	lept_init(&v2);
	EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v2, "[  [ ] , [ 0 ] , [ 0 , 1 ] , [ 0 , 1 , 2 ]  ]"));
	EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(&v2));
	EXPECT_EQ_SIZE_T(4, lept_get_array_size(&v2));
	EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(lept_get_array_element(&v2, 3)));
	EXPECT_EQ_INT(3, lept_get_array_size(lept_get_array_element(&v2, 3)));

	lept_free(&v);
	lept_free(&v1);
	lept_free(&v2);
}
/*-----------------------²âÊÔ¶ÔÏó-----------------------------*/

static void test_parse_miss_key() {
	TEST_ERROR(LEPT_PARSE_MISS_KEY, "{:1,");
	TEST_ERROR(LEPT_PARSE_MISS_KEY, "{1:1,");
	TEST_ERROR(LEPT_PARSE_MISS_KEY, "{true:1,");
	TEST_ERROR(LEPT_PARSE_MISS_KEY, "{false:1,");
	TEST_ERROR(LEPT_PARSE_MISS_KEY, "{null:1,");
	TEST_ERROR(LEPT_PARSE_MISS_KEY, "{[]:1,");
	TEST_ERROR(LEPT_PARSE_MISS_KEY, "{{}:1,");
	TEST_ERROR(LEPT_PARSE_MISS_KEY, "{\"a\":1,");
}

static void test_parse_miss_colon() {
	TEST_ERROR(LEPT_PARSE_MISS_COLON, "{\"a\"}");
	TEST_ERROR(LEPT_PARSE_MISS_COLON, "{\"a\",\"b\"}");
}

static void test_parse_miss_comma_or_curly_bracket() {
	TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1");
	TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1]");
	TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1 \"b\"");
	TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":{}");
}


/*----------------------------------------------------*/

static void test_parse_invalid_value() {
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "nul");
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "?");

	/* invalid number */
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "+0");
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "+1");
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, ".123"); /* at least one digit before '.' */
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "1.");   /* at least one digit after '.' */
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "INF");
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "inf");
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "NAN");
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "nan");

	/* invalid value in array */
#if 1
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "[1,]");
	TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "[\"ab\", nul]");
#endif
}


static void test_access_null() {
	lept_value v;
	lept_init(&v);
	lept_set_string(&v, "a", 1);
	lept_set_null(&v);
	EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
	lept_free(&v);
}

static void test_access_boolean() {
	/* \TODO */
	lept_value v;
	lept_init(&v);
	lept_set_string(&v, "a", 1);
	lept_set_boolean(&v, 1);
	EXPECT_TRUE(lept_get_boolean(&v));
	lept_set_boolean(&v, 0);
	EXPECT_FALSE(lept_get_boolean(&v));
	lept_free(&v);
}

static void test_access_number() {
	lept_value v;
	lept_init(&v);
	lept_set_string(&v, "a", 1);
	lept_set_number(&v, 1234.5);
	EXPECT_EQ_DOUBLE(1234.5, lept_get_number(&v));
	lept_free(&v);
}

static void test_access_string() {
	lept_value v;
	lept_init(&v);
	lept_set_string(&v, "", 0);
	EXPECT_EQ_STRING("", lept_get_string(&v), lept_get_string_length(&v));
	lept_set_string(&v, "Hello", 5);
	EXPECT_EQ_STRING("Hello", lept_get_string(&v), lept_get_string_length(&v));
	lept_free(&v);
}
/*----------------------------------------------------*/
void test_all() {
	test_parse_null();
	test_parse_true();
	test_parse_false();
	test_parse_number();
	test_parse_string();
	test_parse_expect_value();
	test_parse_invalid_value();
	test_parse_root_not_singular();
	test_parse_number_too_big();
	test_parse_array();

	test_parse_missing_quotation_mark();
	test_parse_invalid_string_escape();
	test_parse_invalid_string_char();

	test_access_null();
	test_access_boolean();
	test_access_number();
	test_access_string();


	test_parse_miss_key();
	test_parse_miss_colon();
	test_parse_miss_comma_or_curly_bracket();
}
int main() {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	test_all();
	//test_parse_miss_key();
	//test_parse_miss_colon();
	//test_parse_miss_comma_or_curly_bracket();

	printf("%d/%d" "(%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);

	return main_ret;
}