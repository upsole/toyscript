#include "toyscript.h"
#include <stdarg.h>
#include <sys/mman.h>
#include <unistd.h>
// It's over
void panic(char *lit)
{
	dprintf(2, "!PANIC: %s\n", lit);
	exit(1);
}
// MEMORY
Arena *arena_init(u64 cap)
{
	void *buf = mmap(0, cap, PROT_READ | PROT_WRITE,
			 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	Arena *a = buf;
	a->cap = cap;
	a->used += sizeof(Arena);
	;
	a->buf = buf;
	return a;
}

void arena_reset(Arena *a)
{
	memset(a->buf + sizeof(Arena), 0, a->used);
	a->used = sizeof(Arena);
}

void arena_free(Arena **a)
{
	munmap((*a)->buf, (*a)->used);
	(*a) = NULL;
}

void arena_pop_to(Arena *a, u64 pos)
{
	a->used = pos;
}

void *arena_alloc(Arena *a, u64 size)
{
	void *res;
	if ((a->used + size) > a->cap) panic("Arena ran out of space");
	res = &a->buf[a->used];
	a->used += size;
	return (res);
}

void *arena_alloc_zero(Arena *a, u64 size)
{
	void *res = arena_alloc(a, size);
	memset(res, 0, size);
	return res;
}

ArenaTmp arena_temp(Arena *a)
{
	ArenaTmp tmp = {0};
	tmp.arena = a;
	tmp.pos = a->used;
	return tmp;
}

void arena_temp_reset(ArenaTmp tmp)
{
	arena_pop_to(tmp.arena, tmp.pos);
}

// STRINGS
bool is_alpha(char c)
{
	return ((('a' <= c) && (c <= 'z')) || (('A' <= c) && (c <= 'Z')) ||
		(c == '_'));
}

bool is_num(char c)
{
	return (('0' <= c) && (c <= '9'));
}

bool is_blank(char c)
{
	return (c == ' ' || c == '\n' || c == '\t' || c == '\r');
}

String string(char *buf, size_t len)
{
	return (String){.buf = buf, .len = len};
}

String str_dup(Arena *a, String s)
{
	String res = {0};
	res.buf = arena_alloc(a, s.len);
	res.len = s.len;
	memcpy(res.buf, s.buf, s.len);
	return res;
}

String str_slice(String s, u64 begin, u64 end)
{
	begin = MIN(begin, s.len);
	end = MIN(end, s.len);
	if (end <= begin) return str("");
	return (String){.buf = (s.buf + begin), .len = end - begin};
}

i32 str_cmp(String a, String b)
{
	i32 res = 0;
	u64 i = 0;
	u64 max_len = MAX(a.len, b.len);
	for (i = 0; i < max_len; i++) {
		res = a.buf[i] - b.buf[i];
		if (res) return res;
	}
	return res;
}

i64 str_atol(String s)
{
	i64 x = 0;
	u64 i = 0;
	while (is_num(s.buf[i]) && i < s.len) {
		x = x * 10 + (s.buf[i] - '0');
		i++;
	}
	return x;
}

String str_concat(Arena *a, String s1, String s2)
{
	String res = {0};
	res.len = s1.len + s2.len;
	res.buf = arena_alloc(a, res.len);
	u64 i = 0;
	memcpy(res.buf + i, s1.buf, s1.len);
	i += s1.len;
	memcpy(res.buf + i, s2.buf, s2.len);
	return res;
}

String str_concatv(Arena *a, int count, ...)
{
	va_list args;
	va_start(args, count);
	char *buf;
	u64 len = 0;
	for (int i = 0; i < count; i++) {
		String tmp = va_arg(args, String);
		if (i == 0)
			buf = arena_alloc(a, tmp.len);
		else
			arena_alloc(a, tmp.len);
		memcpy(buf + len, tmp.buf, tmp.len);
		len += tmp.len;
	}
	va_end(args);
	return (String){.buf = buf, .len = len};
}

static String str_fv(Arena *a, char *fmt, va_list src)
{
	String s;
	va_list dest;
	va_copy(dest, src);
	u64 size = vsnprintf(0, 0, fmt, src) + 1;
	s.buf = arena_alloc(a, size);
	vsnprintf((char *)s.buf, size, fmt, dest);
	s.len = size - 1;
	va_end(dest);
	return s;
}

String str_fmt(Arena *a, char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	String s = str_fv(a, fmt, args);
	va_end(args);
	return s;
}

bool str_eq(String a, String b)
{
	return (!str_cmp(a, b));
}

void str_print(String s)
{
	for (u64 i = 0; i < s.len; i++)
		write(1, &s.buf[i], 1);
}

// IO
String str_read_file(Arena *a, char *filename)
{
	String s = {0};
	FILE *f = fopen(filename, "r");
	if (!f) panic("File not found.");
	fseek(f, 0, SEEK_END);
	u64 len = ftell(f);
	fseek(f, 0, SEEK_SET);
	s.buf = arena_alloc(a, len);
	s.len = len;
	fread(s.buf, sizeof(u8), len, f);
	fclose(f);
	return s;
}
