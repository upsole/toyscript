#include "base.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdarg.h>

#define ALIGNMENT 8
#define PAGE_SIZE KB(4)
#define DECOMMIT_MIN MB(32)
// HEADERS
priv void *reserve_virtual_memory(u64 size);
priv void commit_memory(void *block, u64 size);
priv int free_virtual_memory(void *ptr, size_t size);
// ~ARENA

Arena	*arena(u64 cap)
{
	// Align
	u64	commit_min = PAGE_SIZE;
	cap += commit_min - 1;
	cap -= cap % commit_min;
	// Reserve
	void *block = reserve_virtual_memory(cap);
	// Initial Commit
	u64 init_commit = PAGE_SIZE;
	if (init_commit < sizeof(Arena)) exit(1);
	commit_memory(block, init_commit);

	Arena *a = block;
	a->used += sizeof(Arena);
	a->commited = init_commit;
	a->cap = cap;
	return a;
}

void *arena_alloc(Arena *a, u64 size)
{
	if (NEVER(((a->used + size) > a->cap))) {
		dprintf(2, "!PANIC: Arena %p ran out of space\n", a);
		arena_stats(a, __FILE__, __LINE__);
		printf("Tried to alloc %lu bytes.", size);
		exit(1);
	}
	// Align
	u64 align_pos = (a->used + (ALIGNMENT - 1));
	align_pos -= align_pos % ALIGNMENT;
	u64 padding = align_pos - a->used;

	u8	*block = (u8 *)a;
	void *res = block + a->used + padding;
	a->used += size + padding;

	// Commit more memory if needed
	if (a->commited < a->used) {
		u64 commit = a->used - a->commited;
		commit += PAGE_SIZE - 1;
		commit -= commit % PAGE_SIZE;
		commit_memory(block + a->commited, commit);
		a->commited += commit;
	}

	return (res);
}

void	*arena_alloc_zero(Arena *a, u64 size)
{
	void	*res = arena_alloc(a, size);
	memset(res, 0, size);
	return res;
}

void arena_reset(Arena *a)
{
	arena_pop_to(a, sizeof(Arena));
}

void arena_free(Arena **a)
{
	free_virtual_memory((*a), (*a)->cap);
	(*a) = NULL;
}

void arena_pop_to(Arena *a, u64 pos)
{
	u64 min = sizeof(Arena);
	u64 new_pos = (pos > min) ? pos : min;
	a->used = new_pos;

	u64 commit_aligned_pos = a->used + PAGE_SIZE;
	commit_aligned_pos -= commit_aligned_pos % PAGE_SIZE;

	if (commit_aligned_pos + DECOMMIT_MIN <= a->commited) {
		u8 *block = (u8 *) a;
		u64 decommit_size = a->commited - commit_aligned_pos;
		free_virtual_memory(block + a->used, decommit_size);
		a->commited -= decommit_size;
	}
}

priv u64 represent_as_kb(u64 bytes)
{
	return (bytes >= 1024) ? (bytes / 1024) : bytes;
}

void	arena_stats(Arena *a, char *file, i32 line)
{
	printf("--- on %s %d\n", file, line);
	printf("Arena %p:\n", a);
	char *byte_indicator = (a->used < 1024) ? "(bytes)" : "";
	printf("%lu%s/%lu/%lu KB used.\n", 
			represent_as_kb(a->used), byte_indicator,
			a->commited / 1024, a->cap / 1024);
	printf("-----\n");
}

priv void *reserve_virtual_memory(u64 size)
{
	void *buf = mmap(0, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (NEVER(!buf)) {
		dprintf(2, "!PANIC: Could not reserve memory\n"), exit(1);
	} 
	return buf;
}

priv void commit_memory(void *block, u64 size)
{
	mprotect(block, size, PROT_READ | PROT_WRITE);
}

priv int free_virtual_memory(void *ptr, size_t size)
{
    return munmap(ptr, size);
}


// ~STRINGS
String	str_dup(Arena *a, String s)
{
	String res = {0};
	res.buf = arena_alloc(a, s.len);
	res.len = s.len;
	memcpy(res.buf, s.buf, s.len);
	return res;
}

void str_print(String s)
{
	for (u64 i = 0; i < s.len; i++) 
		write(1, &s.buf[i], 1);
}

String str_slice(String s, u64 begin, u64 end)
{
	begin = MIN(begin, s.len);
	end = MIN(end, s.len);
	if (end <= begin) return str("");
	return (String){.buf = (s.buf + begin), .len = end - begin};
}

priv i32 str_cmp(String a, String b)
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

bool str_eq(String a, String b)
{
	return (!str_cmp(a, b));
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

String str_concat_n(Arena *a, int count, ...)
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

priv String str_fv(Arena *a, char *fmt, va_list src)
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

char	*str_dupc(Arena *a, String s)
{
	char	*cstring = arena_alloc(a, s.len + 1);
	memcpy(cstring, s.buf, s.len);
	cstring[s.len] = 0;
	return cstring;
}

priv bool is_num(char c)
{
	return (('0' <= c) && (c <= '9'));
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

StrList	*strlist(Arena *a)
{
	StrList	*list = arena_alloc(a, sizeof(StrList));
	list->arena = a;
	list->head = NULL;
	list->len = 0;
	return list;
}

void strpush(StrList *l, String s)
{
	StrNode	*node = arena_alloc(l->arena, sizeof(StrNode));
	node->string = s;
	node->next = NULL;

	if (ALWAYS(l)); // XXX extend

	if (!l->head) {
		l->head = node;
		l->tail = node;
		l->len++;
		return ;
	}
	l->tail->next = node;
	l->tail = node;
	l->len++;
}

String str_read_file(Arena *a, char *filename)
{
	String s = {0};
	FILE *f = fopen(filename, "r");
	if (!f) 
		dprintf(2, "!PANIC: File %s not found.\n", filename), exit(1);
	fseek(f, 0, SEEK_END);
	u64 len = ftell(f);
	fseek(f, 0, SEEK_SET);
	s.buf = arena_alloc(a, len);
	s.len = len;
	fread(s.buf, sizeof(u8), len, f);
	fclose(f);
	return s;
}
