#include "base.h"
#include "toyscript.h"

priv Element error(String msg);
Element	*elem_alloc(Arena *a, Element elem);
Element	*elem_alloc(Arena *a, Element elem)
{
	Element	*ptr = arena_alloc(a, sizeof(Element));
	*ptr = elem;
	return ptr;
}
// TODO Want to split arenas - One is flushed (eval) and one persists (namespace)
priv Element eval_program(Arena *a, AST *node);
priv Element eval_prefix_expression(Arena *a, String op, Element right);

Element	eval(Arena *a, AST *node)
{
	switch (node->type) {
		case AST_PROGRAM:
			return eval_program(a, node);
		// EXPRESSIONS
		case AST_PREFIX: {
			Element right = eval(a, node->AST_PREFIX.right);
			if (right.type == ERR) return right;
			return eval_prefix_expression(a, node->AST_PREFIX.op, right);
		} break;
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

priv Element eval_program(Arena *a, AST *node)
{
	Element	res = {0};
	for (ASTNode *tmp = node->AST_LIST->head; tmp; tmp = tmp->next) {
		res = eval(a, tmp->ast);
		if (res.type == RETURN) {
			return (*res.RETURN.value);
		} 
		if (res.type == ERR) {
			return res;
		} 
		/// XXX danger
	}
	return res;
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

priv Element error(String msg)
{
	return (Element) { ERR, .ERR = msg };
}

// stdout
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
		case ERR:
			return e.ERR;
		default:
			return str("Unknown");
	}
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

