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
priv Element eval_infix_expression(Arena *a, Element left, String op, Element right);
Element	eval(Arena *a, AST *node)
{
	switch (node->type) {
		case AST_PROGRAM:
			return eval_program(a, node);
		// STATEMENTS
		case AST_RETURN: {
			Element value = eval(a, node->AST_RETURN.value);
			if (value.type == ERR) return value;
			return (Element) { RETURN, .RETURN = { elem_alloc(a, value) }};
		} break;
		// EXPRESSIONS
		case AST_PREFIX: {
			Element right = eval(a, node->AST_PREFIX.right);
			if (right.type == ERR) return right;
			return eval_prefix_expression(a, node->AST_PREFIX.op, right);
		} break;
		case AST_INFIX: {
			Element left = eval(a, node->AST_INFIX.left);
			if (left.type == ERR) return left;
			Element right = eval(a, node->AST_INFIX.right);
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

priv Element eval_program(Arena *a, AST *node)
{
	Element	res = {0};
	for (ASTNode *tmp = node->AST_LIST->head; tmp; tmp = tmp->next) {
		res = eval(a, tmp->ast);
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

