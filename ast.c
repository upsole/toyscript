#include "toyscript.h"

// Lists
StmtList *stmtlist(Arena *a)
{
	StmtList *l = arena_alloc(a, sizeof(StmtList));
	l->arena = a;
	l->head = NULL;
	l->tail = NULL;
	l->len = 0;
	return l;
}

void stmt_push(StmtList *l, Statement st)
{
	StmtNode *s = arena_alloc(l->arena, sizeof(StmtNode));
	s->s = st;
	s->next = NULL;
	if (!l->head) {
		l->head = s;
		l->tail = s;
		l->len++;
		return;
	}
	l->tail->next = s;
	l->tail = s;
	l->len++;
}

IdentList *identlist(Arena *a)
{
	IdentList *l = arena_alloc(a, sizeof(IdentList));
	l->arena = a;
	l->head = NULL;
	l->tail = NULL;
	l->len = 0;
	return l;
}

void ident_push(IdentList *l, Identifier identifier)
{
	IdentNode *id = arena_alloc(l->arena, sizeof(IdentNode));
	id->id = identifier;
	id->next = NULL;
	if (!l->head) {
		l->head = id;
		l->tail = id;
		l->len++;
		return;
	}
	l->tail->next = id;
	l->tail = id;
	l->len++;
}

ExpList *explist(Arena *a)
{
	ExpList *l = arena_alloc(a, sizeof(ExpList));
	l->arena = a;
	l->head = NULL;
	l->tail = NULL;
	l->len = 0;
	return l;
}

void exp_push(ExpList *l, Expression ex)
{
	ExpNode *node = arena_alloc(l->arena, sizeof(ExpNode));
	node->ex = ex;
	node->next = NULL;
	if (!l->head) {
		l->head = node;
		l->tail = node;
		l->len++;
		return;
	}
	l->tail->next = node;
	l->tail = node;
	l->len++;
}
