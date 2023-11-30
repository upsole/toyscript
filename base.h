#ifndef BASE_H
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#define BASE_H

# define arrlen(arr) ((sizeof(arr) / sizeof(arr[0])))
# define ALWAYS(x) ((assert(x)), 1)
# define NEVER(x) (assert(!(x)), 0)

# define MAX(x, y) ((x >= y) ? x : y)
# define MIN(x, y) ((x > y) ? y : x)
# define KB(x) (x * 1024)
# define MB(x) (((u64)x) * 1024 * 1024)
# define GB(x) (((u64)x) * 1024 * 1024 * 1024)

# define global static
# define priv static

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int64_t i64;
typedef int32_t i32;

typedef struct Arena {
	void *buf;
	u64 used;
	u64 commited;
	u64 cap;
} Arena;

typedef struct ArenaTmp {
	u64 pos;
	Arena *arena;
} ArenaTmp;

typedef struct String {
	char	*buf;
	u64		len;
}	String;

typedef struct StrNode StrNode;
struct StrNode {
	String	string;
	StrNode *next;
};
typedef struct StrList {
	Arena	*arena;
	StrNode	*head;
	StrNode *tail;
	u64		len;
} StrList;

// BASE API

#define str(lit) ((String) { lit, (sizeof(lit) - 1) })
#define cstr(s) ((String) { s, (strlen(s)) })
#define _NUMARGS(...) (sizeof((String[]){__VA_ARGS__}) / sizeof(String))
#define CONCAT(a, ...) (str_concat_n(a, _NUMARGS(__VA_ARGS__), __VA_ARGS__))

Arena	*arena(u64 cap);
void 	arena_reset(Arena *a);
void 	arena_pop_to(Arena *a, u64 pos);
void 	arena_free(Arena **a);
void 	*arena_alloc(Arena *a, u64 size);
void 	*arena_alloc_zero(Arena *a, u64 size);
void	arena_stats(Arena *a, char *file, i32 line);

void 	str_print(String s);
String	str_dup(Arena *a, String s);
String	str_slice(String s, u64 begin, u64 end);
bool 	str_eq(String a, String b);
String	str_fmt(Arena *a, char *fmt, ...);
String 	str_concat(Arena *a, String s1, String s2);
String 	str_concat_n(Arena *a, int count, ...);
char	*str_dupc(Arena *a, String s);
i64 	str_atol(String s);

StrList	*strlist(Arena *a);
void	strpush(StrList *l, String s);

String str_read_file(Arena *a, char *filename);
#endif
