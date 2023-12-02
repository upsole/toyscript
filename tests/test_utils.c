#include "tests.h"

bool run_test(Arena *arena, Test test)
{
	str_print(test.title);
	TestResult res = (*test.fn)(arena);
	if (!res.passed) {
		str_print(str(" KO\n"));
		str_print(res.msg);
		str_print(str("\n"));
		return false;
	} else {
		str_print(str(" OK\n"));
		return true;
	}
	arena_reset(arena);
}

TestResult fail(String msg)
{
	return (TestResult) { false, msg };
}

TestResult pass()
{
	return (TestResult){true, str("")};
}

bool astlist_eq(ASTList *l1, ASTList *l2)
{
	if (TEST(l1->len != l2->len))
		return false;
	for (ASTNode *c1 = l1->head, *c2 = l2->head; 
			c1 && c2; c1 = c1->next, c2 = c2->next)
		if (TEST(!ast_eq(c1->ast, c2->ast)))
			return false;
	return true;
}

bool ast_eq(AST *node1, AST *node2)
{

	if (!node1) 
		return (!node2) ? true : false;
	if (!node2) 
		return (!node1) ? true : false;
	
	if (node1->type != node2->type)
		return false;
	ASTType type = node1->type;
	switch (type) {
		case AST_INT:
			return node1->AST_INT.value == node2->AST_INT.value;
		case AST_BOOL:
			return node1->AST_BOOL.value == node2->AST_BOOL.value;
		case AST_IDENT:
		case AST_STR:
			return str_eq(node1->AST_STR, node2->AST_STR);
		case AST_PROGRAM:
		case AST_LIST:
			return astlist_eq(node1->AST_LIST, node2->AST_LIST);
		case AST_RETURN:
			return ast_eq(node1->AST_RETURN.value, node2->AST_RETURN.value);
		case AST_VAL: {
			if (!str_eq(node1->AST_VAL.name, node2->AST_VAL.name))
				return false;
			return ast_eq(node1->AST_VAL.value, node2->AST_VAL.value);
		} break;
		case AST_VAR: {
			if (!str_eq(node1->AST_VAR.name, node2->AST_VAR.name))
				return false;
			return ast_eq(node1->AST_VAR.value, node2->AST_VAR.value);
		} break;
		case AST_ASSIGN: 
			return (ast_eq(node1->AST_ASSIGN.left, node2->AST_ASSIGN.left) &&
					ast_eq(node1->AST_ASSIGN.right, node2->AST_ASSIGN.right));
		case AST_FN: 
			return (astlist_eq(node1->AST_FN.params, node2->AST_FN.params) && 
						astlist_eq(node1->AST_FN.body, node2->AST_FN.body)); 
		case AST_PREFIX: {
			if (!ast_eq(node1->AST_PREFIX.right, node2->AST_PREFIX.right))
				return false;
			return str_eq(node1->AST_PREFIX.op, node2->AST_PREFIX.op);
		} break;
		case AST_INFIX: {
		if (!ast_eq(node1->AST_INFIX.left, node2->AST_INFIX.left))
			return false;
		if (!str_eq(node1->AST_INFIX.op, node2->AST_INFIX.op))
			return false;
		return (ast_eq(node1->AST_INFIX.right, node2->AST_INFIX.right));
		} break;
		case AST_COND: {
			if (!ast_eq(node1->AST_COND.condition, node2->AST_COND.condition))
				return false;
			if (!astlist_eq(node1->AST_COND.consequence, node2->AST_COND.consequence))
				return false;
			if (node1->AST_COND.alternative)
				return (astlist_eq(node1->AST_COND.alternative, node2->AST_COND.alternative));
			return true;
		} break;
		case AST_CALL: {
			if (!ast_eq(node1->AST_CALL.function, node2->AST_CALL.function))
				return false;
			return (astlist_eq(node1->AST_CALL.args, node2->AST_CALL.args));
		} break;
		case AST_INDEX: {
			if (!ast_eq(node1->AST_INDEX.left, node2->AST_INDEX.left))
				return false;
			return (ast_eq(node1->AST_INDEX.index, node2->AST_INDEX.index));
		} break;
	}
	return NEVER(0 && "Some type slept through?");
}

