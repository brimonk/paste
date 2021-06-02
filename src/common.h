#if !defined(COMMON_H)
#define COMMON_H

/*
 * Brian Chrzanowski
 * Thu Jan 16, 2020 16:01
 *
 * Brian's Common Module
 *
 * USAGE
 *
 * In at least one source file, do this:
 *    #define COMMON_IMPLEMENTATION
 *    #include "common.h"
 *
 * and compile regularly.
 *
 * TODO Temporary (Brian)
 * 1. determine how to just make strdup be a thing
 *
 * TODO (Brian)
 * 1. this file should replace the c standard library - avoids a gross dependency
 */

#include <stdio.h>

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <float.h>

#include <assert.h>

#define SWAP(x, y, T) do { T SWAP = x; x = y; y = SWAP; } while (0)

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define ARRSIZE(x)   (sizeof((x))/sizeof((x)[0]))

// some fun macros for variadic functions :^)
#define PP_ARG_N( \
          _1,  _2,  _3,  _4,  _5,  _6,  _7,  _8,  _9, _10, \
         _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, \
         _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
         _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, \
         _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
         _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
         _61, _62, _63, N, ...) N

#define PP_RSEQ_N()                                        \
         63, 62, 61, 60,                                   \
         59, 58, 57, 56, 55, 54, 53, 52, 51, 50,           \
         49, 48, 47, 46, 45, 44, 43, 42, 41, 40,           \
         39, 38, 37, 36, 35, 34, 33, 32, 31, 30,           \
         29, 28, 27, 26, 25, 24, 23, 22, 21, 20,           \
         19, 18, 17, 16, 15, 14, 13, 12, 11, 10,           \
          9,  8,  7,  6,  5,  4,  3,  2,  1,  0

#define PP_NARG_(...)    PP_ARG_N(__VA_ARGS__)
#define PP_NARG(...)     (sizeof(#__VA_ARGS__) - 1 ? PP_NARG_(__VA_ARGS__, PP_RSEQ_N()) : 0)

/* quick and dirty, cleaner typedefs */
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;
typedef char               s8;
typedef short              s16;
typedef int                s32;
typedef long long          s64;
typedef float              f32;
typedef double             f64;

#define BUFSMALL (256)
#define BUFLARGE (4096)
#define BUFGIANT (1 << 20 << 1)

/* c_resize : resizes the ptr should length and capacity be the same */
void c_resize(void *ptr, size_t *len, size_t *cap, size_t bytes);
// USAGE: C_RESIZE(foo->bar);
//   Make sure the corresponding len and cap variables are set to 0.
#define C_RESIZE(x) (c_resize(x,x##_len,x##_cap,sizeof(**x)))

#define MK_RESIZE_ARR(t, x) t *x; size_t x##_len, x##_cap

// NOTE (brian) the regular expression functions were stolen from Rob Pike
/* regex : function to help save some typing */
int regex(char *text, char *regexp);
/* regex_match : search for regexp anywhere in text */
int regex_match (char *regexp, char *text);
/* regex_matchhere: search for regexp at beginning of text */
int regex_matchhere(char *regexp, char *text);
/* regex_matchstar: search for c*regexp at beginning of text */
int regex_matchstar(int c, char *regexp, char *text);

/* strsplit : string split */
size_t strsplit(char **arr, size_t len, char *buf, char sep);
/* strlen_char : returns strlen, but as if 'c' were the string terminator */
size_t strlen_char(char *s, char c);
/* bstrtok : Brian's (Better) strtok */
char *bstrtok(char **str, char *delim);
/* strnullcmp : compare strings, sorting null values as "first" */
int strnullcmp(const void *a, const void *b);
/* strornull : returns the string representation of NULL if the string is */
char *strornull(char *s);

/* sys_readfile : reads an entire file into a memory buffer */
char *sys_readfile(char *path, size_t *len);

/* mkguid : puts a guid in the buffer if it's long enough */
int mkguid(char *buf, size_t len);

/* ltrim : removes whitespace on the "left" (start) of the string */
char *ltrim(char *s);

/* rtrim : removes whitespace on the "right" (end) of the string */
char *rtrim(char *s);

/* mklower : makes the string lower cased */
int mklower(char *s);

/* mkupper : makes the string lower cased */
int mkupper(char *s);

/* streq : return true if the strings are equal */
int streq(char *s, char *t);

/* strneq : return true if the strings are the same, up to the smallest string */
int strneq(char *s, char *t);

/* is_num : returns true if the string is numeric */
int is_num(char *s);

/* c_atoi : stdlib's atoi, but returns 0 if the pointer is NULL */
s32 c_atoi(char *s);

/* c_cmp_strstr : common comparator for two strings */
int c_cmp_strstr(const void *a, const void *b);

/* strdup : just replicates POSIX strdup */
// char *strdup(char *s);

/* strdup_null : duplicates the string if non-null, returns NULL otherwise */
char *strdup_null(char *s);

// strslice : returns a copy of the string starting at s + n chars, and going for at most j
char *strslice(char *s, s32 n, s32 j);

struct pcgrand_t {
	u64 state;
	u64 inc;
	s32 init;
};

// RNG FUNCTIONS
// pcg_rand : get a random num from the pcgrand state
unsigned long pcg_rand(struct pcgrand_t *rng);
// pcg_seed : seed the random structure with some junk data
void pcg_seed(struct pcgrand_t *rng, u64 initstate, u64 initseq);

/* c_fprintf : common printf logging routine, with some extra pizzaz */
int c_fprintf(char *file, int line, const char *func, int level, FILE *fp, char *fmt, ...);

enum {
	  LOG_NON
	, LOG_ERR
	, LOG_WRN
	, LOG_MSG
	, LOG_LOG
	, LOG_DBG
	, LOG_TOTAL
};

// TODO (brian): see if we can use a base macro to make this cleaner/nicer
// TODO (brian): should all of these log to stderr? Yes, if stdout is really output... (should it be?)
#define LOG(fmt, ...) (c_fprintf(__FILE__, __LINE__, __FUNCTION__, LOG_LOG, stderr, fmt, ##__VA_ARGS__)) // basic log message
#define MSG(fmt, ...) (c_fprintf(__FILE__, __LINE__, __FUNCTION__, LOG_MSG, stderr, fmt, ##__VA_ARGS__)) // basic log message
#define WRN(fmt, ...) (c_fprintf(__FILE__, __LINE__, __FUNCTION__, LOG_WRN, stderr, fmt, ##__VA_ARGS__)) // warning message
#define ERR(fmt, ...) (c_fprintf(__FILE__, __LINE__, __FUNCTION__, LOG_ERR, stderr, fmt, ##__VA_ARGS__)) // error message
#define DBG(fmt, ...) (c_fprintf(__FILE__, __LINE__, __FUNCTION__, LOG_DBG, stderr, fmt, ##__VA_ARGS__)) // verbose message

#if defined(COMMON_IMPLEMENTATION)

/* c_resize : resizes the ptr should length and capacity be the same */
void c_resize(void *ptr, size_t *len, size_t *cap, size_t bytes)
{
	void **p;

	if (*len == *cap) {
		p = (void **)ptr;

		if (*cap) {
			if (BUFLARGE < *cap) {
				*cap += BUFLARGE;
			} else {
				*cap *= 2;
			}
		} else {
			*cap = BUFSMALL;
		}

		*p = realloc(*p, bytes * *cap);

		// set the rest of the elements to zero
		memset(((u8 *)*p) + *len * bytes, 0, (*cap - *len) * bytes);
	}
}

/* ltrim : removes whitespace on the "left" (start) of the string */
char *ltrim(char *s)
{
	while (isspace(*s))
		s++;

	return s;
}

/* rtrim : removes whitespace on the "right" (end) of the string */
char *rtrim(char *s)
{
	char *e;

	for (e = s + strlen(s) - 1; isspace(*e); e--)
		*e = 0;

	return s;
}

/* strnullcmp : compare strings, sorting null values as "first" */
int strnullcmp(const void *a, const void *b)
{
	char **stra, **strb;

	stra = (char **)a;
	strb = (char **)b;

	if (*stra == NULL && *strb == NULL) {
		return 0;
	}

	if (*stra == NULL && *strb != NULL) {
		return 1;
	}

	if (*stra != NULL && *strb == NULL) {
		return -1;
	}

	return strcmp(*stra, *strb);
}

/* strornull : returns the string representation of NULL if the string is */
char *strornull(char *s)
{
	// NOTE also returns NULL on empty string
	return (s && strlen(s) != 0) ? s : "NULL";
}

/* strcmpv : a qsort wrapper for strcmp */
int strcmpv(const void *a, const void *b)
{
	// remember, we get char **s
	return (int)strcmp(*(char **)a, *(char **)b);
}

/* regex : function to help save some typing */
int regex(char *text, char *regexp)
{
	return regex_match(regexp, text);
}

/* regex_match : search for regexp anywhere in text */
int regex_match(char *regexp, char *text)
{
	if (regexp[0] == '^')
		return regex_matchhere(regexp+1, text);
	do {    /* must look even if string is empty */
		if (regex_matchhere(regexp, text))
			return 1;
	} while (*text++ != '\0');
	return 0;
}

/* regex_matchhere: search for regexp at beginning of text */
int regex_matchhere(char *regexp, char *text)
{
	if (regexp[0] == '\0')
		return 1;
	if (regexp[1] == '*')
		return regex_matchstar(regexp[0], regexp+2, text);
	if (regexp[0] == '$' && regexp[1] == '\0')
		return *text == '\0';
	if (*text!='\0' && (regexp[0]=='.' || regexp[0]==*text))
		return regex_matchhere(regexp+1, text+1);
	return 0;
}

/* regex_matchstar: search for c*regexp at beginning of text */
int regex_matchstar(int c, char *regexp, char *text)
{
	do {    /* a * matches zero or more instances */
		if (regex_matchhere(regexp, text))
			return 1;
	} while (*text != '\0' && (*text++ == c || c == '.'));
	return 0;
}

/* strsplit : string split */
size_t strsplit(char **arr, size_t len, char *buf, char sep)
{
	size_t num, i;
	char *s;


	// first, we count how many instances of sep there are
	// then we split it into that many pieces

	for (num = 0, s = buf; *s; s++) {
		if (*s == sep) {
			num++;
		}
	}

	if (arr) { // only if we have a valid array, do we actually split the str
		memset(arr, 0, len * sizeof(*arr));
		for (i = 0, s = buf; i < len; i++) {
			if (0 < strlen_char(s, sep)) {
				arr[i] = s;

				s = strchr(s, sep);
				if (s == NULL)
					break;
			} else {
				arr[i] = s + strlen(s); // empty string, point to NULL byte
			}

			*s = 0;
			s++;
		}
	}

	return num;
}

/* strlen_char : returns strlen, but as if 'c' were the string terminator */
size_t strlen_char(char *s, char c)
{
	size_t i;

	// NOTE this will stop at NULLs

	for (i = 0; s[i]; i++)
		if (s[i] == c)
			break;

	return i;
}

/* bstrtok : Brian's (Better) strtok */
char *bstrtok(char **str, char *delim)
{
	/*
	 * strtok is super gross. let's make it better (and worse)
	 *
	 * few things to note
	 *
	 * To use this properly, pass a pointer to your buffer as well as a string
	 * you'd like to delimit your text by. When you've done that, you
	 * effectively have two return values. The NULL terminated C string
	 * explicitly returned from the function, and the **str argument will point
	 * to the next section of the buffer to parse. If str is ever NULL after a
	 * call to this, there is no new delimiting string, and you've reached the
	 * end
	 */

	char *ret, *work;

	ret = *str; /* setup our clean return value */
	work = strstr(*str, delim); /* now do the heavy lifting */

	if (work) {
		/* we ASSUME that str was originally NULL terminated */
		memset(work, 0, strlen(delim));
		work += strlen(delim);
	}

	*str = work; /* setting this will make *str NULL if a delimiter wasn't found */

	return ret;
}

/* sys_readfile : reads an entire file into a memory buffer */
char *sys_readfile(char *path, size_t *len)
{
	FILE *fp;
	s64 size;
	char *buf;

	fp = fopen(path, "rb");
	if (!fp) {
		return NULL;
	}

	// get the file's size
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if (len != NULL) {
		*len = size;
	}

	buf = malloc(size + 1);
	memset(buf, 0, size + 1);

	fread(buf, 1, size, fp);
	fclose(fp);

	return buf;
}

/* streq : return true if the strings are equal */
int streq(char *s, char *t)
{
	return strcmp(s, t) == 0;
}

/* strneq : return true if the strings are the same, up to the smallest string */
int strneq(char *s, char *t)
{
	s32 n;
	n = MIN(strlen(s), strlen(t));
	return strncmp(s, t, n) == 0;
}

/* is_num : returns true if the string is numeric */
int is_num(char *s)
{
	while (s && *s) {
		if (!isdigit(*s))
			return 0;
	}

	return 1;
}

/* c_atoi : stdlib's atoi, but returns 0 if the pointer is NULL */
s32 c_atoi(char *s)
{
	return s == NULL ? 0 : atoi(s);
}

/* mklower : makes the string lower cased */
int mklower(char *s)
{
	char *t;
	for (t = s; *t; t++) {
		*t = tolower(*t);
	}
	return 0;
}

/* mkupper : makes the string lower cased */
int mkupper(char *s)
{
	char *t;
	for (t = s; *t; t++) {
		*t = toupper(*t);
	}
	return 0;
}

/* c_cmp_strstr : common comparator for two strings */
int c_cmp_strstr(const void *a, const void *b)
{
	char *stra, *strb;

	// generic strcmp, with NULL checks

	stra = *(char **)a;
	strb = *(char **)b;

	if (stra == NULL && strb != NULL)
		return 1;

	if (stra != NULL && strb == NULL)
		return -1;

	if (stra == NULL && strb == NULL)
		return 0;

	return strcmp(stra, strb);
}

/* strdup : just replicates POSIX strdup */
#if 0
char *strdup(char *s)
{
	char *t;

	if (!s)
		return NULL;

	t = (char *)calloc(1, strlen(s));
	strcpy(t, s);

	return t;
}
#endif

/* strdup_null : duplicates the string if non-null, returns NULL otherwise */
char *strdup_null(char *s)
{
	return s ? strdup(s) : NULL;
}

// strslice : returns a copy of the string starting at s + n chars, and going for at most j
char *strslice(char *s, s32 n, s32 j)
{
	char *t;

	assert(s);

	t = calloc(1, j - n + 1);
	strncpy(t, s + n, j - n);

	return t;
}

/* c_fprintf : common printf logging routine, with some extra pizzaz */
int c_fprintf(char *file, int line, const char *func, int level, FILE *fp, char *fmt, ...)
{
	va_list args;
	int rc;
	char bigbuf[BUFLARGE];

	// NOTE (Brian): I hate how the compiler thinks const is useful.

	char *logstr[] = {
		  "   " // LOG_NON = 0 (this is a special case, to make the array work)
		, "ERR" // LOG_ERR = 1
		, "WRN" // LOG_WRN = 2
		, "MSG" // LOG_MSG = 3
		, "LOG" // LOG_MSG = 4
		, "DBG" // LOG_VER = 5
	};

	// NOTE (brian)
	// Honestly, it's kinda janky. The whole __FILE__:__LINE__ thing only works for like,
	// debugging messages
	//
	// This writes a formatted message:
	//   __FILE__:__LINE__ LEVELSTR MESSAGE

	assert(LOG_TOTAL == ARRSIZE(logstr));

	if (!(LOG_NON <= level && level <= LOG_DBG)) {
		WRN("Invalid LOG Level from %s:%d\n", file, line);
		level = LOG_NON;
	}

	rc = 0;

	if (strlen(fmt) == 0)
		return rc;

	memset(bigbuf, 0, sizeof(bigbuf));

	va_start(args, fmt); /* get the arguments from the stack */

	if (level == LOG_DBG) {
		// Format:
		//   __FUNC__:__LINE__ LEVELSTR MESSAGE
		rc += fprintf(fp, "%16s:%04d %s ", func, line, logstr[level]);
	} else {
		// Format:
		//   __LEVELSTR__ MESSAGE
		rc += fprintf(fp, "%s ", logstr[level]);
	}

	rc += vfprintf(fp, fmt, args);

	va_end(args); /* cleanup stack arguments */

	return rc;
}

struct pcgrand_t localrand = {0};

// pcg_rand : get a random num from the pcgrand state
unsigned long pcg_rand(struct pcgrand_t *rng)
{
	unsigned long old;
	unsigned int xorshift, rot;

	old = rng->state;

	/* advance internal state */
	rng->state = old * 6364136223846793005ULL + (rng->inc | 1);

	/* calculate output function (XSH RR), uses old state for max ILP */
	xorshift = ((old >> 18u) ^ old) >> 27u;
	rot = old >> 59u;

	return (xorshift >> rot) | (xorshift << ((-rot) & 31));
}

// pcg_seed : seed the random structure with some junk data
void pcg_seed(struct pcgrand_t *rng, u64 initstate, u64 initseq)
{
	if (rng->init) return;

    rng->state = 0U;
    rng->inc = (initseq << 1u) | 1u;
    pcg_rand(rng);
    rng->state += initstate;
    pcg_rand(rng);
}

#endif // COMMON_IMPLEMENTATION

#endif // COMMON_H
