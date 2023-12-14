#include "tests.h"
#include <assert.h>

ASTList *astlist(Arena *a);
void astpush(ASTList *l, AST *ast);
AST *ast_alloc(Arena *a, AST node);

TestResult identifier_tests(Arena *a);
TestResult int_literal_tests(Arena *a);
TestResult string_literal_tests(Arena *a);
TestResult bool_literal_tests(Arena *a);
TestResult list_literal_tests(Arena *a);
TestResult index_expression_tests(Arena *a);
TestResult prefix_expression_tests(Arena *a);
TestResult infix_expression_tests(Arena *a);
TestResult operator_precedence_tests(Arena *a);
TestResult conditional_expression_tests(Arena *a);
TestResult function_literal_tests(Arena *a);
TestResult function_call_tests(Arena *a);
int main(int ac, char **av)
{
	Arena *a = arena(MB(1));
	u64 fail_count = 0;
	Test tests[] = {
	    {str("IDENTIFIER EXPRESSIONS"), &identifier_tests},
	    {str("INT LITERAL EXPRESSIONS"), &int_literal_tests},
	    {str("STRING LITERAL EXPRESSIONS"), &string_literal_tests},
	    {str("BOOL LITERAL EXPRESSIONS"), &bool_literal_tests},
	    {str("LIST LITERAL EXPRESSIONS"), &list_literal_tests},
	    {str("INDEX EXPRESSIONS"), &index_expression_tests},
	    {str("PREFIX EXPRESSIONS"), &prefix_expression_tests},
	    {str("INFIX EXPRESSIONS"), &infix_expression_tests},
	    {str("OPERATOR PRECEDENCE"), &operator_precedence_tests},
	    {str("CONDITIONAL EXPRESSIONS"), &conditional_expression_tests},
	    {str("FUNCTION LITERALS"), &function_literal_tests},
	    {str("FUNCTION CALLS"), &function_call_tests},
	};
	if (ac < 2) {
		for (int i = 0; i < arrlen(tests); i++) {
			str_print(str_fmt(a, "TEST PARSER %d: ", i));
			if (!run_test(a, tests[i]))
				fail_count++, str_print(str("\tTEST PARSER FAILURE\n"));
		}
		str_print(str_fmt(a, "\tTEST PARSER summary: (%lu/%lu)\n\n",
					arrlen(tests) - fail_count, arrlen(tests)));
	}
	for (int i = 1; i < ac; i++) {
		int test_id = atoi(av[i]);
		if (test_id >= arrlen(tests))
			str_print(str_fmt(a, "Test %d not found.\n", test_id)), exit(1);
		if (!run_test(a, tests[test_id]))
			str_print(str("\t TEST PARSER FAILURE\n"));
	}
	arena_free(&a);
}

void parse_stmt_tests(Arena *a)
{
	// val statements
	// return statements
	return;
}

TestResult identifier_tests(Arena *a)
{
	Lexer *l = lexer(a, str("name_dos; blank;i;"));
	Parser *p= parser(a, l);
	String expected[] = {str("name_dos"), str("blank"), str("i")};

	AST *prog = parse_program(p);
	ASTNode *cursor = prog->AST_LIST->head;

	int i = 0;
	while (i < arrlen(expected)) {
		if (TEST(!cursor))
			return fail(str("Less statements than expected"));
		if (TEST(cursor->ast->type != AST_IDENT))
			return fail(str_fmt(a, "Expected: %*.s Type mismatch",
						fmt(asttype_str(AST_IDENT))));
		if (TEST(!str_eq(cursor->ast->AST_STR, expected[i]))) {
			return fail(str_fmt(a, "Expected: %*.s \nActual: %*.s\nMismatch in literal",
						fmt(expected[i]), fmt(cursor->ast->AST_STR)));
		}
		cursor = cursor->next;
		i++;
	}
	return pass();
}

TestResult int_literal_tests(Arena *a)
{
	Lexer *l = lexer(a, str("5; 10;153;"));
	Parser *p= parser(a, l);
	long expected_val[] = {5, 10, 153};

	AST *prog = parse_program(p);
	ASTNode *cursor = prog->AST_LIST->head;

	int i = 0;
	while (i < arrlen(expected_val)) {
		if (TEST(!cursor))
			return fail(str("Less statements than expected"));
		if (TEST(cursor->ast->AST_INT.value != expected_val[i]))
			return fail(str_fmt(a, "Expected: %ld\nActual: %ld\nMismatch in value",
						expected_val[i], cursor->ast->AST_INT.value
						));
		cursor = cursor->next;
		i++;
	}
	return pass();
}

TestResult string_literal_tests(Arena *a)
{
	Lexer *l= lexer(a, str("\"Hiya Worldo\""));
	Parser *p= parser(a, l);
	

	AST *prog = parse_program(p);
	ASTNode *cursor = prog->AST_LIST->head;
	String expected_value[] = {str("Hiya Worldo")};

	int i = 0;
	while (i < arrlen(expected_value)) {
		if (TEST(!cursor))
			return fail(str("Less statements than expected"));

		if (TEST(!str_eq(cursor->ast->AST_STR, expected_value[i])))
			return fail(str_fmt(a, "Expected: %*.s\nActual: %*s\nMismatch in value",
						fmt(expected_value[i]), fmt(cursor->ast->AST_STR)));
		cursor = cursor->next;
		i++;
	}
	return pass();
}

TestResult bool_literal_tests(Arena *a)
{
	Lexer *l = lexer(a, str("true; false;"));
	Parser *p = parser(a, l);

	bool expected_value[] = {true, false};

	AST *prog = parse_program(p);
	ASTNode *cursor = prog->AST_LIST->head;

	int i = 0;
	while (i < arrlen(expected_value)) {
		if (TEST(!cursor))
			return fail(str("Less statements than expected"));
		if (TEST((cursor->ast->AST_BOOL.value != expected_value[i])))
			return fail(str_fmt(a, "Expected: %d\nActual: %d\nMismatch in value",
						expected_value[i], cursor->ast->AST_BOOL.value));
		cursor = cursor->next;
		i++;
	}
	return pass();
}

TestResult prefix_expression_tests(Arena *a)
{
	Lexer *l= lexer(a, str("-3;!10;"));
	Parser *p= parser(a, l);
	i64 expected_value[] = {3, 10};


	AST *prog = parse_program(p);
	ASTNode *cursor = prog->AST_LIST->head;

	int i = 0;
	while (i < arrlen(expected_value)) {
		if (TEST(!cursor))
			return (TestResult){
			    false, str("Less statements than expected")};
		if (TEST(cursor->ast->AST_PREFIX.right->AST_INT.value !=
		    expected_value[i]))
			return fail(str_fmt(
				a,
				"Expected: %ld\nActual: %ld\nMismatch in value",
				expected_value[i],
				cursor->ast->AST_PREFIX.right->AST_INT.value));
		cursor = cursor->next;
		i++;
	}
	return pass();
}

typedef struct InfixTestInput {
	String input;
	i64 left_val;
	String op;
	i64 right_val;
} InfixTestInput;
TestResult test_infix(Arena *a, InfixTestInput params);
TestResult infix_expression_tests(Arena *a)
{
	InfixTestInput tests[] = {
	    {str("10 + 3"), 10, str("+"), 3},
	    {str("3 - 10"), 3, str("-"), 10},
	    {str("7 * 5"), 7, str("*"), 5},
	    {str("7 / 5"), 7, str("/"), 5},
	    {str("3 > 10"), 3, str(">"), 10},
	    {str("3 < 10"), 3, str("<"), 10},
	    {str("5 == 5"), 5, str("=="), 5},
	    {str("5 != 5"), 5, str("!="), 5},
	    {str("true == true"), true, str("=="), true},
	    {str("false == false"), false, str("=="), false},
	    {str("true != false"), true, str("!="), false},
	};
	TestResult res = {0};
	for (int i = 0; i < arrlen(tests); i++) {
		res = test_infix(a, tests[i]);
		if (TEST(!res.passed)) return res;
	}
	return pass();
}

TestResult list_literal_tests(Arena *a)
{
	Lexer *l= lexer(a, str("[1, 3 * 5, 7 + 3]"));
	Parser *p = parser(a, l);
	ASTList *expected_list = astlist(a);
	astpush(expected_list, ast_alloc(a, (AST) { AST_INT, .AST_INT = {1} }));
	astpush(expected_list, ast_alloc(a, (AST) { AST_INFIX, .AST_INFIX = {
				.left = ast_alloc(a, (AST) { AST_INT, .AST_INT = {3} }),
				.op = str("*"),
				.right = ast_alloc(a, (AST) { AST_INT, .AST_INT = {5} })
				}}));

	astpush(expected_list, ast_alloc(a, (AST) { AST_INFIX, .AST_INFIX = {
				.left = ast_alloc(a, (AST) { AST_INT, .AST_INT = {7} }),
				.op = str("+"),
				.right = ast_alloc(a, (AST) { AST_INT, .AST_INT = {3} })
				}}));

	AST	*expected_node = ast_alloc(a, (AST) {
			AST_LIST, .AST_LIST = expected_list
			});

	AST *prog = parse_program(p);
	AST *ex = prog->AST_LIST->head->ast;

	if (TEST(ex->type != AST_LIST))
		return fail(str_fmt(a, "Expression is not of type AST_LIST got: %*.s",
					asttype_str(ex->type)));
	if (TEST(ex->AST_LIST->len != 3))
		return fail(str_fmt(a, "Wrong list len, expected %lu got %lu", 3,
				   ex->AST_LIST->len));
	if (TEST(!ast_eq(expected_node, ex)))
		return fail(str("Nodes are different"));
	return pass();
}

TestResult index_expression_tests(Arena *a)
{
	Lexer *l= lexer(a, str("theList[1 + 1]"));
	Parser *p= parser(a, l);
	AST *prog = parse_program(p);
	AST *ex = prog->AST_LIST->head->ast;

	if (TEST(ex->type != AST_INDEX))
		return fail(str_fmt(a, "Expression is not of type AST_INDEX, got: %*s",
					asttype_str(ex->type)));
	return pass();
}

priv i64 get_rvalue(AST *node)
{
	if (node->type == AST_INT)
		return node->AST_INT.value;
	if (node->type == AST_BOOL)
		return node->AST_BOOL.value;
	assert(0 && "Node does not hold an rvalue");
}
TestResult test_infix(Arena *a, InfixTestInput params)
{
	Lexer *l= lexer(a, params.input);
	Parser *p = parser(a, l);
	AST *prog = parse_program(p);
	if (p->errors && p->errors->len > 0) {
		parser_print_errors(p);
		return fail(str("Parsing has errors"));
	}
	AST *actual = prog->AST_LIST->head->ast;
	if (get_rvalue(actual->AST_INFIX.left) != params.left_val)
		return fail(str_fmt(
			a, "Expected: %ld\nActual: %ld\nLeft value mismatch",
			params.left_val, get_rvalue(actual->AST_INFIX.left)));
	if (!str_eq(params.op, actual->AST_INFIX.op))
		return fail(str_fmt(a, "Actual: %*.s\nExpected: %*.s",
					actual->AST_INFIX.op, params.op));
	if (get_rvalue(actual->AST_INFIX.right) != params.right_val)
		return fail(str_fmt(
			a, "Expected: %ld\nActual: %ld\nRight value mismatch",
			params.right_val, get_rvalue(actual->AST_INFIX.right)));
	return pass();
}

struct OperatorPrecInput {
	String input;
	String expected;
};
TestResult test_operator_precedence(Arena *a, struct OperatorPrecInput params);
TestResult operator_precedence_tests(Arena *a)
{
	struct OperatorPrecInput tests[] = {
	    {
		str("-a * b"),
		str("((-a)*b)"),
	    },
	    {
		str("!-a"),
		str("(!(-a))"),
	    },
	    {
		str("- 5 * 5"),
		str("((-5)*5)"),
	    },
	    {
		str("5 < 4 != 3 > 4"),
		str("((5<4)!=(3>4))"),
	    },
	    {
		str("3 + (2 + 5) + 5"),
		str("((3+(2+5))+5)"),
	    },
	    {
		str("2 / (5 + 5)"),
		str("(2/(5+5))"),
	    },
	    {str("a * [1, 2, 3, 4][b * c] * d"),
	     str("((a*([1, 2, 3, 4][(b*c)]))*d)")}};
	TestResult res = {0};
	for (int i = 0; i < arrlen(tests); i++) {
		res = test_operator_precedence(a, tests[i]);
		if (!res.passed) return res;
	}
	return pass();
}

TestResult test_operator_precedence(Arena *a, struct OperatorPrecInput params)
{
	Lexer *l= lexer(a, params.input);
	Parser *p= parser(a, l);
	AST *prog = parse_program(p);
	if (TEST(p->errors && p->errors->len > 0)) {
		parser_print_errors(p);
		return fail(str("Parsing has errors"));
	}
	AST *ex = prog->AST_LIST->head->ast;
	String ex_string = ast_str(a, ex);
	if (TEST(!str_eq(ex_string, params.expected))) {
		return fail(str_fmt(a, "Expected: %*.s\nActual: %*.s\nExpression strings do not match",
					fmt(params.expected), fmt(ex_string)));
	}
	return pass();
}

TestResult conditional_expression_tests(Arena *a)
{

	Lexer *l= lexer(a, str("if (x < y) { x }"));
	Parser *p= parser(a, l);

	AST *prog = parse_program(p);
	AST	*actual = prog->AST_LIST->head->ast;

	ASTList	*expected_consequence = astlist(a);
		astpush(expected_consequence, ast_alloc(a, (AST) { AST_IDENT, .AST_STR = str("x") }));
	AST *expected = ast_alloc(a, (AST) { AST_COND, .AST_COND = { 
			ast_alloc(a, (AST) { AST_INFIX, .AST_INFIX = { ast_alloc(a, (AST) { AST_IDENT, .AST_STR = str("x") }),
															str("<"), 
															ast_alloc(a, (AST) { AST_IDENT, .AST_STR = str("y") })}}),
			expected_consequence,
			NULL
			}});

	if (TEST(!ast_eq(actual, expected)))
		return fail(str("Nodes are not equal"));
	return pass();
}

TestResult function_literal_tests(Arena *a)
{
	Lexer *l= lexer(a, str("fn(x,y) { x + y;}"));
	Parser *p = parser(a, l);

	AST *prog = parse_program(p);
	AST	*actual = prog->AST_LIST->head->ast;

	ASTList *exp_params = astlist(a);
		astpush(exp_params, ast_alloc(a, (AST) { AST_IDENT, .AST_STR = str("x") }));
		astpush(exp_params, ast_alloc(a, (AST) { AST_IDENT, .AST_STR = str("y") }));
	ASTList *exp_body = astlist(a);
		astpush(exp_body, ast_alloc(a, (AST) { AST_INFIX, 
				.AST_INFIX = {	ast_alloc(a, (AST) { AST_IDENT, .AST_STR = str("x") }),
								str("+"),
								ast_alloc(a, (AST) { AST_IDENT, .AST_STR = str("y") })}}));

	AST *expected = ast_alloc(a, (AST) { AST_FN, .AST_FN = { exp_params, exp_body } });

	if (TEST(p->errors && p->errors->len > 0)) {
		parser_print_errors(p);
		return fail(str("Parsing has errors"));
	}
	if (TEST(!ast_eq(actual, expected)))
		return fail(str("Nodes are not equal"));
	return pass();
}

TestResult function_call_tests(Arena *a)
{
	Lexer *l= lexer(a, str("suma(5 * 3, 2, 1 + 10)"));
	Parser *p= parser(a, l);
	AST *prog = parse_program(p);

	if (TEST(p->errors && p->errors->len > 0))
		return (parser_print_errors(p), fail(str("Parsing has errors")));

	AST *expected_fn = ast_alloc(a, (AST) { AST_IDENT, .AST_STR = str("suma") });
	ASTList *expected_args = astlist(a);
		astpush(expected_args, ast_alloc(a, (AST) { AST_INFIX, 
					.AST_INFIX = {
						.left = ast_alloc(a, (AST) { AST_INT, .AST_INT = {5} }),
						.op = str("*"),
						.right = ast_alloc(a, (AST) { AST_INT, .AST_INT = {3} })
					}
					}));
		astpush(expected_args, ast_alloc(a, (AST) { AST_INT, .AST_INT = {2} }));
		astpush(expected_args, ast_alloc(a, (AST) { AST_INFIX, 
					.AST_INFIX = {
						.left = ast_alloc(a, (AST) { AST_INT, .AST_INT = {1} }),
						.op = str("+"),
						.right = ast_alloc(a, (AST) { AST_INT, .AST_INT = {10} })
					}}));
	AST *expected = ast_alloc(a, (AST) { AST_CALL, .AST_CALL = { expected_fn, expected_args }});
	AST	*actual = prog->AST_LIST->head->ast;
	if (TEST(!ast_eq(expected, actual)))
		return fail(str("Actual and expected are not equal"));
	return pass();
}
