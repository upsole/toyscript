#include "tests.h"

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
	Arena *arena = arena_init(MB(1));
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
			str_print(str_fmt(arena, "TEST PARSER %d: ", i));
			if (!run_test(arena, tests[i]))
				fail_count++, fail("\tTEST PARSER FAILURE\n");
		}
		printf("\tTEST PARSER summary: (%lu/%lu)\n\n",
		       arrlen(tests) - fail_count, arrlen(tests));
	}
	for (int i = 1; i < ac; i++) {
		int test_id = atoi(av[i]);
		if (test_id >= arrlen(tests))
			printf("Test %d not found.\n", test_id), exit(1);
		;
		if (!run_test(arena, tests[test_id]))
			fail("\t TEST PARSER FAILURE\n");
	}
	arena_free(&arena);
}

static i64 get_val(Expression ex);
void parse_stmt_tests(Arena *a)
{
	// val statements
	// return statements
	return;
}

TestResult identifier_tests(Arena *a)
{
	Lexer *lexer = lex_init(a, str("name_dos; blank;i;"));
	Parser *parser = parser_init(a, lexer);
	Program prog = parse_program(parser);
	StmtNode *cursor = prog.statements->head;
	String expected[] = {str("name_dos"), str("blank"), str("i")};

	int i = 0;
	while (i < arrlen(expected)) {
		if (!cursor)
			return (TestResult){
			    false, str("Less statements than expected")};
		if (!str_eq(cursor->s._exp._ident.value, expected[i])) {
			return (TestResult){
			    false, str_concatv(a, 5, str("Expected: "),
					       expected[i], str("\nActual: "),
					       cursor->s._exp._ident.value,
					       str("\nMismatch in literal"))};
		}
		cursor = cursor->next;
		i++;
	}
	return (TestResult){true, str("")};
}

TestResult int_literal_tests(Arena *a)
{
	Lexer *lexer = lex_init(a, str("5; 10;153;"));
	Parser *parser = parser_init(a, lexer);
	Program prog = parse_program(parser);
	StmtNode *cursor = prog.statements->head;
	String expected_lit[] = {str("5"), str("10"), str("153")};
	long expected_value[] = {5, 10, 153};
	int i = 0;
	while (i < arrlen(expected_lit)) {
		if (!cursor)
			return (TestResult){
			    false, str("Less statements than expected")};
		if (!str_eq(cursor->s._exp._int.tok.lit, expected_lit[i]))
			return (TestResult){
			    false,
			    str_concatv(a, 5, str("Expected: "),
					expected_lit[i], str("\nActual: "),
					cursor->s._exp._int.value,
					str("\nMismatch in literal"))};
		if (!str_eq(cursor->s._exp._int.tok.lit, expected_lit[i]) ||
		    (cursor->s._exp._int.value != expected_value[i]))
			return (TestResult){
			    false,
			    str_fmt(
				a,
				"Expected: %ld\nActual: %ld\nMismatch in value",
				expected_value[i], cursor->s._exp._int.value)};
		cursor = cursor->next;
		i++;
	}
	return pass();
}

TestResult string_literal_tests(Arena *a)
{
	Lexer *lexer = lex_init(a, str("\"Hiya Worldo\""));
	Parser *parser = parser_init(a, lexer);
	Program prog = parse_program(parser);
	StmtNode *cursor = prog.statements->head;
	String expected_lit[] = {str("Hiya Worldo")};
	String expected_value[] = {str("Hiya Worldo")};

	int i = 0;
	while (i < arrlen(expected_lit)) {
		if (!cursor)
			return (TestResult){
			    false, str("Less statements than expected")};
		if (!str_eq(cursor->s._exp._string.tok.lit, expected_lit[i]))
			return (TestResult){
			    false,
			    str_concatv(a, 5, str("Expected: "),
					expected_lit[i], str("\nActual: "),
					cursor->s._exp._int.value,
					str("\nMismatch in literal"))};
		if (!str_eq(cursor->s._exp._string.value, expected_value[i]))
			return (TestResult){
			    false,
			    str_concatv(a, 5, str("Expected: "),
					expected_value[i], str("\nActual: "),
					cursor->s._exp._string.value,
					str("\nMismatch in value"))};
		cursor = cursor->next;
		i++;
	}
	return pass();
}

TestResult bool_literal_tests(Arena *a)
{
	Lexer *lexer = lex_init(a, str("true; false;"));
	Parser *parser = parser_init(a, lexer);
	Program prog = parse_program(parser);
	StmtNode *cursor = prog.statements->head;
	String expected_lit[] = {str("true"), str("false")};
	bool expected_value[] = {true, false};

	int i = 0;
	while (i < arrlen(expected_lit)) {
		if (!cursor)
			return (TestResult){
			    false, str("Less statements than expected")};

		if (!str_eq(cursor->s._exp._bool.tok.lit, expected_lit[i]))
			return (TestResult){
			    false,
			    str_concatv(a, 5, str("Expected: "),
					expected_lit[i], str("\nActual: "),
					cursor->s._exp._bool.tok.lit,
					str("\nMismatch in literal"))};
		if ((cursor->s._exp._bool.value != expected_value[i]))
			return (TestResult){
			    false,
			    str_fmt(
				a,
				"Expected: %d\nActual: %d\nMismatch in value",
				expected_value[i], cursor->s._exp._bool.value)};
		cursor = cursor->next;
		i++;
	}
	return pass();
}

TestResult prefix_expression_tests(Arena *a)
{
	Lexer *lexer = lex_init(a, str("-3;!10;"));
	Parser *parser = parser_init(a, lexer);
	Program prog = parse_program(parser);
	StmtNode *cursor = prog.statements->head;
	String expected_lit[] = {str("-"), str("!")};
	i64 expected_value[] = {3, 10};
	int i = 0;

	while (i < arrlen(expected_lit)) {
		if (!cursor)
			return (TestResult){
			    false, str("Less statements than expected")};
		if (!str_eq(cursor->s._exp._prefix.operator, expected_lit[i]))
			return (TestResult){
			    false,
			    str_concatv(a, 5, str("Expected: "),
					expected_lit[i], str("\nActual: "),
					cursor->s._exp._prefix.operator,
					str("\nMismatch in literal"))};
		if (cursor->s._exp._prefix.right->_int.value !=
		    expected_value[i])
			return (TestResult){
			    false,
			    str_fmt(
				a,
				"Expected: %ld\nActual: %ld\nMismatch in value",
				expected_value[i],
				cursor->s._exp._prefix.right->_int.value)};
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
		if (!res.passed) return res;
	}
	return pass();
}

TestResult list_literal_tests(Arena *a)
{
	Lexer *lexer = lex_init(a, str("[1, 3 * 5, 7 + 3]"));
	Parser *parser = parser_init(a, lexer);
	Program prog = parse_program(parser);
	Expression ex = prog.statements->head->s._exp;

	if (ex.type != EXP_LIST)
		return (TestResult){
		    false,
		    str_concat(a,
			       str("Expression is not of type EXP_LIST got: "),
			       exptype_string(ex.type))};
	if (ex._list.items->len != 3)
		return (TestResult){
		    false, str_fmt(a, "Wrong list len, expected %lu got %lu", 3,
				   ex._list.items->len)};

	ExpNode *cursor = ex._list.items->head;
	if (cursor->ex.type != EXP_INT)
		return (TestResult){
		    false,
		    str_concat(a, str("List[0] is not of type EXP_INT got: "),
			       exptype_string(cursor->ex.type))};
	cursor = cursor->next;
	if (cursor->ex.type != EXP_INFIX) {
		return (TestResult){
		    false,
		    str_concat(a, str("List[1] is not of type EXP_INFIX got: "),
			       exptype_string(cursor->ex.type))};
	}

	if (!str_eq(cursor->ex._infix.operator, str("*"))) {
		return (TestResult){
		    false,
		    str_concatv(a, 5, str("Expected: "), str("*"),
				str("\nActual: "), cursor->ex._infix.operator,
				str("\nWrong infix operator at List[1]"))};
	}
	cursor = cursor->next;
	if (cursor->ex.type != EXP_INFIX) {
		return (TestResult){
		    false,
		    str_concat(a, str("List[2] is not of type EXP_INFIX got: "),
			       exptype_string(cursor->ex.type))};
	}

	if (!str_eq(cursor->ex._infix.operator, str("+"))) {
		return (TestResult){
		    false,
		    str_concatv(a, 5, str("Expected: "), str("+"),
				str("\nActual: "), cursor->ex._infix.operator,
				str("\nWrong infix operator at List[2]"))};
	}
	return pass();
}

TestResult index_expression_tests(Arena *a)
{
	Lexer *lexer = lex_init(a, str("theList[1 + 1]"));
	Parser *parser = parser_init(a, lexer);
	Program prog = parse_program(parser);
	Expression ex = prog.statements->head->s._exp;

	if (ex.type != EXP_INDEX)
		return (TestResult){
		    false,
		    str_concat(a,
			       str("Expression is not of type EXP_INDEX got: "),
			       exptype_string(ex.type))};

	return pass();
}

TestResult test_infix(Arena *a, InfixTestInput params)
{
	Lexer *lexer = lex_init(a, params.input);
	Parser *parser = parser_init(a, lexer);
	Program prog = parse_program(parser);
	if (parser->errors && parser->errors->len > 0) {
		strlist_print(parser->errors);
		return (TestResult){false, str("Parsing has errors")};
	}
	Statement actual = prog.statements->head->s;
	if (get_val(*actual._exp._infix.left) != params.left_val)
		return (TestResult){
		    false,
		    str_fmt(
			a, "Expected: %ld\nActual: %ld\nLeft value mismatch",
			params.left_val, get_val(*actual._exp._infix.left))};

	if (!str_eq(params.op, actual._exp._infix.operator))
		return (TestResult){
		    false, str_concatv(a, 4, str("Actual: "),
				       actual._exp._infix.operator,
				       str("\nExpected: "), params.op)};

	if (get_val(*actual._exp._infix.right) != params.right_val)
		return (TestResult){
		    false,
		    str_fmt(
			a, "Expected: %ld\nActual: %ld\nRight value mismatch",
			params.right_val, get_val(*actual._exp._infix.right))};
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
	     str("((a*(([1 2 3 4])[(b*c)]))*d)")}};
	TestResult res = {0};
	for (int i = 0; i < arrlen(tests); i++) {
		res = test_operator_precedence(a, tests[i]);
		if (!res.passed) return res;
	}
	return pass();
}

TestResult test_operator_precedence(Arena *a, struct OperatorPrecInput params)
{
	Lexer *lexer = lex_init(a, params.input);
	Parser *parser = parser_init(a, lexer);
	Program prog = parse_program(parser);
	if (parser->errors && parser->errors->len > 0) {
		strlist_print(parser->errors);
		return (TestResult){false, str("Parsing has errors")};
	}
	Expression ex = prog.statements->head->s._exp;
	String ex_string = exp_string(a, ex);
	if (!str_eq(ex_string, params.expected)) {
		return (TestResult){
		    false,
		    str_concatv(a, 5, str("Expected: "), params.expected,
				str("\nActual: "), ex_string,
				str("\nExpression strings do not match"))};
	}
	return pass();
}

TestResult conditional_expression_tests(Arena *a)
{

	Lexer *lexer = lex_init(a, str("if (x < y) { x }"));
	Parser *parser = parser_init(a, lexer);
	Program prog = parse_program(parser);

	Expression actual = prog.statements->head->s._exp;
	if (actual.type != EXP_COND)
		return (TestResult){
		    false,
		    str_concatv(a, 5, str("Expected: "),
				exptype_string(actual.type), str("\nActual: "),
				exptype_string(EXP_COND),
				str("\n Expression is not conditional."))};
	ConditionalExp actual_condition = actual._conditional;
	if (!str_eq(actual_condition.condition->_infix.left->_ident.value,
		    str("x")))
		return (TestResult){
		    false,
		    str_concatv(
			a, 5, str("Expected: "),
			actual_condition.condition->_infix.left->_ident.value,
			str("\nActual: "), str("x"),
			str("\n Left expressions don't match"))};

	if (!str_eq(actual_condition.condition->_infix.operator, str("<")))
		return (TestResult){
		    false,
		    str_concatv(a, 5, str("Expected: "),
				actual_condition.condition->_infix.operator,
				str("\nActual: "), str("<"),
				str("\n Operators don't match"))};

	if (!str_eq(actual_condition.condition->_infix.right->_ident.value,
		    str("y")))
		return (TestResult){
		    false,
		    str_concatv(
			a, 5, str("Expected: "),
			actual_condition.condition->_infix.right->_ident.value,
			str("\nActual: "), str("y"),
			str("\n Right expressions don't match"))};
	StmtList *consequence_block = actual_condition.consequence->statements;
	for (StmtNode *cursor = consequence_block->head; cursor;
	     cursor = cursor->next) {
		if (!str_eq(cursor->s._exp._ident.value, str("x")))
			return (TestResult){
			    false,
			    str("Consequence block statement[0] is not 'x'")};
	}
	if (actual_condition.alternative != NULL)
		return (TestResult){false, str("Alternative not null\n")};
	return pass();
}

TestResult function_literal_tests(Arena *a)
{
	Lexer *lexer = lex_init(a, str("fn(x,y) { x + y;}"));
	Parser *parser = parser_init(a, lexer);
	Program prog = parse_program(parser);
	if (parser->errors && parser->errors->len > 0) {
		strlist_print(parser->errors);
		return (TestResult){false, str("Parsing has errors")};
	}
	Statement stmt = prog.statements->head->s;
	if (stmt._exp.type != EXP_FN)
		return (TestResult){
		    false,
		    str_concatv(
			a, 3, str("Got statement of type "),
			exptype_string(stmt._exp.type),
			str("\nStatement is not a function expression."))};
	FunctionLiteral func = stmt._exp._fn;
	if (func.params->len != 2)
		return (TestResult){
		    false,
		    str_fmt(
			a, "Expected: %lu\nActual: %lu\nWrong number of params",
			func.params->len, 2UL)};
	if (!str_eq(func.params->head->id.value, str("x")))
		return (TestResult){
		    false,
		    str_concatv(a, 4, str("Expected: x"), str("\nActual: "),
				func.params->head->id.value,
				str("\nWrong first parameter"))};
	if (!str_eq(func.params->tail->id.value, str("y")))
		return (TestResult){
		    false,
		    str_concatv(a, 4, str("Expected: y"), str("\nActual: "),
				func.params->head->id.value,
				str("\nWrong second parameter"))};

	BlockStatement *body = func.body;
	if (body->statements->len != 1)
		return (TestResult){false,
				    str_fmt(a,
					    "Expected: %lu\nActual: %lu\nWrong "
					    "number of statements in body",
					    1UL, body->statements->len)};

	if (body->statements->head->s._exp.type != EXP_INFIX)
		return (TestResult){
		    false,
		    str_concatv(
			a, 3, str("Got statement of type "),
			exptype_string(body->statements->head->s._exp.type),
			str("\nStatement is not an infix expression."))};

	InfixExp ifix = body->statements->head->s._exp._infix;
	if (!str_eq(ifix.left->_ident.value, str("x")))
		return (TestResult){false,
				    str_concatv(a, 3, str("Left is not x"),
						str("\nGot: "),
						ifix.left->_ident.value)};

	if (!str_eq(ifix.operator, str("+")))
		return (TestResult){false,
				    str_concatv(a, 3, str("Operator is not +"),
						str("\nGot: "), ifix.operator)};
	if (!str_eq(ifix.right->_ident.value, str("y")))
		return (TestResult){false,
				    str_concatv(a, 3, str("Right is not y"),
						str("\nGot: "),
						ifix.right->_ident.value)};
	return pass();
}

TestResult function_call_tests(Arena *a)
{
	Lexer *lexer = lex_init(a, str("suma(5 * 3, 2, 1 + 10)"));
	Parser *parser = parser_init(a, lexer);
	Program prog = parse_program(parser);

	if (parser->errors && parser->errors->len > 0) {
		strlist_print(parser->errors);
		return (TestResult){false, str("Parsing has errors")};
	}

	Statement stmt = prog.statements->head->s;
	if (stmt.type != STMT_EXP)
		return (TestResult){
		    false,
		    str_concatv(
			a, 3, str("Got statement of type "),
			stmttype_string(stmt.type),
			str("\nStatement is not an ExpressionStatement."))};

	CallExp call_exp = stmt._exp._call;
	if (call_exp.function->type != EXP_IDENT)
		return (TestResult){
		    false,
		    str_concatv(a, 3, str("Got an expression of type "),
				exptype_string(call_exp.function->type),
				str("\nFunction token is not an identifier"))};
	if (!str_eq(call_exp.function->_ident.value, str("suma")))
		return (TestResult){
		    false,
		    str_concatv(a, 3, str("Identifier name is: "),
				call_exp.function->_ident.value,
				str("\nFunction token is not an identifier"))};

	ExpNode *cursor = call_exp.arguments->head;
	if (cursor->ex.type != EXP_INFIX)
		return (TestResult){
		    false,
		    str_concatv(a, 3, str("Got an expression of type "),
				exptype_string(cursor->ex.type),
				str("\nExpected first argument to be infix"))};
	cursor = cursor->next;
	if (cursor->ex.type != EXP_INT)
		return (TestResult){
		    false,
		    str_concatv(
			a, 3, str("Got an expression of type "),
			exptype_string(cursor->ex.type),
			str("\nExpected second argument to be int literal"))};
	cursor = cursor->next;
	if (cursor->ex.type != EXP_INFIX)
		return (TestResult){
		    false,
		    str_concatv(a, 3, str("Got an expression of type "),
				exptype_string(cursor->ex.type),
				str("\nExpected third argument to be infix"))};
	return pass();
}
// Helpers
static i64 get_val(Expression ex)
{
	switch (ex.type) {
		case EXP_INT:
			return ex._int.value;
		case EXP_BOOL:
			return ex._bool.value;
		default:
			return 0;
	}
}
