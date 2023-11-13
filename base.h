#ifndef BASE_H
#define BASE_H
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define arrlen(arr) ((sizeof(arr) / sizeof(arr[0])))
#define MAX(x, y) ((x >= y) ? x : y)
#define MIN(x, y) ((x > y) ? y : x)
#define KB(x) (x * 1024)
#define MB(x) (((u64)x) * 1024 * 1024)
#define GB(x) (((u64)x) * 1024 * 1024 * 1024)

// It's over
void panic(char *lit);
// TYPES
typedef uint8_t u8;
typedef uint64_t u64;
typedef int64_t i64;
typedef int32_t i32;
// MEMORY
typedef struct Arena {
	void *buf;
	u64 used;
	u64 cap;
} Arena;

typedef struct ArenaTmp {
	u64 pos;
	Arena *arena;
} ArenaTmp;

ArenaTmp arena_temp(Arena *a);
void arena_temp_reset(ArenaTmp tmp);

Arena *arena_init(u64 cap);
void arena_reset(Arena *a);
void arena_free(Arena **a);
void *arena_alloc(Arena *a, u64 size);
void *arena_alloc_zero(Arena *a, u64 size);

// STRINGS
typedef struct String {
	char *buf;
	size_t len;
} String;

bool is_alpha(char c);
bool is_num(char c);
bool is_blank(char c);
String string(char *buf, size_t len);
String str_dup(Arena *a, String s);
String str_slice(String s, u64 begin, u64 end);
String str_concat(Arena *a, String s1, String s2);
String str_concatv(Arena *a, int count, ...);
String str_fmt(Arena *a, char *fmt, ...);
i32 str_cmp(String a, String b);
bool str_eq(String a, String b);
i64 str_atol(String s);
void str_print(String s);
String str_read_file(Arena *a, char *filename);
#define str(lit) string(lit, sizeof(lit) - 1)
#define str_c(s) string(s, strlen(s))

#endif
