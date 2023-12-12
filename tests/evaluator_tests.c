#include "tests.h"

TestResult test_integer_eval(Arena *a);
TestResult test_string_eval(Arena *a);
TestResult test_string_concat(Arena *a);
TestResult test_array_eval(Arena *a);
TestResult test_index_eval(Arena *a);
TestResult test_bang_eval(Arena *a);
TestResult test_cond_eval(Arena *a);
TestResult test_return_eval(Arena *a);
TestResult test_errors(Arena *a);
TestResult test_val_eval(Arena *a);
TestResult test_function_eval(Arena *a);
TestResult test_function_call(Arena *a);
TestResult test_builtin_function(Arena *a);
TestResult test_assignment(Arena *a);
TestResult test_while_loop(Arena *a);
TestResult test_arr_concat(Arena *a);

int main(int ac, char **av)
{
	Arena *a = arena(MB(1));
	u64 fail_count = 0;
	Test tests[] = {
			{str("INTEGERS"), &test_integer_eval},
			{str("STRINGS"), &test_string_eval},
			{str("STRING CONCAT"), &test_string_concat},
			{str("ARRAY"), &test_array_eval},
			{str("INDEX"), &test_index_eval},
			{str("BANG"), &test_bang_eval},
			{str("CONDITIONALS"), &test_cond_eval},
			{str("RETURNS"), &test_return_eval},
			{str("ERRORS"), &test_errors},
			{str("VALS"), &test_val_eval},
			{str("FUNCTION"), &test_function_eval},
			{str("CALL"), &test_function_call},
			{str("BUILTIN"), &test_builtin_function},
			{str("ASSIGNMENT"), &test_assignment},
			{str("WHILE"), &test_while_loop},
			{str("WHILE"), &test_arr_concat},
	};

	if (ac < 2) {
		for (int i = 0; i < arrlen(tests); i++) {
			str_print(str_fmt(a, "TEST EVALUATOR %d: ", i));
			if (!run_test(a, tests[i]))
				fail_count++, str_print(str("\tTEST EVALUATOR FAILURE\n"));
		}
		str_print(str_fmt(a, "\tTEST EVALUATOR summary: (%lu/%lu)\n\n",
					arrlen(tests) - fail_count, arrlen(tests)));
	}
	for (int i = 1; i < ac; i++) {
		int test_id = atoi(av[i]);
		if (test_id >= arrlen(tests))
			str_print(str_fmt(a, "Test %d not found.\n", test_id)), exit(1);
		if (!run_test(a, tests[test_id]))
			str_print(str("\t TEST EVALUATOR FAILURE\n"));
	}
	arena_free(&a);
}

Element eval_wrapper(Arena *a, String input);
TestResult test_integer_eval(Arena *a)
{
	struct int_input {
		String input;
		i64 expected;
	};
	struct int_input tests[] = {
	    {str("15"), 15},	    {str("-15"), -15},
	    {str("-0"), 0},	    	{str("2 + 3"), 5},
	    {str("2 * 5 + 3"), 13}, {str("2 * (5 + 3)"), 16},
	    {str("2 > 5"), 0},	    {str("2 < 5"), 1},
	    {str("3 == 3"), 1},	    {str("3 == 5"), 0},
	    {str("3 != 5"), 1},
	};
	for (int i = 0; i < arrlen(tests); i++) {
		Element res = eval_wrapper(a, tests[i].input);
		if (TEST(res.INT != tests[i].expected))
			return fail(CONCAT(a, str_fmt(a, "Value mismatch got %ld expected %ld", res.INT, tests[i].expected)));
	}
	return pass();
}

TestResult test_string_eval(Arena *a)
{
	struct str_input {
		String input;
		String expected;
	};
	struct str_input tests[] = {
	    {str("\"Hiya Worldo\""), str("Hiya Worldo")}};
	for (int i = 0; i < arrlen(tests); i++) {
		Element res = eval_wrapper(a, tests[i].input);
		if (TEST(res.type != STR))
			return fail(str("Wrong type"));
		if (TEST(!str_eq(res.STR, tests[i].expected)))
			return fail(str("Value mismatch"));
	}
	return pass();
}

TestResult test_string_concat(Arena *a)
{
	struct str_input {
		String input;
		String expected;
	};
	struct str_input tests[] = {
	    {str("\"Hiya\" + \" \" + \"Worldo\""), str("Hiya Worldo")}};
	for (int i = 0; i < arrlen(tests); i++) {
		Element res = eval_wrapper(a, tests[i].input);
		if (TEST(res.type != STR))
			return fail(str("Wrong type"));

		if (TEST(!str_eq(res.STR, tests[i].expected)))
			return fail(str("Value mismatch"));
	}
	return pass();
}

TestResult test_array_eval(Arena *a)
{
	String input = str("[1, 3 * 5, 7 + 3]");
	i64 expected[] = {1, 15, 10};
	Element res = eval_wrapper(a, input);
	if (TEST(res.type != ARRAY)) 
		return fail(str("Wrong type"));

	if (TEST(res.ARRAY->len != 3))
		return fail(str("Wrong number of elements"));
	for (int i = 0; i < res.ARRAY->len; i++) {
		if (TEST(res.ARRAY->items[i].INT != expected[i]))
			return fail(str("Value mismatch"));
	}
	return pass();
}

TestResult test_index_eval(Arena *a)
{
	struct test {
		String input;
		Element expected;
	};
	struct test tests[] = {
	    {str("[1, 3, 4][0]"), { INT, .INT = 1}},
	    {str("[2, false, 5][1]"), { BOOL, .BOOL = false}},
	    {str("[1, 3, 4][-1]"), { ELE_NULL }},
	    {str("[1, 3, 4][3]"), { ELE_NULL }},
		{str("[\"string1\", 0, false][0]"), { STR, .STR = str("string1") }},
		{str("var x = [1, 3, 7]; x[0]"), { INT, .INT = 1 }},
	    {str("var x = [2, false, 5]; x[1]"), { BOOL, .BOOL = false}},
	    {str("var x = [1, 3, 4]; x[-1]"), { ELE_NULL }},
	    {str("var x = [1, 3, 4]; x[3]"), { ELE_NULL }},
		{str("var x = [\"string1\", 0, false]; x[0]"), { STR, .STR = str("string1") }}
	};
	for (int i = 0; i < arrlen(tests); i++) {
		Element res = eval_wrapper(a, tests[i].input);
		if (TEST(res.type != tests[i].expected.type))
			return fail(str("Wrong type"));
		if (res.type == INT) {
			if (TEST(res.INT != tests[i].expected.INT))
				return fail(str("Value mismatch"));
		}
		if (res.type == BOOL) {
			if (TEST(res.BOOL != tests[i].expected.BOOL))
				return fail(str("Value mismatch"));
		}
		if (res.type == STR) {
			if (TEST(!str_eq(res.STR, tests[i].expected.STR)))
				return fail(str("Value mismatch"));
		}
	}
	return pass();
}

TestResult test_bang_eval(Arena *a)
{
	struct int_input {
		String input;
		i64 expected;
	};
	struct int_input tests[] = {
					{str("!false"), 1},
				    {str("!!false"), 0},
				    {str("!0"), 1},
				    {str("!5"), 0},
				    {str("!!5"), 1},
				    {str("false == false"), 1},
				    {str("true == false"), 0},
				    {str("true != false"), 1}
	};
	for (int i = 0; i < arrlen(tests); i++) {
		Element res = eval_wrapper(a, tests[i].input);
		if (TEST(res.type != BOOL))
			return fail(str("Wrong type"));
		if (TEST(res.BOOL != tests[i].expected))
			return fail(str("Value mismatch"));
	}
	return pass();
}

TestResult test_cond_eval(Arena *a)
{
	struct elem_input {
		String input;
		Element expected;
		ElementType expected_type;
	};
	Element expected_10 = {.type = INT, .INT = 10};
	Element expected_20 = {.type = INT, .INT = 20};
	Element null = {.type = ELE_NULL};
	struct elem_input tests[] = {
	    {str("if (true) { 10 }"), expected_10, INT},
	    {str("if (false) { 10 }"), null, ELE_NULL},
	    {str("if (2 > 3) { 10 } else { 20 }"), expected_20, INT},
	};
	for (int i = 0; i < arrlen(tests); i++) {
		Element res = eval_wrapper(a, tests[i].input);
		if (TEST(res.type != tests[i].expected_type))
			return fail(str("Wrong type"));
		if (res.type == INT) {
			if (TEST(res.INT != tests[i].expected.INT))
				return fail(str("Value mismatch"));
		}
	}
	return pass();
}

TestResult test_return_eval(Arena *a)
{
	struct elem_input {
		String input;
		Element expected;
		ElementType expected_type;
	};
	Element expected10 = {.type = INT, .INT = 10};
	char *nested_return = "if (3 < 10) { if (7 < 10) {return 1;} return 11; }";
	struct elem_input tests[] = {
					{str("return 10;"), expected10, INT},
				    {str("return false;"), {.type = BOOL, .BOOL = false}, BOOL},
				    {str("return ;"), { ELE_NULL }, ELE_NULL},
				    {cstr(nested_return), {.type = INT, .INT = 1}, INT} 
	};
	for (int i = 0; i < arrlen(tests); i++) {
		Element res = eval_wrapper(a, tests[i].input);
		if (TEST(res.type != tests[i].expected_type))
			return fail(str("Wrong type"));
		if (res.type == INT) {
			if (TEST(res.INT != tests[i].expected.INT))
				return fail(str("Value mismatch"));
		}
		if (res.type == BOOL) {
			if (TEST(res.BOOL != tests[i].expected.BOOL))
				return fail(str("Value mismatch"));
		}
		arena_reset(a);
	}
	return pass();
}

TestResult test_errors(Arena *a)
{
	struct error_test {
		String input;
		String expected_msg;
	};
	struct error_test tests[] = {
	    {str("5 + false;"), str("Invalid types in operation: INT+BOOL")},
	    {str("-true;"), str("Invalid operation: -BOOL")},
	    {str("true < false;"), str("Invalid operation: BOOL<BOOL")},
	    {str("\"String 1\" - \"String \""), str("Invalid operation: STR-STR")},
	    {str("if (1 < 10) { if (1 < 10) { false - 5; } 5; }"), str("Invalid types in operation: BOOL-INT")},
	    {str("len(\"asdf\", \"asdf\")"), str("Wrong number of args for len: got 2, expected 1")},
	    {str("len(5)"), str("Type error: len called with argument of type: INT")}
	};
	for (int i = 0; i < arrlen(tests); i++) {
		Element res = eval_wrapper(a, tests[i].input);
		if (TEST(res.type != ERR))
			return fail(str("Wrong type"));
		if (TEST(!str_eq(res.ERR, tests[i].expected_msg)))
			return fail(str("Wrong error message"));
	}
	return pass();
}

TestResult test_val_eval(Arena *a)
{
	struct val_test {
		String input;
		i64 expected;
	};
	struct val_test tests[] = {
	    {str("val x = 5; x;"), 5},
	    {str("val x = 5 * 3; x;"), 15},
	    {str("val x = 7; val y = x; y"), 7},
	    {str("val x = 10; val y = x; val z = x + y; z"), 20}};
	for (int i = 0; i < arrlen(tests); i++) {
		Element res = eval_wrapper(a, tests[i].input);
		if (TEST(res.type != INT))
			return fail(str("Wrong type"));
		if (res.INT != tests[i].expected)
			return fail(str("Value mismatch"));
	}
	return pass();
}

AST *ast_alloc(Arena *a, AST node);
ASTList *astlist(Arena *a);
void astpush(ASTList *l, AST *ast);
TestResult test_function_eval(Arena *a)
{
	Element res = eval_wrapper(a, str("fn(x) { x + 2; };"));
	ASTList *exp_params = astlist(a);
	astpush(exp_params, ast_alloc(a, (AST) { AST_IDENT, .AST_STR = str("x")}));

	ASTList *exp_body = astlist(a);
	astpush(exp_body, ast_alloc(a, (AST) { AST_INFIX, .AST_INFIX = {
				.left = ast_alloc(a, (AST) { AST_IDENT, .AST_STR = str("x") }),
				.op = str("+"),
				.right = ast_alloc(a, (AST) { AST_INT, .AST_INT = {2} })
				}}));
	Element expected = (Element) {
		FUNCTION, .FUNCTION = {.params = exp_params, .body = exp_body}
	};

	if (TEST(res.type != FUNCTION))
		return fail(str("Wrong type"));
	if (TEST(!astlist_eq(res.FUNCTION.params, expected.FUNCTION.params)));
	if (TEST(!astlist_eq(res.FUNCTION.body, expected.FUNCTION.body)));
	return pass();
}

TestResult test_function_call(Arena *a)
{
	struct test {
		String input;
		i64 expected;
	};

	char *multiline = "val nest_sum = fn(z) {"
			  "fn(v) { z + v; };"
			  "};"
			  "val sum_two = nest_sum(2);"
			  "sum_two(5);";
	struct test tests[] = {
	    {str("val id = fn(x) { return x; }; id(3)"), 3},
	    {str("val sum = fn(x, y) { return x + y; } sum(3, 4);"), 7},
	    {str("val square = fn(x) { x * x; } square(7);"), 49},
	    {cstr(multiline), 7}
	};

	for (int i = 0; i < arrlen(tests); i++) {
		Element res = eval_wrapper(a, tests[i].input);
		if (TEST(res.type != INT)) {
			return fail(str("Wrong type"));
		}
		if (TEST(res.INT != tests[i].expected))
			return fail(str("Value mismatch")); 
	}
	return pass();
}

ElemList *elemlist(Arena *a);
void elem_push(ElemList *l, Element el);
void element_print(Arena *a, Element el);
TestResult test_builtin_function(Arena *a)
{
	struct test_int {
		String input;
		i64 expected;
	};
	struct test_int tests[] = {
				{str("len(\"\")"), 0},
				{str("len(\"ola\")"), 3},
				{str("len(\"ola ola\")"), 7}
	};
	for (int i = 0; i < arrlen(tests); i++) {
		Element res = eval_wrapper(a, tests[i].input);
		if (TEST(res.type != INT))
			return fail(str("Wrong type"));
		if (TEST(res.INT != tests[i].expected))
			return fail(str("Value mismatch"));
	}
	return pass();
}

TestResult test_assignment(Arena *a)
{
	struct test {
		String input;
		Element expected;
	};
	struct test tests[] = {
		{str("var x = 5; x = 10; x;"), (Element) { INT, .INT = 10 }},
		{str("var x = false; x = true; x;"), (Element) { BOOL, .BOOL = true }},
		{str("val x = 5; x = 10; x"), (Element) { ERR, .ERR = str("x binding is not mutable") }},
	};
	for (int i = 0; i < arrlen(tests); i++) {
		Element res = eval_wrapper(a, tests[i].input);
		if (TEST(res.type != tests[i].expected.type))
			return fail(str("Different result type"));
		if (res.type == INT) {
			if (TEST(res.INT != tests[i].expected.INT))
				return fail(str("Value mismatch"));
		}
		if (res.type == BOOL) {
			if (TEST(res.BOOL != tests[i].expected.BOOL))
				return fail(str("Value mismatch"));
		}
		if (res.type == ERR) {
			if (TEST(!str_eq(res.ERR, tests[i].expected.ERR)))
				return fail(str("Value mismatch"));
		}
	}
	return pass();
}

priv bool elem_eq(Element e1, Element e2);
priv bool elemlist_eq(ElemList *l1, ElemList *l2)
{
	if (l1->len != l2->len)
		return false;
	for (ElemNode *n1 = l1->head, *n2 = l2->head; 
			n1 && n2; n1 = n1->next, n2 = n2->next)
		if (!elem_eq(n1->element, n2->element)) return false;
	return true;
}

priv bool elemarray_eq(ElemArray *a1, ElemArray *a2)
{
	if (a1->len != a2->len)
		return false;
	for (int i = 0; i < a1->len; i++)
		if (!elem_eq(a1->items[i], a2->items[i]))
			return false;
	return true;
}

priv bool elem_eq(Element e1, Element e2)
{
	if (e1.type != e2.type) return false;
	switch (e1.type) {
		case ELE_NULL: return true;
		case INT: return e1.INT == e2.INT;
		case BOOL: return e1.BOOL == e2.BOOL;
		case ERR: case STR: return str_eq(e1.STR, e2.STR);
		case TYPE: return e1.TYPE == e2.TYPE;
		case ARRAY: return elemarray_eq(e1.ARRAY, e2.ARRAY);
		case LIST: return elemlist_eq(e1.LIST, e2.LIST);
		case RETURN: return elem_eq(*e1.RETURN.value, *e2.RETURN.value);
		case FUNCTION: return astlist_eq(e1.FUNCTION.params, e2.FUNCTION.params) 
				&& astlist_eq(e1.FUNCTION.body, e1.FUNCTION.body);
		case BUILTIN: return false;
	}
	return (NEVER(1 && "Type slipped through switch"));
}

ElemArray *elemarray(Arena *a, int len);
TestResult test_arr_concat(Arena *a)
{
	ElemArray *arr1 = elemarray(a, 3);
	arr1->items[0] = (Element) { INT, .INT = 0 };
	arr1->items[1] = (Element) { INT, .INT = 1 };
	arr1->items[2] = (Element) { INT, .INT = 2 };

	ElemArray *arr2nested = elemarray(a, 3);
	arr2nested->items[0] = (Element) { INT, .INT = 0 };
	arr2nested->items[1] = (Element) { INT, .INT = 1 };
	arr2nested->items[2] = (Element) { INT, .INT = 2 };
	ElemArray *arr2 = elemarray(a, 1);
	arr2->items[0] = (Element) { ARRAY, .ARRAY = arr2nested };

	struct {
		String input;
		Element expected;
	} tests[] = {
		{ str("[0] + [1, 2];"), (Element) {ARRAY, .ARRAY = arr1 } },
		{ str("[] + [];"), (Element) {ARRAY, .ARRAY = elemarray(a, 0) } },
		{ str("[] + [[0, 1, 2]];"), (Element) {ARRAY, .ARRAY = arr2 } },
	};
	for (int i = 0; i < arrlen(tests); i++) {
		Element res = eval_wrapper(a, tests[i].input);
		if (TEST(res.type != tests[i].expected.type)) 
			return fail(str("Type mismatch"));
		if (TEST(!elem_eq(res, tests[i].expected)))
			return fail(str("Value mismatch"));
	}
	return pass();
}

TestResult test_while_loop(Arena *a)
{
	struct {
		String input;
		Element expected;
	} tests[] = {
		{ str("var i = 0; while (i < 10) { i = i + 1; } i;"), (Element) { INT, .INT = 10 }},
		{ str("var i = 0; var j = 5; while (i < 10) { var j = i; i = i + 1; j = j + 2; } j;"), (Element) { INT, .INT = 5 }},
		{ str("var i = 0; while (i < 10 { i = i + 1; }"), (Element) { ELE_NULL }}, // Parser error expected
	};

	for (int i = 0; i < arrlen(tests); i++) {
		Element res = eval_wrapper(a, tests[i].input);
		if (res.type == ELE_NULL && tests[i].expected.type == ELE_NULL)
			return pass();
		if (res.type == ELE_NULL && !tests[i].expected.type == ELE_NULL)
			return fail(str(""));
		if (res.TYPE == INT) {
			if(TEST(res.INT != tests[i].expected.INT))
				return fail(str("Value mismatch"));
		}
	}
	return pass();
}

/* // Helpers */
Element eval_wrapper(Arena *a, String input)
{
	Lexer *l = lexer(a, input);
	Parser *p = parser(a, l);
	Namespace *ns = ns_create(a, 16);
	AST *prog = parse_program(p);
	if (p->errors) {
		parser_print_errors(p);
		return (Element) { ELE_NULL };
	}
	return eval(a, ns, prog);
}
