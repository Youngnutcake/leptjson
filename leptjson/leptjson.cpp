// leptjson.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "leptjson.h"

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h> 

void lept_parse_whitespace(lept_context* c)
{
	const char*p = c->json;
	while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
	{
		++p;
	}
	c->json = p;
}

int lept_parse_literal(lept_context* c, lept_value* v, const char* literal, lept_type type) {
	size_t i;
	EXPECT(c, literal[0]);
	for (i = 0; literal[i + 1]; i++)
		if (c->json[i] != literal[i + 1])
			return LEPT_PARSE_INVALID_VALUE;
	c->json += i;
	v->type = type;
	return LEPT_PARSE_OK;
}

/*----------------------解析数字--------------------------*/
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')

int lept_parse_number(lept_context* c, lept_value* v) {
	/* \TODO validate number */
	const char *p = c->json;
	if (*p == '-') {
		p++;
	}
	if (*p == '0') {
		p++;
	}
	else {
		if (!ISDIGIT1TO9(*p)) return LEPT_PARSE_INVALID_VALUE;
		for (p++; ISDIGIT(*p); p++);
	}
	if (*p == '.') {
		p++;
		if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
		for (p++; ISDIGIT(*p); p++);
	}
	if (*p == 'e' || *p == 'E') {
		p++;
		if (*p == '+' || *p == '-') p++;
		if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
		for (p++; ISDIGIT(*p); p++);
	}

	v->u.n = strtod(c->json, NULL);
	if (errno == ERANGE && (v->u.n == HUGE_VAL || v->u.n == -HUGE_VAL))
		return LEPT_PARSE_NUMBER_TOO_BIG;
	c->json = p;
	v->type = LEPT_NUMBER;
	return LEPT_PARSE_OK;
}

/*----------------------解析字符串------------------------*/
#define lept_set_null(v) lept_free(v)


void lept_free(lept_value* v) {
	assert(v != NULL);
	if (v->type == LEPT_STRING)
	{
		free(v->u.s.s);
	}
	if (v->type == LEPT_ARRAY)
	{
		for (size_t i = 0; i < v->u.a.size; i++)
		{
			lept_free(&(v->u.a.e[i]));
		}
		free(v->u.a.e);
	}
	if (v->type == LEPT_OBJECT)
	{
		for (int i = 0; i < v->u.o.size; i++)
		{
			free(v->u.o.m[i].k);
			lept_free(&v->u.o.m[i].v);
		}
		free(v->u.o.m);
	}
	v->type = LEPT_NULL;
}

#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif

void* lept_context_push(lept_context* c, size_t size) {
	void* ret;
	assert(size > 0);
	if (c->top + size >= c->size) {
		if (c->size == 0)
			c->size = LEPT_PARSE_STACK_INIT_SIZE;
		while (c->top + size >= c->size)
			c->size += c->size >> 1;  /* c->size * 1.5 */
		c->stack = (char*)realloc(c->stack, c->size);
	}
	ret = c->stack + c->top;//ret记录分配前内存的位置，返回去用来填充
	c->top += size;//压入相应大小的内存后，top指向新的起点，也就是栈顶
	return ret;
}

void* lept_context_pop(lept_context* c, size_t size) {
	assert(c->top >= size);
	return c->stack + (c->top -= size);
}

#define PUTC(c, ch) do { *(char*)lept_context_push(c, sizeof(char)) = (ch); } while(0)

int lept_parse_string_raw(lept_context* c, char** str, size_t* len) {
	size_t head = c->top;
	const char* p;
	EXPECT(c, '\"');
	p = c->json;
	for (;;) {
		char ch = *p++;
		switch (ch) {
		case '\"':
			*len = c->top - head;
			*str = (char*)lept_context_pop(c, *len);
			c->json = p;
			return LEPT_PARSE_OK;
		case '\0':
			c->top = head;
			return LEPT_PARSE_MISS_QUOTATION_MARK;
		case '\\':
			switch (*p++) {
			case '\"': PUTC(c, '\"'); break;
			case '\\': PUTC(c, '\\'); break;
			case '/':  PUTC(c, '/'); break;
			case 'b':  PUTC(c, '\b'); break;
			case 'f':  PUTC(c, '\f'); break;
			case 'n':  PUTC(c, '\n'); break;
			case 'r':  PUTC(c, '\r'); break;
			case 't':  PUTC(c, '\t'); break;
			default:
				c->top = head;
				return LEPT_PARSE_INVALID_STRING_ESCAPE;
			}
			break;
		default:
			if ((unsigned char)ch < 0x20) {
				c->top = head;
				return LEPT_PARSE_INVALID_STRING_CHAR;
			}
			PUTC(c, ch);
		}
	}
}

int lept_parse_string(lept_context* c, lept_value* v) {
	int ret;
	char* str;
	size_t len;
	ret = lept_parse_string_raw(c, &str, &len);
	if (ret == LEPT_PARSE_OK)
	{
		lept_set_string(v, str, len);
	}
	return ret;
}
/*----------------------解析数组--------------------------------*/
//前向声明
int lept_parse_value(lept_context* c, lept_value* v);

int lept_parse_array(lept_context* c, lept_value* v) {
	size_t size = 0;
	EXPECT(c, '[');
	lept_parse_whitespace(c);
	if (*c->json == ']') {
		v->type = LEPT_ARRAY;
		v->u.a.size = 0;
		v->u.a.e = NULL;
		c->json++;
		return LEPT_PARSE_OK;
	}
	int ret;
	for (; ;)
	{
		lept_value e;
		lept_init(&e);
		lept_parse_whitespace(c);
		if ((ret = lept_parse_value(c, &e)) != LEPT_PARSE_OK) {
			for (int i = 0; i < size; i++)
			{
				lept_free((lept_value*)lept_context_pop(c, sizeof(lept_value)));
			}
			return ret;
		}
		//这个地方不可以先压栈后用得到的指针作为待填充的lept_value，因为再次进入lept_value堆栈可能再次分配内存，使指针失效。
		memcpy(lept_context_push(c, sizeof(lept_value)), &e, sizeof(lept_value));
		size++;
		lept_parse_whitespace(c);
		if (*c->json == ',') {
			c->json++;
		}
		else if (*c->json == ']')
		{
			c->json++;
			v->type = LEPT_ARRAY;
			v->u.a.size = size;
			size *= sizeof(lept_value);
			memcpy(v->u.a.e = (lept_value*)malloc(size), lept_context_pop(c, size), size);
			return LEPT_PARSE_OK;
		}
		else
		{
			for (int i = 0; i < size; i++)
			{
				lept_free((lept_value*)lept_context_pop(c, sizeof(lept_value)));
			}
			return LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
		}
	}
}

/*----------------------解析对象--------------------------------*/

int lept_parse_object(lept_context* c, lept_value* v) {
	int ret;
	size_t size;
	lept_member m;
	EXPECT(c, '{');
	lept_parse_whitespace(c);
	if (*(c->json) == '}') {
		v->type = LEPT_OBJECT;
		v->u.o.m = 0;//空指针也要初始化
		v->u.o.size = 0;
		c->json++;
		return LEPT_PARSE_OK;
	}
	m.k = NULL;
	size = 0;
	for (;;)
	{
		lept_init(&m.v);
		size_t len;
		char* str;
		if (*(c->json) != '"') {
			ret = LEPT_PARSE_MISS_KEY; break;
		}

		if ((ret = lept_parse_string_raw(c, &str, &m.klen)) != LEPT_PARSE_OK) {   //str没有调用free，是因为它并不是指向了内存堆
			ret = LEPT_PARSE_MISS_KEY; break;
		}

		//m.klen = len;
		//m.k = str;  虽然指向同样的内容,但是m.k的值并不是由malloc系列函数分配的，所以没法free
		memcpy(m.k = (char*)malloc(m.klen + 1), str, m.klen);
		m.k[m.klen] = '\0';

		lept_parse_whitespace(c);
		if (*(c->json) != ':') {
			ret = LEPT_PARSE_MISS_COLON;
			break;
		}
		c->json++;
		lept_parse_whitespace(c);

		if ((ret = lept_parse_value(c, &m.v)) != LEPT_PARSE_OK)
			break;
		memcpy(lept_context_push(c, sizeof(lept_member)), &m, sizeof(lept_member));
		size++;
		m.k = NULL;

		lept_parse_whitespace(c);
		if (*(c->json) == ',') {
			c->json++;
			lept_parse_whitespace(c);
		}
		else if (*(c->json) == '}') {
			c->json++;
			v->type = LEPT_OBJECT;
			v->u.o.size = size;
			size_t s = size * sizeof(lept_member);
			memcpy(v->u.o.m = (lept_member*)malloc(s), lept_context_pop(c, s), s);
			return LEPT_PARSE_OK;
		}
		else {
			ret = LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET;
			break;
		}
		lept_parse_whitespace(c);
	}

	free(m.k);
	for (int i = 0; i < size; i++)
	{
		lept_member* m = (lept_member*)lept_context_pop(c, sizeof(lept_member));
		free(m->k);
		lept_free(&m->v);
	}
	v->type = LEPT_NULL;
	return ret;
}
/*--------------------------------------------------------------*/
int lept_parse_value(lept_context* c, lept_value* v) {
	switch (*c->json) {
	case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
	case 't':  return lept_parse_literal(c, v, "true", LEPT_TRUE);
	case 'f':  return lept_parse_literal(c, v, "false", LEPT_FALSE);
	case '\"': return lept_parse_string(c, v);
	case '[':  return lept_parse_array(c, v);
	case '{':  return lept_parse_object(c, v);
	case '\0': return LEPT_PARSE_EXPECT_VALUE;
	default:   return lept_parse_number(c, v);
	}
}

int lept_parse(lept_value* v, const char* json)
{
	lept_context c;
	assert(v != NULL);
	c.json = json;
	c.stack = NULL;
	c.size = c.top = 0;
	lept_init(v);

	lept_parse_whitespace(&c);
	int ret = lept_parse_value(&c, v);
	if (ret == LEPT_PARSE_OK)
	{
		lept_parse_whitespace(&c);
		if (*c.json != '\0')
		{
			v->type = LEPT_NULL;
			ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
		}
	}
	assert(c.top == 0);    /* <- */
	free(c.stack);         /* <- */
	return ret;
}
/*--------------------------------------------------------*/

lept_type lept_get_type(const lept_value* v)
{
	return v->type;
}

int lept_get_boolean(const lept_value* v) {
	assert(v != NULL && (v->type == LEPT_TRUE || v->type == LEPT_FALSE));
	return v->type == LEPT_TRUE;
}

void lept_set_boolean(lept_value* v, int b) {
	lept_free(v);
	v->type = b ? LEPT_TRUE : LEPT_FALSE;
}

double lept_get_number(const lept_value* v) {
	assert(v != NULL && v->type == LEPT_NUMBER);
	return v->u.n;
}

void lept_set_number(lept_value* v, double n) {
	lept_free(v);
	v->u.n = n;
	v->type = LEPT_NUMBER;
}

const char* lept_get_string(const lept_value* v) {
	assert(v != NULL && v->type == LEPT_STRING);
	return v->u.s.s;
}

size_t lept_get_string_length(const lept_value* v) {
	assert(v != NULL && v->type == LEPT_STRING);
	return v->u.s.len;
}

void lept_set_string(lept_value* v, const char* s, size_t len) {
	assert(v != NULL && (s != NULL || len == 0));
	lept_free(v);
	v->u.s.s = (char *)malloc(len + 1);
	memcpy(v->u.s.s, s, len);
	v->u.s.s[len] = '\0';
	v->u.s.len = len;
	v->type = LEPT_STRING;
}

size_t lept_get_array_size(const lept_value* v) {
	assert(v != NULL&&v->type == LEPT_ARRAY);
	return v->u.a.size;
}

lept_value* lept_get_array_element(const lept_value* v, size_t index) {
	assert(v != NULL && v->type == LEPT_ARRAY);
	assert(index < v->u.a.size);
	return &v->u.a.e[index];
}

size_t lept_get_object_size(const lept_value* v) {
	assert(v != NULL && v->type == LEPT_OBJECT);
	return v->u.o.size;
}
const char* lept_get_object_key(const lept_value* v, size_t index) {
	assert(v != NULL&&v->type == LEPT_OBJECT);
	assert(index < v->u.o.size);
	return v->u.o.m[index].k;
}
size_t lept_get_object_key_length(const lept_value* v, size_t index) {
	assert(v != NULL&&v->type == LEPT_OBJECT);
	assert(index < v->u.o.size);
	return v->u.o.m[index].klen;
}
lept_value* lept_get_object_value(const lept_value* v, size_t index) {
	assert(v != NULL&&v->type == LEPT_OBJECT);
	assert(index < v->u.o.size);
	return &v->u.o.m[index].v;
}