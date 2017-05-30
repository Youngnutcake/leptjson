#ifndef LEPTJSON_H__
#define LEPTJSON_H__
#include <assert.h>
#include <stdlib.h>
#include <errno.h>   /* errno, ERANGE */
#include <math.h>    /* HUGE_VAL */

#define EXPECT(c,ch) do { assert(*c->json == ch); c->json++; }while(0)

typedef enum { LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT } lept_type;

struct lept_member;
struct lept_value;
struct lept_value
{
	union
	{
		struct { lept_member* m; size_t size; } o;/* object */
		struct { lept_value* e; size_t size; }a; /* array */
		struct
		{
			char* s;
			size_t len;
		} s;
		double n;
	}  u;
	lept_type type;
};

struct lept_member
{
	char *k; size_t klen;//很多时候都是这种写法，动态内存需要记录指针的大小,所以是k指针和klen的长度组合
	lept_value v;
};

enum {
	LEPT_PARSE_OK = 0,
	LEPT_PARSE_EXPECT_VALUE,
	LEPT_PARSE_INVALID_VALUE,
	LEPT_PARSE_ROOT_NOT_SINGULAR,
	LEPT_PARSE_NUMBER_TOO_BIG,
	LEPT_PARSE_MISS_QUOTATION_MARK,
	LEPT_PARSE_INVALID_STRING_ESCAPE,
	LEPT_PARSE_INVALID_STRING_CHAR,
	LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET,
	LEPT_PARSE_MISS_KEY,
	LEPT_PARSE_MISS_COLON,
	LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET
};

struct lept_context
{
	const char* json;
	char* stack;
	size_t size, top;
};

#define lept_init(v) do { (v)->type = LEPT_NULL; } while(0)

int lept_parse(lept_value* v, const char* json);

void lept_free(lept_value* v);

lept_type lept_get_type(const lept_value* v);

#define lept_set_null(v) lept_free(v)

int lept_get_boolean(const lept_value* v);
void lept_set_boolean(lept_value* v, int b);

double lept_get_number(const lept_value* v);
void lept_set_number(lept_value* v, double n);

const char* lept_get_string(const lept_value* v);
size_t lept_get_string_length(const lept_value* v);
void lept_set_string(lept_value* v, const char* s, size_t len);

size_t lept_get_array_size(const lept_value* v);
lept_value* lept_get_array_element(const lept_value* v, size_t index);

size_t lept_get_object_size(const lept_value* v);
const char* lept_get_object_key(const lept_value* v, size_t index);
size_t lept_get_object_key_length(const lept_value* v, size_t index);
lept_value* lept_get_object_value(const lept_value* v, size_t index);

#endif // !LEPTJSON_H__
