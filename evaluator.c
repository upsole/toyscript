#include "base.h"
#include "toyscript.h"

#define IMMUTABLE 0
#define MUTABLE 1
priv Namespace *ns_inner(Arena *a, Namespace *parent, u64 cap); 
priv Bind *ns_get(Namespace *ns, String key);
priv void ns_put(Namespace *ns, String key, Element elem, bool is_mutable); 

priv ElemList *elemlist(Arena *a);
priv void elempush(ElemList *lst, Element el);

priv Element error(String msg);
priv Element *elem_alloc(Arena *a, Element elem);
// TODO Want to split arenas - One is flushed (eval) and one persists (namespace)
priv Element eval_program(Arena *a, Namespace *ns, AST *node);
priv Element eval_prefix_expression(Arena *a, String op, Element right);
priv Element eval_infix_expression(Arena *a, Element left, String op, Element right);
priv Element eval_identifier(Arena *a, Namespace *ns, String name);
priv ElemList *eval_list(Arena *a, Namespace *ns, ASTList *lst);
priv Element eval_index_expression(Arena *a, Namespace *ns, Element left, Element index);
Element	eval(Arena *a, Namespace *ns, AST *node)
{
	switch (node->type) {
		case AST_PROGRAM:
			return eval_program(a, ns, node);
		// STATEMENTS
		case AST_VAL: {
			Element value = eval(a, ns, node->AST_VAL.value);
			if (value.type == ERR) return value;
			ns_put(ns, node->AST_VAL.name, value, IMMUTABLE);
			return value;
		} break;
		case AST_VAR: {
			Element value = eval(a, ns, node->AST_VAR.value);
			if (value.type == ERR) return value;
			ns_put(ns, node->AST_VAR.name, value, MUTABLE);
			return value;
		} break;
		case AST_RETURN: {
			Element value = eval(a, ns, node->AST_RETURN.value);
			if (value.type == ERR) return value;
			return (Element) { RETURN, .RETURN = { elem_alloc(a, value) }};
		} break;
		// EXPRESSIONS
		case AST_IDENT:
			return eval_identifier(a, ns, node->AST_STR);
		case AST_LIST: {
			ElemList *lst = eval_list(a, ns, node->AST_LIST);
			if (lst->len == 1 && lst->head->element.type == ERR)
				return lst->head->element;
			return (Element) { LIST, .LIST = lst };
		} break;
		case AST_INDEX: {
			Element left = eval(a, ns, node->AST_INDEX.left);	
			if (left.type == ERR) return left;
			Element index = eval(a, ns, node->AST_INDEX.index);
			if (index.type == ERR) return index;
			return eval_index_expression(a, ns, left, index);
		}
		case AST_PREFIX: {
			Element right = eval(a, ns, node->AST_PREFIX.right);
			if (right.type == ERR) return right;
			return eval_prefix_expression(a, node->AST_PREFIX.op, right);
		} break;
		case AST_INFIX: {
			Element left = eval(a, ns, node->AST_INFIX.left);
			if (left.type == ERR) return left;
			Element right = eval(a, ns, node->AST_INFIX.right);
			if (right.type == ERR) return right;
			return eval_infix_expression(a, left, node->AST_INFIX.op, right);
		}
		// LITERALS
		case AST_INT:
			return (Element) { INT, .INT =  node->AST_INT.value  };
		case AST_BOOL:
			return (Element) { BOOL, .BOOL = node->AST_BOOL.value  };
		case AST_STR:
			return (Element) { STR, .STR = node->AST_STR };
		default:
			return (Element) { ELE_NULL };
	}
	return (Element) { ELE_NULL };
}

priv Element eval_program(Arena *a, Namespace *ns, AST *node)
{
	Element	res = {0};
	for (ASTNode *tmp = node->AST_LIST->head; tmp; tmp = tmp->next) {
		res = eval(a, ns, tmp->ast);
		if (res.type == RETURN) {
			if (NEVER(!res.RETURN.value))
				return (Element) { ELE_NULL };
			return (*res.RETURN.value);
		} 
		if (res.type == ERR) {
			return res;
		} 
	}
	return res;
}

priv Element eval_identifier(Arena *a, Namespace *ns, String name)
{
	Bind *res = ns_get(ns, name);
	if (res && ALWAYS(res->element)) return (*res->element);
	// TODO resolve builtins
	return error(CONCAT(a, str("Name not found: "), name));
}

priv ElemList *elemlist_single(Arena *a, Element el);
priv ElemList *eval_list(Arena *a, Namespace *ns, ASTList *lst)
{
	u64	previous_offset = a->used;
	ElemList *res = elemlist(a);
	Element tmp = {0};
	for (ASTNode *cursor = lst->head; cursor; cursor = cursor->next) {
		tmp = eval(a, ns, cursor->ast);
		if (tmp.type == ERR) {
			arena_pop_to(a, previous_offset);
			return elemlist_single(a, tmp);
		}
		elempush(res, tmp);
	}
	return res;
}

priv Element eval_list_index(Arena *a, Element left, Element index);
priv Element eval_index_expression(Arena *a, Namespace *ns, Element left, Element index)
{
	if (left.type == LIST && index.type == INT)
		return eval_list_index(a, left, index);
	return error(CONCAT(a, 
				str("No index operation implemented for: "),
				type_str(left.type)));
}
priv Element eval_list_index(Arena *a, Element left, Element index)
{
	i64	id = index.INT;
	if (id < 0 || id >= left.LIST->len)
		return (Element) { ELE_NULL };
	ElemNode *tmp = left.LIST->head;
	for (int i = 0; i < id; i++)
		tmp = tmp->next;
	return tmp->element;
}

priv Element eval_bang(Arena *a, Element right);
priv Element eval_minus(Arena *a, Element right);
priv Element eval_prefix_expression(Arena *a, String operator, Element right)
{
	if (str_eq(operator, str("!")))
		return eval_bang(a, right);
	if (str_eq(operator, str("-")))
		return eval_minus(a, right);
	return error(CONCAT(a, str("Invalid operation: "), operator));
}

priv Element eval_minus(Arena *a, Element right)
{
	if (right.type != INT) 
		return error(CONCAT(a, str("Invalid operation: -"), type_str(right.type)));

	return (Element) { INT, .INT = -right.INT };
}

priv Element eval_bang(Arena *a, Element right)
{
	switch (right.type) {
		case BOOL:
			return (Element) { BOOL, .BOOL = !right.BOOL };
		case INT:
			return (Element) { BOOL, .BOOL = (right.INT == 0) };
		case ELE_NULL:
			return (Element) { BOOL, .BOOL = false };
		default:
			return error(CONCAT(a, str("Invalid operation: !"), type_str(right.type)));
	}
}

priv Element eval_infix_int(Arena *a, i64 left, String op, i64 right);
priv Element eval_infix_str(Arena *a, String left, String op, String right);
priv Element eval_infix_expression(Arena *a, Element left, String op, Element right)
{
	if (left.type != right.type) 
		return error(CONCAT(a, str("Invalid types in operation: "),
					type_str(left.type), op, type_str(right.type)));

	if (left.type == INT && right.type == INT)
		return eval_infix_int(a, left.INT, op, right.INT);
	if (left.type == STR && right.type == STR)
		return eval_infix_str(a, left.STR, op, right.STR);

	if (left.type == BOOL && right.type == BOOL) {
		if (str_eq(op, str("=="))) 
			return (Element) { BOOL, .BOOL = (left.BOOL == right.BOOL) };
		if (str_eq(op, str("!="))) 
			return (Element) { BOOL, .BOOL = (left.BOOL != right.BOOL) };
	}
	return error(CONCAT(a, str("Invalid operation: "),
				type_str(left.type), op, type_str(right.type)));
}

priv Element eval_infix_int(Arena *a, i64 left, String op, i64 right)
{ // XXX Could handle overflows here?
	if (str_eq(op, str("+"))) 
		return (Element) { INT, .INT = (left + right) };
	if (str_eq(op, str("-")))
		return (Element) { INT, .INT = (left - right) };
	if (str_eq(op, str("*"))) 
		return (Element) { INT, .INT = (left * right) };
	if (str_eq(op, str("/")))
		return (Element) { INT, .INT = (left / right) };
	if (str_eq(op, str("%")))
		return (Element) { INT, .INT = (left % right) };
	if (str_eq(op, str("==")))
		return (Element) { BOOL, .BOOL = (left == right) };
	if (str_eq(op, str("!=")))
		return (Element) { BOOL, .BOOL = (left != right) };
	if (str_eq(op, str(">")))
		return (Element) { BOOL, .BOOL = (left > right) };
	if (str_eq(op, str("<")))
		return (Element) { BOOL, .BOOL = (left < right) };
	return error(CONCAT(a, str("Invalid operation: "),
				type_str(INT), op, type_str(INT)));
}

priv Element eval_infix_str(Arena *a, String left, String op, String right)
{
	if (!str_eq(op, str("+"))) 
		return error(CONCAT(a, str("Invalid operation: "),
					type_str(STR), op, type_str(STR)));
	return (Element) { STR, .STR = str_concat(a, left, right) };
}

priv Element error(String msg)
{
	return (Element) { ERR, .ERR = msg };
}

// HELPER
priv Element *elem_alloc(Arena *a, Element elem)
{
	Element	*ptr = arena_alloc(a, sizeof(Element));
	*ptr = elem;
	return ptr;
}

// STDOUT
priv String list_to_string(Arena *a, ElemList *lst);
String	to_string(Arena *a, Element e)
{
	switch (e.type) {
		case ELE_NULL:
			return str("null");
		case INT:
			return str_fmt(a, "%ld", e.INT);
		case BOOL:
			return (e.BOOL) ? str("true") : str("false");
		case STR:
			return e.STR;
		case LIST:
			return list_to_string(a, e.LIST);
		case ERR:
			return e.ERR;
		default:
			return str("Unknown");
	}
}

priv String list_to_string(Arena *a, ElemList *lst)
{
	if (!lst) return str("");
	char *buf = arena_alloc(a, 1);
	buf[0] = '[';
	u64 len = 1;
	for (ElemNode *cursor = lst->head; cursor; cursor = cursor->next) {
		String tmp = to_string(a, cursor->element);
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
	return (String){ buf, len };
}

String	type_str(ElementType type)
{
	String strings[] = { 
		str("NULL"), str("ERR"), str("INT"), 
		str("BOOL"), str("STR"), str("LIST"),
		str("RETURN"), str("FUNCTION"), str("BUILTIN")
	};
	if (NEVER(type < 0 || type >= arrlen(strings)))
		return str("Unknown type");
	return strings[type];
}

// ~ELEMLIST
priv ElemList *elemlist(Arena *a)
{
	ElemList *l = arena_alloc(a, sizeof(ElemList));
	l->arena = a;
	l->head = NULL;
	l->len = 0;
	return l;
}

priv void elempush(ElemList *l, Element el)
{
	ElemNode *node = arena_alloc(l->arena, sizeof(ElemNode));
	node->element = el;
	node->next = NULL;

	if (NEVER(!l)); // XXX extend

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

priv ElemList *elemlist_single(Arena *a, Element el)
{
	ElemList *lst = elemlist(a);
	elempush(lst, el);
	return lst;
}
// ~NAMESPACE
Namespace *ns_create(Arena *a, u64 cap)
{
	Namespace *ns = arena_alloc(a, sizeof(Namespace));
	ns->arena = a;
	ns->len = 0;
	ns->cap = cap;
	ns->values = arena_alloc_zero(a, sizeof(Bind *) * ns->cap);
	ns->parent = NULL;
	return ns;
}

priv Namespace *ns_inner(Arena *a, Namespace *parent, u64 cap)
{
	Namespace *ns = ns_create(a, cap);
	ns->parent = parent;
	return ns;
}

priv u64 hash(String key) // FNV hash
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

priv Bind *bind_alloc(Arena *a, String key, Element el, bool is_mutable)
{
	Bind *b = arena_alloc(a, sizeof(Bind));
	b->key = key;
	b->element = elem_alloc(a, el);
	b->mutable = is_mutable;
	b->next = NULL;
	return b;
}

priv void ns_put(Namespace *ns, String key, Element elem, bool is_mutable) // XXX remember if this the only check we need when implementing val / var
{
	u64 id = hash(key) % ns->cap;
	for (Bind *b = ns->values[id]; b; b = b->next) {
		if (str_eq(key, b->key)) { // if key used, update
			if (is_mutable)
				b->element = elem_alloc(ns->arena, elem);
			return;
		}
	}
	Bind *b = bind_alloc(ns->arena, key, elem, is_mutable);
	b->next = ns->values[id];
	ns->values[id] = b;
	ns->len++;
}

priv Bind *ns_get_inner(Namespace *ns, String key)
{
	u64 id = hash(key) % ns->cap;
	for (Bind *tmp = ns->values[id]; tmp; tmp = tmp->next)
		if (str_eq(key, tmp->key))
			return tmp;
	return NULL;
}

priv Bind *ns_get(Namespace *ns, String key)
{
	Bind *res = ns_get_inner(ns, key);
	if (!res && ns->parent) res = ns_get(ns->parent, key);
	return res;
}
