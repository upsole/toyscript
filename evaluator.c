#include "base.h"
#include "toyscript.h"
#include <stdio.h>

#define IMMUTABLE 0
#define MUTABLE 1
priv Namespace *ns_inner(Arena *a, Namespace *parent, u64 cap); 
priv Bind *ns_get(Namespace *ns, String key);
priv int ns_put(Namespace *ns, String key, Element elem, bool is_mutable); 
priv int ns_update(Namespace *ns, String key, Element elem);
priv Namespace *ns_copy(Arena *a, Namespace *ns);

priv ElemList *elemlist(Arena *a);
priv ElemList *elemlist_copy(Arena *a, ElemList *lst);
priv void elempush(ElemList *lst, Element el);
priv ElemArray *elemarray(Arena *a, int len);
priv ElemArray *elemarray_copy(Arena *a, ElemArray *arr);

priv Element BUILTINS(String name);

priv Element error(String msg);
priv Element *elem_alloc(Arena *a, Element elem);
priv Element elem_copy(Arena *a, Element elem);
ASTList *astlist_copy(Arena *a, ASTList *lst);

priv Element eval_program(Arena *a, Namespace *ns, AST *node);
priv Element eval_prefix_expression(Arena *a, String op, Element right);
priv Element eval_infix_expression(Arena *a, Element left, String op, Element right);
priv Element eval_identifier(Arena *a, Namespace *ns, String name);
priv Element eval_mutable_identifier(Arena *a, Namespace *ns, String name);

priv Element eval_array(Arena *a, Namespace *ns, ASTList *lst) ;
priv ElemArray *elemarray_from_ast(Arena *a, Namespace *ns, ASTList *lst);
priv Element eval_list(Arena *a, Namespace *ns, ASTList *lst);
priv ElemList *elemlist_from_ast(Arena *a, Namespace *ns, ASTList *lst);

priv Element eval_assignement(Arena *a, Namespace *ns, struct AST_ASSIGN node);

priv bool is_truthy(Element e);
priv Element eval_block(Arena *a, Namespace *ns, ASTList *list);
priv Element eval_index_expression(Arena *a, Namespace *ns, Element left, Element index);
priv Element eval_cond_expression(Arena *a, Namespace *ns, AST *node);
priv Element eval_call(Arena *a, Namespace *ns, Element callee, ElemArray *args);

Element	eval(Arena *a, Namespace *ns, AST *node)
{
	if (!node) return (Element) { ERR, .ERR = str("Program has errors") };
	switch (node->type) {
		case AST_PROGRAM:
			return eval_program(a, ns, node);
		// STATEMENTS
		case AST_VAL: {
			if (NEVER(!node->AST_VAL.value))
				return (Element) { ELE_NULL };
			Element value = eval(a, ns, node->AST_VAL.value);
			if (value.type == ERR) return value;
			if(ns_put(ns, node->AST_VAL.name, value, IMMUTABLE) == -1)
				return (Element) { ERR, .ERR = str("Immutable variable already bound") };
			return value;
		} break;
		case AST_VAR: {
			if (NEVER(!node->AST_VAR.value))
				return (Element) { ELE_NULL };
			Element value = {0};
			if (node->AST_VAR.value->type == AST_LIST)
				value = eval_list(a, ns, node->AST_VAR.value->AST_LIST);
			else
				value = eval(a, ns, node->AST_VAR.value);
			if (value.type == ERR) return value;
			ns_put(ns, node->AST_VAR.name, value, MUTABLE);
			return value;
		} break;
		case AST_RETURN: {
			if (NEVER(!node->AST_RETURN.value))
				return (Element) { ELE_NULL };
			Element value = eval(a, ns, node->AST_RETURN.value);
			if (value.type == ERR) return value;
			return (Element) { RETURN, .RETURN = { elem_alloc(a, value) }};
		} break;
		case AST_ASSIGN: {
			if (NEVER(!node->AST_ASSIGN.left || !node->AST_ASSIGN.right)) 
				return (Element) { ELE_NULL };
			return eval_assignement(a, ns, node->AST_ASSIGN);
		} break;
		// EXPRESSIONS
		case AST_IDENT:
			return eval_identifier(a, ns, node->AST_STR);
		case AST_LIST: {
			return eval_array(a, ns, node->AST_LIST);
		} break;
		case AST_FN: {
			ASTList *params = node->AST_FN.params;
			ASTList *body = node->AST_FN.body;
			return (Element) { FUNCTION, .FUNCTION = { params, body, ns } };
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
		case AST_COND: 
			return eval_cond_expression(a, ns, node);
		case AST_CALL: {
			Element fn = eval(a, ns, node->AST_CALL.function);
			if (fn.type == ERR) return fn;
			Namespace *func_namespace = (fn.type == FUNCTION) ? fn.FUNCTION.namespace : ns;
			ElemArray *args = elemarray_from_ast(a, ns, node->AST_CALL.args);
			if (args->len == 1 && args->items[0].type == ERR)
				return args->items[0];
			return eval_call(a, func_namespace, fn, args);
		}
		case AST_WHILE: {
			Element condition = eval(a, ns, node->AST_WHILE.condition);
			if (condition.type == ERR) return condition;
			Arena *block_arena = arena(MB(1));
			Namespace *block_ns = ns_inner(block_arena, ns, 16);
			while (is_truthy(condition)) {
				Element block = eval_block(a, block_ns, node->AST_WHILE.body);
				if (block.type == ERR) return (arena_free(&block_arena), block);
				condition = eval(a, ns, node->AST_WHILE.condition);
				if (condition.type == ERR) return (arena_free(&block_arena), condition);
			}
			arena_free(&block_arena);
			return (Element) { ELE_NULL };
		}
		// LITERALS
		case AST_NULL:
			return (Element) { ELE_NULL };
		case AST_INT:
			return (Element) { INT, .INT =  node->AST_INT.value  };
		case AST_BOOL:
			return (Element) { BOOL, .BOOL = node->AST_BOOL.value  };
		case AST_STR:
			return (Element) { STR, .STR = node->AST_STR };
	}
	return (Element) { ELE_NULL };
}

priv Element eval_program(Arena *a, Namespace *ns, AST *node)
{
	if (NEVER(node->type != AST_PROGRAM))
		return (Element) { ELE_NULL };
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

priv Element eval_block(Arena *a, Namespace *ns, ASTList *list)
{
	Element	res = {0};
	for (ASTNode *tmp = list->head; tmp; tmp = tmp->next) {
		res = eval(a, ns, tmp->ast);
		if (res.type == RETURN) {
			if (NEVER(!res.RETURN.value))
				return (Element) { ELE_NULL };
			return res; // Return without unwrapping so program ends - this is the main difference with eval_program
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
	if (res) return res->element;
	Element builtin = BUILTINS(name);
	if (builtin.type == BUILTIN) return builtin;
	return error(CONCAT(a, str("Name not found: "), name));
}

priv Element eval_mutable_identifier(Arena *a, Namespace *ns, String name)
{
	Bind *res = ns_get(ns, name);
	if (!res)
		return error(CONCAT(a, str("Name not found: "), name));
	if (!res->mutable)
		return error(CONCAT(a, name, str(" binding is not mutable")));
	return (res->element);
}

priv ElemArray *elemarray_from_ast(Arena *a, Namespace *ns, ASTList *lst);
priv Element eval_array(Arena *a, Namespace *ns, ASTList *lst) 
{
	if(NEVER(!lst))
		return (Element) { ELE_NULL };
	ElemArray *res = elemarray_from_ast(a, ns, lst);
	if (res->len == 1 && res->items[0].type == ERR)
		return res->items[0];
	return (Element) { ARRAY, .ARRAY = res };
}

priv ElemList *elemlist_from_ast(Arena *a, Namespace *ns, ASTList *lst);
priv Element eval_list(Arena *a, Namespace *ns, ASTList *lst) 
{
	if(NEVER(!lst))
		return (Element) { ELE_NULL };
	ElemList *res = elemlist_from_ast(a, ns, lst);
	if (res->len == 1 && res->head->element.type == ERR)
		return res->head->element;
	return (Element) { LIST, .LIST = res };
}

priv Element eval_array_index(Arena *a, Element left, Element index);
priv Element eval_list_index(Arena *a, Element left, Element index);
priv Element eval_index_expression(Arena *a, Namespace *ns, Element left, Element index)
{
	if (left.type == ARRAY && index.type == INT)
		return eval_array_index(a, left, index);

	if (left.type == LIST && index.type == INT)
		return eval_list_index(a, left, index);

	return error(CONCAT(a, 
				str("No index operation implemented for: "),
				type_str(left.type)));
}

priv Element eval_array_index(Arena *a, Element left, Element index)
{
	i64	id = index.INT;
	if (id < 0 || id >= left.ARRAY->len)
		return (Element) { ELE_NULL };
	return left.ARRAY->items[id];
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

priv bool is_truthy(Element e)
{
	switch (e.type) {
		case ERR:
			return false;
		case ELE_NULL:
			return false;
		case INT:
			return (e.INT != 0);
		case BOOL:
			return e.BOOL;
		case STR:
			return (!str_eq(e.STR, str("")));
		default:
			return true;
	}
}

priv Element eval_cond_expression(Arena *a, Namespace *ns, AST *node)
{
	if (NEVER(node->type != AST_COND))
		return (Element) { ELE_NULL };
	Element condition = eval(a, ns, node->AST_COND.condition);
	if (is_truthy(condition)) {
		return eval_block(a, ns, node->AST_COND.consequence);
	} else if (node->AST_COND.alternative) {
		return eval_block(a, ns, node->AST_COND.alternative);
	} else {
		return (Element) { ELE_NULL };
	}
}

priv Element eval_function_call(Arena *a, Namespace *ns, struct FUNCTION fn, ElemArray *args);
priv Element eval_call(Arena *a, Namespace *ns, Element fn, ElemArray *args)
{
	if (fn.type == FUNCTION)
		return eval_function_call(a, ns, fn.FUNCTION, args);
	if (fn.type == BUILTIN)
		return fn.BUILTIN(a, ns, args);
	return error(CONCAT(a, str("Not a callable element: "), to_string(a, fn)));
}

priv Element eval_function_call(Arena *a, Namespace *ns, struct FUNCTION fn, ElemArray *args)
{
	if (fn.params->len != args->len) 
		return error(str_fmt(a, "Invalid number of arguments: Got %lu, expected %lu",
					args->len, fn.params->len));

	Arena	*call_arena = arena(MB(1));
	Namespace *call_ns = ns_inner(call_arena, ns, 16);

	ASTNode *params_node = fn.params->head;
	for (int i = 0; i < args->len; i++) {
		if (NEVER(!params_node))
			return (arena_free(&call_arena), (Element) { ELE_NULL });
		ns_put(call_ns, params_node->ast->AST_STR, args->items[i], MUTABLE);
		params_node = params_node->next;
	}

	Element res = eval_block(call_arena, call_ns, fn.body);
	if (res.type == RETURN) {
		Element return_value = elem_copy(a, (*res.RETURN.value));
		arena_free(&call_arena);
		return return_value;
	}
	Element *copy = elem_alloc(a, res);
	arena_free(&call_arena);
	return (*copy);
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

priv Element eval_assignement_to_index(Arena *a, Namespace *ns, struct AST_INDEX index, AST *right);
priv Element eval_assignement(Arena *a, Namespace *ns, struct AST_ASSIGN node)
{
	if (node.left->type == AST_IDENT) {
		Element left = eval_mutable_identifier(a, ns, node.left->AST_STR);
		if (left.type == ERR)
			return left;
		Element right = eval(a, ns, node.right);
		if (right.type == ERR)
			return left;
		if (right.type != left.type)
			return error(CONCAT(a, str("Can't assign type "), type_str(right.type), str(" to variable "), 
						node.left->AST_STR, str(" of type "), type_str(left.type)));
		ns_update(ns, node.left->AST_STR, right);
		return right;		
	}

	if (node.left->type == AST_INDEX) {
		return eval_assignement_to_index(a, ns, node.left->AST_INDEX, node.right);
	}
	return error(CONCAT(a, str("Can't assign to type "), type_str(node.left->type)));
}

priv Element eval_assignement_to_index(Arena *a, Namespace *ns, struct AST_INDEX index, AST *new_val_ast)
{
	Element new_val = eval(a, ns, new_val_ast);
	if (new_val.type == ERR)
		return new_val;
	Element right = eval(a, ns, index.index);
	if (index.left->type == AST_IDENT) {
		// Eval Ident
		Element left = eval(a, ns, index.left);
		if (left.type == ARRAY) {
			if (right.type != INT)
				return error(str("Index should be an INT for ARRAY indexing"));
			if (right.INT < 0 || right.INT >= left.ARRAY->len)
				return error(str_fmt(a, "Out of bounds assignment: max index is %d, attempted to access %d", 
							(left.ARRAY->len - 1), right.INT));
			left.ARRAY->items[right.INT] = new_val;
			return new_val;
		}
		if (left.type == LIST) {
			if (right.type != INT)
				return error(str("Index should be an INT for LIST indexing"));
			if (right.INT < 0 || right.INT >= left.LIST->len)
				return error(str_fmt(a, "Out of bounds assignment: max index is %d, attempted to access %d", 
							(left.LIST->len - 1), right.INT));
			ElemNode *tmp = left.LIST->head;
			for (int i = 0; i < right.INT; i++)
				tmp = tmp->next;
			tmp->element = new_val;
			return new_val;
		}
		return error(str("Not an indexable item."));
	}
	return error(str("Trying to assign to a non-bound value"));
}
priv Element builtin_concat(Arena *a, Namespace *ns, ElemArray *args);
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
	if (left.type == LIST && right.type == LIST) { // TODO convert second arg to list/array as needed
		ElemArray *lst = elemarray(a, 2);
		lst->items[0] = left; lst->items[1] = right;
		return builtin_concat(a, NULL, lst); // XXX
	}
	if (left.type == ARRAY && right.type == ARRAY) {
		ElemArray *lst = elemarray(a, 2);
		lst->items[0] = left; lst->items[1] = right;
		return builtin_concat(a, NULL, lst); // XXX
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
	if (str_eq(op, str("+")))
		return (Element) { STR, .STR = str_concat(a, left, right) };

	if (str_eq(op, str("==")))
		return (Element) { BOOL, .BOOL = str_eq(left, right) };

	if (str_eq(op, str("!=")))
		return (Element) { BOOL, .BOOL = (!str_eq(left, right)) };

	return error(CONCAT(a, str("Invalid operation: "),
				type_str(STR), op, type_str(STR)));
}

priv Element error(String msg)
{
	return (Element) { ERR, .ERR = msg };
}

// ~BUILTINs
priv Element builtin_len(Arena *a, Namespace *ns, ElemArray *args);
priv Element builtin_print(Arena *a, Namespace *ns, ElemArray *args);
priv Element builtin_type(Arena *a, Namespace *ns, ElemArray *args);
priv Element builtin_push(Arena *a, Namespace *ns, ElemArray *args);
priv Element builtin_car(Arena *a, Namespace *ns, ElemArray *args);
priv Element builtin_cdr(Arena *a, Namespace *ns, ElemArray *args);
priv Element builtin_slurp(Arena *a, Namespace *ns, ElemArray *args);
priv Element BUILTINS(String name)
{
	if (str_eq(str("print"), name))
		return (Element) { BUILTIN, .BUILTIN = &builtin_print };
	if (str_eq(str("len"), name))
		return (Element) { BUILTIN, .BUILTIN = &builtin_len };
	if (str_eq(str("type"), name))
		return (Element) { BUILTIN, .BUILTIN = &builtin_type };
	if (str_eq(str("push"), name))
		return (Element) { BUILTIN, .BUILTIN = &builtin_push };
	if (str_eq(str("car"), name))
		return (Element) { BUILTIN, .BUILTIN = &builtin_car };
	if (str_eq(str("cdr"), name))
		return (Element) { BUILTIN, .BUILTIN = &builtin_cdr };
	if (str_eq(str("concat"), name))
		return (Element) { BUILTIN, .BUILTIN = &builtin_concat };
	if (str_eq(str("slurp"), name))
		return (Element) { BUILTIN, .BUILTIN = &builtin_slurp };
	return (Element) { ELE_NULL };	
}
priv Element elemlist_concat(Arena *a, ElemList *left, ElemList *right);
priv Element elemarray_concat(Arena *a, ElemArray *left, ElemArray *right);
priv Element builtin_concat(Arena *a, Namespace *ns, ElemArray *args)
{
	if (args->len != 2)
		return error(str_fmt(a, "Wrong number of args for concat: got %lu, expected 2", args->len));
	Element arg0 = args->items[0];
	if (arg0.type == ERR)
		return arg0;
	Element arg1 = args->items[1];
	if (arg1.type == ERR)
		return arg1;
	if (arg0.type == ARRAY) {
		if (ALWAYS(arg1.type == ARRAY))
			return elemarray_concat(a, arg0.ARRAY, arg1.ARRAY);
	}
	if (arg0.type == LIST) {
		if (ALWAYS(arg1.type == LIST))
			return elemlist_concat(a, arg0.LIST, arg1.LIST);
	}
	// IF STR
	return error(CONCAT(a, str("Wrong types for concat, got "),
				type_str(arg0.type), str(" and "), type_str(arg1.type)));
}

priv Element elemlist_concat(Arena *a, ElemList *left, ElemList *right)
{
	if (!left || !left->head)
		return (Element) { LIST, .LIST = right };
	if (!right || !right->head)
		return (Element) { LIST, .LIST = left };
	ElemNode *tmp = left->head;
	while (tmp->next) tmp = tmp->next; // Find last
	tmp->next = right->head;
	left->len += right->len;
	return (Element) { LIST, .LIST = left};
}
priv Element elemarray_concat(Arena *a, ElemArray *left, ElemArray *right)
{
	ElemArray *new = elemarray(a, left->len + right->len);
	int j = 0;
	for (int i = 0; i < left->len; i++) {
		new->items[j] = left->items[i];
		j++;
	}
	for (int i = 0; i < right->len; i++) {
		new->items[j] = right->items[i];
		j++;
	}
	return (Element) { ARRAY, .ARRAY = new };
}

priv Element read_file_to_elem(Arena *a, char *filename);
priv Element builtin_slurp(Arena *a, Namespace *ns, ElemArray *args)
{
	if (args->len != 1)
		return error(str_fmt(a, "Wrong number of args for slurp: got %lu, expected 1", args->len));
	Element arg0 = args->items[0];
	if (arg0.type == ERR)
		return arg0;
	if (arg0.type != STR)
		return error(CONCAT(a, str("Called slurp with the wrong type "), type_str(arg0.type), str(" wanted STR")));
	Element file = read_file_to_elem(a, str_dupc(a, arg0.STR));
	return file;
}

priv Element read_file_to_elem(Arena *a, char *filename)
{
	String s = {0};
	FILE *f = fopen(filename, "r");
	if (!f) 
		return error(str_fmt(a, "File '%s' not found", filename));
	fseek(f, 0, SEEK_END);
	u64 len = ftell(f);
	fseek(f, 0, SEEK_SET);
	s.buf = arena_alloc(a, len);
	s.len = len;
	fread(s.buf, sizeof(u8), len, f);
	fclose(f);
	return (Element) { STR, .STR = s };
}

priv Element builtin_cdr(Arena *a, Namespace *ns, ElemArray *args)
{
	if (args->len != 1)
		return error(str_fmt(a, "Wrong number of args for car: got %lu, expected 1", args->len));
	Element arg0 = args->items[0];
	if (arg0.type == ERR)
		return arg0;

	if (arg0.type == LIST) {
		ElemNode *cdr = arg0.LIST->head;
		if (!cdr || !cdr->next)
			return (Element) { ELE_NULL };
		ElemList *lst = elemlist(a);
		lst->len = (arg0.LIST->len == 0) ? 0 : arg0.LIST->len - 1;
		lst->head = cdr->next;
		return (Element) { LIST, .LIST = lst };
	}
	if (arg0.type == ARRAY) {
		if (arg0.ARRAY->len < 2)
			return (Element) { ELE_NULL };
		ElemArray *arr = elemarray(a, arg0.ARRAY->len - 1);
		for (int i = 1; i < arg0.ARRAY->len; i++)
			arr->items[i - 1] = arg0.ARRAY->items[i];
		return (Element) { ARRAY, .ARRAY = arr };
	}
	if (arg0.type == STR) {
		if (arg0.STR.len < 1) 
			return (Element) { STR, .STR = str("") };
		return (Element) { STR, .STR = str_slice(arg0.STR, 1, arg0.STR.len) };
	}
	if (arg0.type == ELE_NULL)  
		return arg0;
	return error(CONCAT(a, str("Wrong type for car, got "),
				type_str(arg0.type)));
}

priv Element builtin_car(Arena *a, Namespace *ns, ElemArray *args)
{
	if (args->len != 1)
		return error(str_fmt(a, "Wrong number of args for car: got %lu, expected 1", args->len));
	Element arg0 = args->items[0];
	if (arg0.type == ERR)
		return arg0;

	if (arg0.type == LIST) {
		if (arg0.LIST->head)
			return arg0.LIST->head->element;
		else
			return (Element) { ELE_NULL };
	}
	if (arg0.type == ARRAY) {
		if (arg0.ARRAY->len < 1)
			return (Element) { ELE_NULL };
		return arg0.ARRAY->items[0];
	}
	if (arg0.type == STR) {
		if (arg0.STR.len < 1) 
			return (Element) { STR, .STR = str("") };
		return (Element) { STR, .STR = str_slice(arg0.STR, 0, 1) };
	}
	if (arg0.type == ELE_NULL)  
		return arg0;
	return error(CONCAT(a, str("Wrong type for car, got "),
				type_str(arg0.type)));
}

priv Element builtin_push(Arena *a, Namespace *ns, ElemArray *args)
{
	if (args->len != 2)
		return error(str_fmt(a, "Wrong number of args for push: got %lu, expected 2", args->len));
	Element arg0 = args->items[0];
	if (arg0.type == ERR)
		return arg0;
	Element arg1 = args->items[1];
	if (arg1.type == ERR)
		return arg1;
	if (arg0.type == LIST) {
		elempush(arg0.LIST, arg1);
		return arg0;
	}
	if (arg0.type == ARRAY) {
		int a0_len = arg0.ARRAY->len;
		ElemArray *res = elemarray(a, (a0_len + 1));
		for (int i = 0; i < a0_len; i++)
			res->items[i] = arg0.ARRAY->items[i];
		res->items[a0_len] = arg1;
		return (Element) { ARRAY, .ARRAY = res };
	}
	return error(CONCAT(a, str("Wrong types for push, got "),
				type_str(arg0.type), str(" and "), type_str(arg1.type)));
}

priv Element builtin_type(Arena *a, Namespace *ns, ElemArray *args)
{
	if (args->len != 1)
		return error(str_fmt(a, "Wrong number of args for type: got %lu, expected 1", args->len));
	return (Element) { TYPE, .TYPE = args->items[0].type };
}

priv Element builtin_print(Arena *a, Namespace *ns, ElemArray *args)
{
	u64	previous_offset = a->used;
	for (int i = 0; i < args->len; i++)
		str_print(to_string(a, args->items[i]));
	arena_pop_to(a, previous_offset);
	str_print(str("\n"));
	return (Element) { ELE_NULL };
}

priv Element builtin_len(Arena *a, Namespace *ns, ElemArray *args)
{
	if (args->len != 1)
		return error(str_fmt(a, "Wrong number of args for len: got %lu, expected 1", args->len));
	Element arg0 = args->items[0];
	if (arg0.type == STR)
		return (Element) { INT, .INT = arg0.STR.len };
	if (arg0.type == ARRAY)
		return (Element) { INT, .INT = arg0.ARRAY->len };
	if (arg0.type == LIST)
		return (Element) { INT, .INT = arg0.LIST->len };
	return error(CONCAT(a, str("Type error: len called with argument of type: "), type_str(arg0.type)));
}

// HELPER
priv Element *elem_alloc(Arena *a, Element elem)
{
	Element	*ptr = arena_alloc(a, sizeof(Element));
	if (elem.type == STR || elem.type == ERR)
		elem.STR = str_dup(a, elem.STR);
	if (elem.type == RETURN)
		elem.RETURN.value = elem_alloc(a, *elem.RETURN.value);
	if (elem.type == LIST) {
		elem.LIST = elemlist_copy(a, elem.LIST);
	}
	if (elem.type == ARRAY) {
		elem.ARRAY = elemarray_copy(a, elem.ARRAY);
	}
	if (elem.type == FUNCTION) {
		elem.FUNCTION.params = astlist_copy(a, elem.FUNCTION.params);
		elem.FUNCTION.body = astlist_copy(a, elem.FUNCTION.body);
		elem.FUNCTION.namespace = ns_copy(a, elem.FUNCTION.namespace);
	}
	*ptr = elem;
	return ptr;
}

priv Element elem_copy(Arena *a, Element elem)
{
	return (*elem_alloc(a, elem));
}

// STDOUT
priv String array_to_string(Arena *a, ElemArray *arr);
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
			return CONCAT(a, str("\""), e.STR, str("\""));
		case RETURN:
			return to_string(a, (*e.RETURN.value));
		case ARRAY:
			return array_to_string(a, e.ARRAY);
		case LIST:
			return list_to_string(a, e.LIST);
		case FUNCTION:
			return str_fmt(a, "fn(namespace: %p)", e.FUNCTION.namespace);
		case ERR:
			return e.ERR;
		case BUILTIN:
			return str("builtin fn");
		case TYPE:
			return type_str(e.TYPE);
	}
	return (NEVER(0 && "Some type slept through?"), str(""));
}

priv String array_to_string(Arena *a, ElemArray *arr)
{
	if (!arr) return str("");
	char *buf = arena_alloc(a, 1);
	buf[0] = '[';
	u64 len = 1;
	for (int i = 0; i < arr->len; i++) {
		String tmp = to_string(a, arr->items[i]);
		arena_alloc(a, tmp.len);
		memcpy(buf + len, tmp.buf, tmp.len);
		len += tmp.len;
		if (i < (arr->len - 1)) {
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
		str("BOOL"), str("STR"), str("LIST"), str("ARRAY"),
		str("RETURN"), str("FUNCTION"), str("BUILTIN"),
		str("TYPE")
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
	node->element = elem_copy(l->arena, el);
	node->next = NULL;

	if (NEVER(!l));

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

priv ElemList *elemlist_from_ast(Arena *a, Namespace *ns, ASTList *lst)
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

priv ElemList *elemlist_copy(Arena *a, ElemList *lst)
{
	ElemList *res = elemlist(a);
	for (ElemNode *cursor = lst->head; cursor; cursor = cursor->next)
		elempush(res, cursor->element);
	return res;
}

// ~ELEMARRAY
priv ElemArray *elemarray(Arena *a, int len)
{
	ElemArray *arr = arena_alloc_zero(a, sizeof(ElemArray));
	arr->items = arena_alloc(a, len * sizeof(Element));
	arr->len = len;
	return arr;
}


priv ElemArray *elemarray_single(Arena *a, Element el);
priv ElemArray *elemarray_from_ast(Arena *a, Namespace *ns, ASTList *lst)
{
	u64	previous_offset = a->used;
	ElemArray *arr = elemarray(a, lst->len);

	int	i = 0;
	Element tmp = {0};
	for (ASTNode *cursor = lst->head; cursor; cursor = cursor->next) {
		tmp = eval(a, ns, cursor->ast);
		if (tmp.type == ERR) {
			arena_pop_to(a, previous_offset);
			return elemarray_single(a, tmp);
		}
		arr->items[i] = tmp;
		i++;
	}
	return arr;
}

priv ElemArray *elemarray_single(Arena *a, Element el)
{
	ElemArray *arr = elemarray(a, 1);
	arr->items[0] = elem_copy(a, el);
	return arr;
}

priv ElemArray *elemarray_copy(Arena *a, ElemArray *arr)
{
	ElemArray *res = elemarray(a, arr->len);
	for (int i = 0; i < arr->len; i++)
		res->items[i] = elem_copy(a, arr->items[i]);
	return res;
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
	b->key = str_dup(a, key);
	b->element = el;
	b->mutable = is_mutable;
	b->next = NULL;
	return b;
}

priv int ns_put(Namespace *ns, String key, Element elem, bool is_mutable)
{
	u64 id = hash(key) % ns->cap;
	for (Bind *b = ns->values[id]; b; b = b->next) {
		if (str_eq(key, b->key)) { // if key used, update
			if (is_mutable) {
				b->element = elem;
				return 1;
			}
			else
				return -1; // found an immutable binding
		}
	}
	Bind *b = bind_alloc(ns->arena, key, elem, is_mutable);
	b->next = ns->values[id];
	ns->values[id] = b;
	ns->len++;
	return 1;
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

priv int ns_update(Namespace *ns, String key, Element elem)
{
	// Updates first hit of binding in the namespace it's found - Assumes it's been confirmed to exist as MUTABLE before
	Bind *target = ns_get_inner(ns, key);
	if (!target) {
		if (ns->parent)
			return ns_update(ns->parent, key, elem);
		else
			return 0;
	}
	if (NEVER(!target->mutable))
		return 0;
	target->element = elem_copy(ns->arena, elem);
	return 1;
}

priv Namespace *ns_copy(Arena *a, Namespace *ns)
{
	Namespace *res = ns_create(a, ns->cap);
	for (int i = 0; i < ns->cap; i++) {
		if (ns->values[i]) {
			res->values[i] =
			    bind_alloc(res->arena, ns->values[i]->key,
				       ns->values[i]->element, ns->values[i]->mutable);
			res->len++;
			if (ns->values[i]->next) {
				for (Bind *ns_cursor = ns->values[i]->next;
				     ns_cursor; ns_cursor = ns_cursor->next) {
					Bind *p = bind_alloc(a, ns_cursor->key,
							     ns_cursor->element, ns_cursor->mutable);
					p->next = res->values[i];
					res->values[i] = p;
					res->len++;
				}
			}
		}
	}
	return res;
}
