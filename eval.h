#ifndef EVAL_H
#define EVAL_H
#include "toyscript.h"

typedef struct Element Element;
typedef String (*Stringer)(Arena *a, Element el);

typedef Element (*BuiltinFunction)(Arena *a, void *args);

typedef enum ElemType {
	ELE_NULL,
	ELE_ERR,
	ELE_INT,
	ELE_STR,
	ELE_BOOL,
	ELE_LIST,
	ELE_RETURN,
	ELE_FUNCTION,
	ELE_BUILTIN
} ElemType;

String type_str(ElemType type);

typedef struct Error ElemError;
typedef struct Element Element;
typedef struct ElemList ElemList;
typedef struct Namespace Namespace;
struct Element {
	ElemType type;
	union {
		struct Null {
		} _null;
		struct Error {
			String msg;
		} _error;
		struct Integer {
			i64 value;
		} _int;
		struct ElemString {
			String value;
		} _string;
		struct Boolean {
			bool value;
		} _bool;
		struct List {
			ElemList *items;
		} _list;
		struct Return {
			Element *value;
		} _return;
		struct Function {
			IdentList *params;
			StmtList *body;
			Namespace *namespace;
		} _fn;
		struct Builtin {
			BuiltinFunction fn;
		} _builtin;
	};
	Stringer string;
};

// Namespace HashMap
typedef struct Pair Pair;
struct Pair {
	String key;
	Element elem;
	Pair *next;
};

struct Namespace {
	Arena *arena;
	u64 len;
	u64 cap;
	Pair **values;
	Namespace *global;
};
Namespace *ns_create(Arena *a, u64 cap);
Namespace *ns_inner(Arena *a, Namespace *global, u64 cap);
Pair *ns_get(Namespace *env, String key);
void ns_put(Namespace *env, String key, Element elem);

typedef struct Environment {
	Arena *arena;
	Namespace *namespace;
} Environment;

void element_print(Arena *a, Element el);
void element_aprint(Element el);
Element eval(Environment *a, Statement s);

// List
typedef struct ElemNode ElemNode;
struct ElemNode {
	Element el;
	ElemNode *next;
};

struct ElemList {
	Arena *arena;
	ElemNode *head;
	ElemNode *tail;
	u64 len;
};

#endif
