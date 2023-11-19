#include "eval.h"
#define NULL_ELEM ((Element){.type = ELE_NULL, .string = &stringer_null})

static String stringer_int(Arena *a, Element el)
{
	return str_fmt(a, "%ld", el._int.value);
}

static String stringer_bool(Arena *a, Element el)
{
	if (el._bool.value) return str("true");
	return str("false");
}

static String stringer_err(Arena *a, Element el)
{
	return el._error.msg;
}

static String stringer_null(Arena *a, Element el)
{
	return str("NULL");
}

static String stringer_list(Arena *a, Element el)
{
	if (el.type != ELE_LIST) return el.string(a, el);
	ElemList *list = el._list.items;
	if (!list) return (String){0};
	char *buf = arena_alloc(a, 1);
	buf[0] = '[';
	u64 len = 1;
	for (ElemNode *cursor = list->head; cursor; cursor = cursor->next) {
		String tmp = element_str(a, cursor->el);
		arena_alloc(a, tmp.len);
		memcpy(buf + len, tmp.buf, tmp.len);
		len += tmp.len;
		if (cursor->next) {
			tmp = str(", ");
			arena_alloc(a, tmp.len);
			memcpy(buf + len, tmp.buf, tmp.len);
			len += tmp.len;
		}
	}
	String tmp = str("]");
	arena_alloc(a, tmp.len);
	memcpy(buf + len, tmp.buf, tmp.len);
	len += tmp.len;
	return (String){.buf = buf, .len = len};
}

String identlist_string(Arena *a, IdentList *list);
String stmtlist_string(Arena *a, StmtList *list);
static String stringer_fn(Arena *a, Element el)
{
	return str_concatv(
	    a, 4, str("["), identlist_string(a, el._fn.params), str("]->"),
	    stmtlist_string(
		a, el._fn.body)); // FIXME this somehow writes to stdin  in repl
}

static String stringer_default(Arena *a, Element el)
{
	switch (el.type) {
		case ELE_ERR:
			return stringer_err(a, el);
		case ELE_INT:
			return stringer_int(a, el);
		case ELE_STR:
			return el._string.value;
		case ELE_LIST:
			return stringer_list(a, el);
		case ELE_BOOL:
			return stringer_bool(a, el);
		case ELE_NULL:
			return stringer_null(a, el);
		case ELE_RETURN:
			return stringer_default(a, *el._return.value);
		case ELE_FUNCTION:
			return stringer_fn(a, el);
		default:
			return str_fmt(a, "%p", &el);
	}
}

static Element error(String msg);
Element *alloc_element(Arena *a, Element el);

// Builtins

Element builtin_print(Arena *a, void *ptr)
{
	ElemList *args = ptr;
	for (ElemNode *cursor = args->head; cursor; cursor = cursor->next) {
		element_print(a, cursor->el);
	}
	str_print(str("\n"));
	return NULL_ELEM;
}

Element builtin_len(Arena *a, void *ptr)
{
	ElemList *args = ptr;
	if (args->len != 1) {
		return error(
		    str_fmt(a, "Wrong number of args for len (%lu), expected 1",
			    args->len));
	}
	Element arg = args->head->el;
	if (arg.type == ELE_STR)
		return (Element){.type = ELE_INT,
				 ._int.value = arg._string.value.len};
	if (arg.type == ELE_LIST)
		return (Element){.type = ELE_INT,
				 ._int.value = arg._list.items->len};
	return error(
	    str_concat(a, str("Type error: len called with arg of type: "),
		       type_str(arg.type)));
}

ElemList *elemlist(Arena *a);
void elem_push(ElemList *l, Element el);
ElemList *elemlist_single(Arena *a, Element el);
Element builtin_push(Arena *a, void *ptr)
{
	ElemList *args = ptr;
	if (args->len != 2) {
		return error(str_fmt(a,
				     "Wrong number of args for len (%lu), "
				     "expected 2 (list and new element)",
				     args->len));
	}
	Element arg_list = args->head->el;
	if (arg_list.type != ELE_LIST)
		return error(str("First argument must be of type list"));
	Element arg_elem = args->head->next->el;
	elem_push(arg_list._list.items, arg_elem);
	return (Element){.type = ELE_LIST, ._list.items = arg_list._list.items};
}

Element builtin_car(Arena *a, void *ptr)
{
	ElemList *args = ptr;
	if (args->len != 1) {
		return error(str_fmt(
		    a, "Wrong number of args for len (%lu), expected 2 (list)",
		    args->len));
	}
	Element arg_list = args->head->el;
	if (arg_list.type != ELE_LIST)
		return error(str("First argument must be of type list"));
	return arg_list._list.items->head->el;
}

ElemNode *last_elem(ElemList *l);
Element builtin_cdr(Arena *a, void *ptr)
{
	ElemList *args = ptr;
	if (args->len != 1) {
		return error(str_fmt(
		    a, "Wrong number of args for len (%lu), expected 2 (list)",
		    args->len));
	}
	Element arg_list = args->head->el;
	if (arg_list.type != ELE_LIST)
		return error(str("First argument must be of type list"));
	ElemNode *cdr = arg_list._list.items->head;
	if (!cdr) return NULL_ELEM;
	cdr = cdr->next;
	if (!cdr) return NULL_ELEM;
	ElemList *l = arena_alloc(a, sizeof(ElemList)); // XXX rethink this
	l->len = (arg_list._list.items->len == 0)
		     ? 0
		     : arg_list._list.items->len - 1;
	l->head = cdr;
	return (Element){.type = ELE_LIST, ._list.items = l};
}

Element builtin_cons(Arena *a, void *ptr)
{
	ElemList *args = ptr;
	if (args->len != 2) {
		return error(str_fmt(a,
				     "Wrong number of args for len (%lu), "
				     "expected 2 (list, elem)",
				     args->len));
	}
	Element first = args->head->el;
	if (first.type != ELE_LIST) // XXX good place to get tuples
		return error(str("First argument must be of type list"));
	Element second = args->head->next->el;
	if (second.type == ELE_NULL) return first;
	if (second.type == ELE_LIST) {
		last_elem(first._list.items)->next = second._list.items->head;
		first._list.items->len += second._list.items->len;
		return first;
	}
	elem_push(first._list.items, second); // XXX see above; implement tuples
	return (Element){.type = ELE_LIST, ._list.items = first._list.items};
}

Element builtin_last(Arena *a, void *ptr)
{
	ElemList *args = ptr;
	if (args->len != 1) {
		return error(str_fmt(
		    a, "Wrong number of args for len (%lu), expected 1 (list)",
		    args->len));
	}
	Element list = args->head->el;
	if (list.type != ELE_LIST)
		return error(str("First argument must be of type list"));
	ElemNode *last = last_elem(list._list.items);
	if (!last) return NULL_ELEM;
	return last->el;
}

Element builtins(String fn_name)
{
	if (str_eq(fn_name, str("len")))
		return (Element){.type = ELE_BUILTIN,
				 ._builtin.fn = &builtin_len};
	if (str_eq(fn_name, str("print")))
		return (Element){.type = ELE_BUILTIN,
				 ._builtin.fn = &builtin_print};
	if (str_eq(fn_name, str("push")))
		return (Element){.type = ELE_BUILTIN,
				 ._builtin.fn = &builtin_push};
	if (str_eq(fn_name, str("car")))
		return (Element){.type = ELE_BUILTIN,
				 ._builtin.fn = &builtin_car};
	if (str_eq(fn_name, str("cdr")))
		return (Element){.type = ELE_BUILTIN,
				 ._builtin.fn = &builtin_cdr};
	if (str_eq(fn_name, str("last")))
		return (Element){.type = ELE_BUILTIN,
				 ._builtin.fn = &builtin_last};
	if (str_eq(fn_name, str("cons")))
		return (Element){.type = ELE_BUILTIN,
				 ._builtin.fn = &builtin_cons};
	return error(str("No builtin function found"));
}

Element eval_identifier(Environment *env, Expression identifier);
Element eval_statements(Environment *env, StmtList *list);
Element eval_program(Environment *env, Program prog);
Element eval_expression(Environment *env, Expression ex);
Element eval_prefix_expression(Arena *a, String operator, Element right);
Element eval_infix_expression(Arena *a, String operator, Element left,
			      Element right);
Element eval_index_expression(Arena *a, Element left, Element index);
Element eval_cond_expression(Environment *env, struct ConditionalExp cond);
Element eval_call_function(Environment *env, Element function, ElemList *args);
ElemList *eval_expressions(Environment *env, ExpList *exps);
Element eval(Environment *env, Statement s)
{
	switch (s.type) {
		case PROGRAM:
			return eval_program(env, s._program);
		case STMT_VAL: {
			Element val = eval_expression(env, s._val.value);
			if (val.type == ELE_ERR) return val;
			ns_put(env->namespace, s._val.name.value, val);
			return val;
		}
		case STMT_RET: {
			Element val = eval_expression(env, s._return.value);
			if (val.type == ELE_ERR) return val;
			return (Element){.type = ELE_RETURN,
					 ._return.value =
					     alloc_element(env->arena, val)};
		}
		case STMT_EXP:
			return eval_expression(env, s._exp);
		default:
			return (Element){0};
	}
}

Element eval_expression(Environment *env, Expression ex)
{
	switch (ex.type) {
		case EXP_IDENT:
			return eval_identifier(env, ex);
		case EXP_INT:
			return (Element){.type = ELE_INT,
					 ._int.value = ex._int.value,
					 .string = &stringer_int};
		case EXP_STR:
			return (Element){.type = ELE_STR,
					 ._string.value = ex._string.value};
		case EXP_BOOL:
			return (Element){.type = ELE_BOOL,
					 ._bool.value = ex._bool.value,
					 .string = &stringer_bool};
		case EXP_LIST: {
			ElemList *items = eval_expressions(env, ex._list.items);
			if (items->len == 1 &&
			    items->head->el.type == ELE_ERR) {
				return items->head->el;
			}
			return (Element){.type = ELE_LIST,
					 ._list.items = items};
		}
		case EXP_INDEX: {
			Element left = eval_expression(env, *ex._index.left);
			if (left.type == ELE_ERR) return left;
			Element index = eval_expression(env, *ex._index.index);
			if (index.type == ELE_ERR) return index;
			return eval_index_expression(env->arena, left, index);
		}
		case EXP_PREFIX: {
			Element right = eval_expression(env, *ex._prefix.right);
			if (right.type == ELE_ERR) return right;
			return eval_prefix_expression(
			    env->arena, ex._prefix.operator, right);
		}
		case EXP_INFIX: {
			Element left = eval_expression(env, *ex._infix.left);
			if (left.type == ELE_ERR) return left;
			Element right = eval_expression(env, *ex._infix.right);
			if (right.type == ELE_ERR) return right;
			return eval_infix_expression(
			    env->arena, ex._infix.operator, left, right);
		}
		case EXP_COND:
			return eval_cond_expression(env, ex._conditional);
		case EXP_FN: {
			IdentList *params = ex._fn.params;
			StmtList *body = ex._fn.body->statements;
			return (Element){.type = ELE_FUNCTION,
					 ._fn = {.body = body,
						 .params = params,
						 .namespace = env->namespace}};
		}
		case EXP_CALL: {
			Element fn = eval_expression(env, *ex._call.function);
			if (fn.type == ELE_ERR) return fn;
			Environment f_env = {env->arena,
					     (fn.type == ELE_FUNCTION)
						 ? fn._fn.namespace
						 : env->namespace};
			ElemList *args =
			    eval_expressions(env, ex._call.arguments);
			if (args->len == 1 && args->head->el.type == ELE_ERR) {
				return args->head->el;
			}
			return eval_call_function(&f_env, fn, args);
		}
		default:
			return (Element){0};
	}
}

Element eval_call_function(Environment *env, Element function, ElemList *args)
{
	if (function.type == ELE_FUNCTION) {
		if (function._fn.params->len != args->len) {
			return error(str_fmt(env->arena,
					     "Invalid number of params "
					     "got %lu, expected %lu",
					     args->len,
					     function._fn.params->len));
		}

		Namespace *fn_ns =
		    ns_inner(env->arena, env->namespace,
			     16); // Environment enclosed to function
		ElemNode *el_n = args->head;
		for (IdentNode *id_n = function._fn.params->head; id_n;
		     id_n = id_n->next) {
			ns_put(fn_ns, id_n->id.value, el_n->el);
			el_n = el_n->next;
		}
		Environment fn_env = {env->arena, fn_ns};

		Element res = eval_statements(&fn_env, function._fn.body);
		if (res.type == ELE_RETURN) {
			return *res._return.value;
		}
		return res;
	}
	if (function.type == ELE_BUILTIN) {
		return function._builtin.fn(env->arena, args);
	}
	return error(str("Not a function"));
}

Element eval_program(Environment *env, Program prog)
{
	Element res = {0};
	for (StmtNode *cursor = prog.statements->head; cursor;
	     cursor = cursor->next) {
		res = eval(env, cursor->s);
		if (res.type == ELE_RETURN) return *res._return.value;
		if (res.type == ELE_ERR) return res; // Stop on error
	}
	return res;
}

Element eval_statements(Environment *env, StmtList *list)
{
	if (!list) return (Element){0};
	Element res = {0};
	for (StmtNode *cursor = list->head; cursor; cursor = cursor->next) {
		res = eval(env, cursor->s);
		if (res.type == ELE_RETURN)
			return res; // Return without "unwrapping" so it
				    // does end the program even in a
				    // nested block
		if (res.type == ELE_ERR) return res;
	}
	return res;
}

ElemList *eval_expressions(Environment *env, ExpList *exps)
{
	ElemList *l = elemlist(env->arena);
	Element tmp = {0};
	for (ExpNode *cursor = exps->head; cursor; cursor = cursor->next) {
		tmp = eval_expression(env, cursor->ex);
		if (tmp.type == ELE_ERR)
			return elemlist_single(env->arena, tmp);
		elem_push(l, tmp);
	}
	return l;
}

Element eval_bang(Element right);
Element eval_minus(Arena *a, Element right);
Element eval_prefix_expression(Arena *a, String operator, Element right)
{
	if (str_eq(str("!"), operator)) {
		return eval_bang(right);
	}
	if (str_eq(str("-"), operator)) {
		return eval_minus(a, right);
	}
	return error(str_concatv(a, 3, str("Invalid operation: "), operator,
				 type_str(right.type)));
}

Element eval_minus(Arena *a, Element right)
{
	if (right.type != ELE_INT)
		return error(str_concatv(a, 2, str("Invalid operation: -"),
					 type_str(right.type)));
	return (Element){.type = ELE_INT,
			 ._int.value = -right._int.value,
			 .string = &stringer_int};
}

Element eval_bang(Element right)
{
	switch (right.type) {
		case ELE_BOOL:
			return (Element){.type = ELE_BOOL,
					 ._bool.value = !right._bool.value,
					 .string = &stringer_bool};
		case ELE_NULL:
			return (Element){.type = ELE_BOOL,
					 ._bool.value = false,
					 .string = &stringer_bool};
		case ELE_INT:
			return (Element){.type = ELE_BOOL,
					 ._bool.value = right._int.value == 0,
					 .string = &stringer_bool};
		default:
			return (Element){.type = ELE_BOOL,
					 ._bool.value = false,
					 .string = &stringer_bool};
	}
}

Element eval_infix_int(Arena *a, String operator, i64 left, i64 right);
Element eval_infix_str(Arena *a, String operator, String left, String right);
Element eval_infix_expression(Arena *a, String operator, Element left,
			      Element right)
{
	if (left.type == ELE_INT && right.type == ELE_INT) {
		return eval_infix_int(a, operator, left._int.value,
				      right._int.value);
	}
	if (left.type == ELE_STR && right.type == ELE_STR) {
		return eval_infix_str(a, operator, left._string.value,
				      right._string.value);
	}

	if (str_eq(operator, str("=="))) {
		return (Element){.type = ELE_BOOL,
				 ._bool.value =
				     (left._bool.value == right._bool.value),
				 .string = &stringer_bool};
	}
	if (str_eq(operator, str("!="))) {
		return (Element){.type = ELE_BOOL,
				 ._bool.value =
				     (left._bool.value != right._bool.value),
				 .string = &stringer_bool};
	}
	if (left.type != right.type) {
		return error(str_concatv(
		    a, 4, str("Invalid types in operation: "),
		    type_str(left.type), operator, type_str(right.type)));
	}
	return error(str_concatv(a, 4, str("Invalid operation: "),
				 type_str(left.type), operator,
				 type_str(right.type)));
}

Element eval_infix_int(Arena *a, String operator, i64 left, i64 right)
{
	Element res = {0};
	res.type = ELE_INT;
	res.string = &stringer_int;
	if (str_eq(operator, str("+")))
		return (res._int.value = left + right, res);
	if (str_eq(operator, str("-")))
		return (res._int.value = left - right, res);
	if (str_eq(operator, str("*")))
		return (res._int.value = left * right, res);
	if (str_eq(operator, str("/")))
		return (res._int.value = left / right, res);
	if (str_eq(operator, str("==")))
		return (Element){.type = ELE_BOOL,
				 ._bool.value = (left == right),
				 .string = &stringer_bool};
	if (str_eq(operator, str("!=")))
		return (Element){.type = ELE_BOOL,
				 ._bool.value = (left != right),
				 .string = &stringer_bool};
	if (str_eq(operator, str(">")))
		return (Element){.type = ELE_BOOL,
				 ._bool.value = (left > right),
				 .string = &stringer_bool};
	if (str_eq(operator, str("<")))
		return (Element){.type = ELE_BOOL,
				 ._bool.value = (left < right),
				 .string = &stringer_bool};
	return error(str_concatv(a, 4, str("Invalid operator: "),
				 type_str(ELE_INT), operator,
				 type_str(ELE_INT)));
}

Element eval_infix_str(Arena *a, String operator, String left, String right)
{
	if (!str_eq(operator, str("+")))
		return error(str_concatv(a, 4, str("Invalid operation: "),
					 type_str(ELE_STR), operator,
					 type_str(ELE_STR)));
	return (Element){.type = ELE_STR,
			 ._string.value = str_concat(a, left, right)};
}

Element eval_list_index(Arena *a, Element left, Element index);
Element eval_index_expression(Arena *a, Element left, Element index)
{
	if (left.type == ELE_LIST && index.type == ELE_INT) {
		return eval_list_index(a, left, index);
	}
	return error(str_concat(a, str("No index operation implemented for "),
				type_str(left.type)));
}

Element eval_list_index(Arena *a, Element left, Element index)
{
	i64 id = index._int.value;
	if (id < 0 || id >= left._list.items->len) {
		return NULL_ELEM;
	}
	ElemNode *cursor = left._list.items->head;
	for (int i = 0; i < id; i++) {
		cursor = cursor->next;
	}
	return cursor->el;
}

bool truthyness(Element e);
Element eval_cond_expression(Environment *env, struct ConditionalExp cond)
{
	Element condition = eval_expression(env, *cond.condition);
	if (truthyness(condition)) {
		return eval_statements(env, cond.consequence->statements);
	} else if (cond.alternative) {
		return eval_statements(env, cond.alternative->statements);
	} else {
		return NULL_ELEM;
	}
}

bool truthyness(Element e)
{
	switch (e.type) {
		case ELE_NULL:
			return false;
		case ELE_BOOL:
			return e._bool.value;
		default:
			return true;
	}
}

Element *alloc_element(Arena *a, Element el)
{
	Element *ptr = arena_alloc(a, sizeof(el));
	ptr->type = el.type;
	switch (el.type) {
		case ELE_NULL:
			return ptr;
		case ELE_INT:
			ptr->_int.value = el._int.value;
			return ptr;
		case ELE_BOOL:
			ptr->_bool.value = el._bool.value;
			return ptr;
		case ELE_LIST:
			ptr->_list.items = el._list.items;
			return ptr;
		case ELE_STR:
			ptr->_string.value = el._string.value;
			return ptr;
		case ELE_FUNCTION:
			ptr->_fn = el._fn;
		default:
			return NULL;
	}
}

// Namespace HashMap
static u64 hash(String key) // FNV hash
{
	char *buf = key.buf;
	u64 hash = 1099511628211LU;
	for (int i = 0; i < key.len; i++) {
		hash ^= (u64)(u8)*buf;
		hash *= 14695981039346656037LU;
		buf++;
	}
	return hash;
}

Namespace *ns_create(Arena *a, u64 cap)
{
	Namespace *ns = arena_alloc(a, sizeof(Namespace));
	ns->arena = a;
	ns->len = 0;
	ns->cap = cap;
	ns->values = arena_alloc_zero(a, sizeof(Pair *) * ns->cap);
	ns->global = NULL;
	return ns;
}

Namespace *ns_inner(Arena *a, Namespace *global, u64 cap)
{
	Namespace *ns = ns_create(a, cap);
	ns->global = global;
	return ns;
}

Pair *pair_alloc(Arena *a, String key, Element elem)
{
	Pair *p = arena_alloc(a, sizeof(Pair));
	p->key = key;
	p->elem = elem;
	p->next = NULL;
	return p;
}

Pair *ns_get_inner(Namespace *env, String key)
{
	u64 id = hash(key) % env->cap;
	for (Pair *cursor = env->values[id]; cursor; cursor = cursor->next) {
		if (str_eq(key, cursor->key)) {
			return cursor;
		}
	}
	return NULL;
}

Pair *ns_get(Namespace *ns, String key)
{
	Pair *res = ns_get_inner(ns, key);
	if (!res && ns->global) res = ns_get(ns->global, key);
	return res;
}

void ns_put(Namespace *env, String key, Element elem)
{
	u64 id = hash(key) % env->cap;
	for (Pair *p = env->values[id]; p; p = p->next) {
		if (str_eq(key, p->key)) { // if key used, update
			p->elem = elem;
			return;
		}
	}
	Pair *p = pair_alloc(env->arena, key, elem);
	p->next = env->values[id];
	env->values[id] = p;
	env->len++;
}

Element eval_identifier(Environment *env, Expression identifier)
{
	Pair *res = ns_get(env->namespace, identifier._ident.value);
	if (res) return (res->elem);
	Element builtin = builtins(identifier._ident.value);
	if (builtin.type == ELE_BUILTIN) return builtin;
	return error(str_concat(env->arena, str("Name not found: "),
				identifier._ident.value));
}

// STDOUT
void element_print(Arena *a, Element el)
{
	ArenaTmp tmp = arena_temp(a);
	if (!el.string) {
		str_print(stringer_default(tmp.arena, el));
		return;
	}
	str_print(el.string(tmp.arena, el));
	arena_temp_reset(tmp);
}

String element_str(Arena *a, Element el)
{
	if (!el.string) return stringer_default(a, el);
	return el.string(a, el);
}

void element_aprint(Element el)
{
	Arena *a = arena_init(KB(1));
	if (!el.string) {
		str_print(stringer_default(a, el));
		return;
	}
	str_print(el.string(a, el));
	arena_free(&a);
}

// List
ElemList *elemlist(Arena *a)
{
	ElemList *l = arena_alloc(a, sizeof(ElemList));
	l->arena = a;
	l->head = NULL;
	l->tail = NULL;
	l->len = 0;
	return l;
}

void elem_push(ElemList *l, Element el)
{
	ElemNode *node = arena_alloc(l->arena, sizeof(ElemNode));
	node->el = el;
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

ElemNode *last_elem(ElemList *l)
{
	ElemNode *cursor = l->head;
	if (!cursor) return NULL;
	while (cursor->next)
		cursor = cursor->next;
	return cursor;
}

ElemList *elemlist_single(Arena *a, Element el)
{
	ElemList *l = elemlist(a);
	elem_push(l, el);
	return (l);
}
// Helpers
static Element error(String msg)
{
	return (Element){.type = ELE_ERR, ._error.msg = msg};
}

String type_str(ElemType type)
{
	if (type == ELE_NULL) return str("NULL");
	if (type == ELE_ERR) return str("ERROR");
	if (type == ELE_INT) return str("INT");
	if (type == ELE_STR) return str("STRING");
	if (type == ELE_BOOL) return str("BOOL");
	if (type == ELE_FUNCTION) return str("FUNCTION");
	if (type == ELE_BUILTIN) return str("BUILTIN");
	if (type == ELE_LIST) return str("LIST");
	if (type == ELE_RETURN) return str("RETURN");
	return str("UNKOWN");
}
