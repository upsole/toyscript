#include "tests.h"

TestResult test_integer_eval(Arena *a);
TestResult test_string_eval(Arena *a);
TestResult test_string_concat(Arena *a);
TestResult test_list_eval(Arena *a);
TestResult test_index_eval(Arena *a);
TestResult test_bang_eval(Arena *a);
TestResult test_cond_eval(Arena *a);
TestResult test_return_eval(Arena *a);
TestResult test_errors(Arena *a);
TestResult test_val_eval(Arena *a);
TestResult test_function_eval(Arena *a);
TestResult test_function_call(Arena *a);
TestResult test_builtin_function(Arena *a);

int main(int ac, char **av)
{
	Arena *arena = arena_init(MB(1));
	u64 fail_count = 0;
	Test tests[] = {{str("INTEGERS"), &test_integer_eval},
			{str("STRINGS"), &test_string_eval},
			{str("STRING CONCAT"), &test_string_concat},
			{str("LISTS"), &test_list_eval},
			{str("INDEX"), &test_index_eval},
			{str("BANG"), &test_bang_eval},
			{str("CONDITIONALS"), &test_cond_eval},
			{str("RETURNS"), &test_return_eval},
			{str("ERRORS"), &test_errors},
			{str("VALS"), &test_val_eval},
			{str("FUNCTION"), &test_function_eval},
			{str("CALL"), &test_function_call},
			{str("BUILTIN"), &test_builtin_function}};
	if (ac < 2) {
		for (int i = 0; i < arrlen(tests); i++) {
			str_print(str_fmt(arena, "TEST EVAL %d: ", i));
			if (!run_test(arena, tests[i]))
				fail_count++, fail("\tTEST EVAL FAILURE\n");
		}
		printf("\tTEST EVAL summary: (%lu/%lu)\n\n",
		       arrlen(tests) - fail_count, arrlen(tests));
	}
	for (int i = 1; i < ac; i++) {
		int test_id = atoi(av[i]);
		if (test_id >= arrlen(tests))
			printf("Test %d not found.\n", test_id), exit(1);
		;
		if (!run_test(arena, tests[test_id]))
			fail("\t TEST EVAL FAILURE\n");
	}
	arena_free(&arena);
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
	    {str("-0"), 0},	    {str("2 + 3"), 5},
	    {str("2 * 5 + 3"), 13}, {str("2 * (5 + 3)"), 16},
	    {str("2 > 5"), 0},	    {str("2 < 5"), 1},
	    {str("3 == 3"), 1},	    {str("3 == 5"), 0},
	    {str("3 != 5"), 1},
	};
	for (int i = 0; i < arrlen(tests); i++) {
		Element res = eval_wrapper(a, tests[i].input);
		if (res._int.value != tests[i].expected)
			return (TestResult){
			    false,
			    str_concatv(
				a, 3,
				str_fmt(a,
					"Value mismatch: got %ld expected %ld",
					res._int.value, tests[i].expected),
				str("\nInput: "), tests[i].input)};
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
		if (res.type != ELE_STR)
			return (TestResult){
			    false,
			    str_concatv(a, 5, str("Got type: "),
					type_str(res.type),
					str("\nExpected ELE_STR"),
					str("\nInput: "), tests[i].input)};
		if (!str_eq(res._string.value, tests[i].expected))
			return (TestResult){
			    false,
			    str_concatv(a, 5, str("Expected: "),
					tests[i].expected, str("\nActual: "),
					res._string.value,
					str("\nMismatch in value"))};
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
		if (res.type != ELE_STR)
			return (TestResult){
			    false,
			    str_concatv(a, 5, str("Got type: "),
					type_str(res.type),
					str("\nExpected ELE_STR"),
					str("\nInput: "), tests[i].input)};
		if (!str_eq(res._string.value, tests[i].expected))
			return (TestResult){
			    false,
			    str_concatv(a, 5, str("Expected: "),
					tests[i].expected, str("\nActual: "),
					res._string.value,
					str("\nMismatch in value"))};
	}
	return pass();
}

TestResult test_list_eval(Arena *a)
{
	String input = str("[1, 3 * 5, 7 + 3]");
	i64 expected[] = {1, 15, 10};
	Element res = eval_wrapper(a, input);
	if (res.type != ELE_LIST) {
		return (TestResult){false,
				    str_concatv(a, 5, str("Got type: "),
						type_str(res.type),
						str("\nExpected ELE_LIST"),
						str("\nInput: "), input)};
	}
	if (res._list.items->len != 3)
		return (TestResult){
		    false,
		    str_fmt(
			a,
			"Wrong number of list elements, expected: 3 got: %lu",
			res._list.items->len)};
	ElemNode *cursor = res._list.items->head;
	for (int i = 0; i < arrlen(expected); i++) {
		if (!cursor)
			return (TestResult){
			    false,
			    str("Actual list is shorter than expected list")};
		if (cursor->el._int.value != expected[i])
			return (TestResult){
			    false,
			    str_fmt(a,
				    "Expected: %ld\nActual: %ld\nValue "
				    "mismatch at list[%d]",
				    expected[i], cursor->el._int.value, i)};
		cursor = cursor->next;
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
	    {str("[1, 3, 4][0]"), {.type = ELE_INT, ._int.value = 1}},
	    {str("[2, false, 5][1]"), {.type = ELE_BOOL, ._bool.value = false}},
	    {str("[1, 3, 4][-1]"), {.type = ELE_NULL}},
	    {str("[1, 3, 4][3]"), {.type = ELE_NULL}}};
	for (int i = 0; i < arrlen(tests); i++) {
		Element res = eval_wrapper(a, tests[i].input);
		if (res.type != tests[i].expected.type)
			return (TestResult){
			    false,
			    str_concatv(a, 5, str("Expected: "),
					type_str(tests[i].expected.type),
					str("\nActual: "), type_str(res.type),
					str("\nType mismatch"))};
		if (res.type == ELE_INT) {
			if (res._int.value != tests[i].expected._int.value)
				return (TestResult){
				    false, str_fmt(a,
						   "Expected: %ld\nActual: "
						   "%ld\nValue mismatch",
						   tests[i].expected._int.value,
						   res._int.value)};
		}

		if (res.type == ELE_BOOL) {
			if (res._bool.value != tests[i].expected._bool.value)
				return (TestResult){
				    false,
				    str_fmt(a,
					    "Expected: %d\nActual: %d\nValue "
					    "mismatch",
					    tests[i].expected._bool.value,
					    res._bool.value)};
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
	struct int_input tests[] = {{str("!false"), 1},
				    {str("!!false"), 0},
				    {str("!0"), 1},
				    {str("!5"), 0},
				    {str("!!5"), 1},
				    {str("false == false"), 1},
				    {str("true == false"), 0},
				    {str("true != false"), 1}};
	for (int i = 0; i < arrlen(tests); i++) {
		Element res = eval_wrapper(a, tests[i].input);
		if (res.type != ELE_BOOL)
			return (TestResult){
			    false,
			    str_concatv(a, 5, str("Got type: "),
					type_str(res.type),
					str("\nExpected ELE_BOOL"),
					str("\nInput: "), tests[i].input)};
		if (res._bool.value != tests[i].expected)
			return (TestResult){
			    false,
			    str_concatv(
				a, 3,
				str_fmt(a,
					"Value mismatch: got %d expected %ld",
					res._int.value, tests[i].expected),
				str("\nInput: "), tests[i].input)};
	}
	return pass();
}

TestResult test_cond_eval(Arena *a)
{
	struct elem_input {
		String input;
		Element expected;
		ElemType expected_type;
	};
	Element expected_10 = {.type = ELE_INT, ._int.value = 10};
	Element expected_20 = {.type = ELE_INT, ._int.value = 20};
	Element null = {.type = ELE_NULL};
	struct elem_input tests[] = {
	    {str("if (true) { 10 }"), expected_10, ELE_INT},
	    {str("if (false) { 10 }"), null, ELE_NULL},
	    {str("if (2 > 3) { 10 } else { 20 }"), expected_20, ELE_INT},
	};
	for (int i = 0; i < arrlen(tests); i++) {
		Element res = eval_wrapper(a, tests[i].input);
		if (res.type != tests[i].expected_type)
			return (TestResult){
			    false,
			    str_concatv(a, 6, str("Got type: "),
					type_str(res.type), str("\nExpected: "),
					type_str(tests[i].expected_type),
					str("\nInput: "), tests[i].input)};

		if (res.type == ELE_INT) {
			if (res._int.value != tests[i].expected._int.value)
				return (TestResult){
				    false,
				    str_concatv(
					a, 3,
					str_fmt(a,
						"Value mismatch: got %ld "
						"expected %ld",
						res._int.value,
						tests[i].expected._int.value),
					str("\nInput: "), tests[i].input)};
		}
	}
	return pass();
}

TestResult test_return_eval(Arena *a)
{
	struct elem_input {
		String input;
		Element expected;
		ElemType expected_type;
	};
	struct Element expected10 = {.type = ELE_INT, ._int.value = 10};
	char *nested_return =
	    "if (3 < 10) { if (7 < 10) {return 1;} return 11; }";
	struct elem_input tests[] = {{str("return 10;"), expected10, ELE_INT},
				     {str("return false;"),
				      {.type = ELE_BOOL, ._bool.value = false},
				      ELE_BOOL},
				     {str_c(nested_return),
				      {.type = ELE_INT, ._int.value = 1},
				      ELE_INT}};
	for (int i = 0; i < arrlen(tests); i++) {
		Element res = eval_wrapper(a, tests[i].input);
		if (res.type != tests[i].expected_type)
			return (TestResult){
			    false,
			    str_concatv(a, 6, str("Got type: "),
					type_str(res.type), str("\nExpected: "),
					type_str(tests[i].expected_type),
					str("\nInput: "), tests[i].input)};
		if (res.type == ELE_INT) {
			if (res._int.value != tests[i].expected._int.value)
				if (res._int.value !=
				    tests[i].expected._int.value)
					return (TestResult){
					    false,
					    str_concatv(
						a, 3,
						str_fmt(
						    a,
						    "Value mismatch: got %ld "
						    "expected %ld",
						    res._int.value,
						    tests[i]
							.expected._int.value),
						str("\nInput: "),
						tests[i].input)};
		}
		if (res.type == ELE_BOOL) {
			if (res._bool.value != tests[i].expected._bool.value)
				return (TestResult){
				    false,
				    str_concatv(
					a, 3,
					str_fmt(a,
						"Value mismatch: got %d "
						"expected %d",
						res._bool.value,
						tests[i].expected._bool.value),
					str("\nInput: "), tests[i].input)};
		}
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
	    {str("\"String 1\" - \"String \""),
	     str("Invalid operation: STRING-STRING")},
	    {str("if (1 < 10) { if (1 < 10) { false - 5; } 5; }"),
	     str("Invalid types in operation: BOOL-INT")},
	    {str("len(\"asdf\", \"asdf\")"),
	     str("Wrong number of args for len (2), expected 1")},
	    {str("len(5)"),
	     str("Type error: len called with arg of type: INT")}};
	for (int i = 0; i < arrlen(tests); i++) {
		Element res = eval_wrapper(a, tests[i].input);
		if (res.type != ELE_ERR)
			return (TestResult){
			    false,
			    str_concatv(a, 5, str("Got type: "),
					type_str(res.type),
					str("\nExpected ELE_ERR"),
					str("\nInput: "), tests[i].input)};
		if (!str_eq(res._error.msg, tests[i].expected_msg))
			return (TestResult){
			    false,
			    str_concatv(
				a, 5, str("Expected: "), tests[i].expected_msg,
				str("\nActual: "), res._error.msg,
				str("\nError message is not matching"))};
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
		if (res.type != ELE_INT)
			return (TestResult){
			    false,
			    str_concatv(a, 5, str("Got type: "),
					type_str(res.type),
					str("\nExpected ELE_INT"),
					str("\nInput: "), tests[i].input)};
		if (res._int.value != tests[i].expected)
			return (TestResult){
			    false,
			    str_concatv(
				a, 3,
				str_fmt(a,
					"Value mismatch: got %ld expected %ld",
					res._int.value, tests[i].expected),
				str("\nInput: "), tests[i].input)};
	}
	return pass();
}

TestResult test_function_eval(Arena *a)
{
	Element res = eval_wrapper(a, str("fn(x) { x + 2; };"));
	if (res.type != ELE_FUNCTION)
		return (TestResult){
		    false,
		    str_concatv(a, 5, str("Got type: "), type_str(res.type),
				str("\nExpected ELE_FUNCTION"),
				str("\nInput: "), str("fn(x) { x + 2; };"))};
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
			  "sum_two(3);";

	struct test tests[] = {
	    {str("val id = fn(x) { return x; }; id(3)"), 3},
	    {str("val sum = fn(x, y) { return x + y; } sum(3, 4);"), 7},
	    {str("val square = fn(x) { x * x; } square(7);"), 49},
	    {str_c(multiline), 5}};

	for (int i = 0; i < arrlen(tests); i++) {
		Element res = eval_wrapper(a, tests[i].input);
		if (res.type != ELE_INT) {
			str_print(str("\n"));
			element_print(a, res);
			/* str_print(res.string(a, res)); */
			return (TestResult){
			    false,
			    str_concatv(a, 5, str("Got type: "),
					type_str(res.type),
					str("\nExpected ELE_INT"),
					str("\nInput: "), tests[i].input)};
		}
		if (res._int.value != tests[i].expected)
			return (TestResult){
			    false,
			    str_concatv(
				a, 3,
				str_fmt(a,
					"Value mismatch: got %ld expected %ld",
					res._int.value, tests[i].expected),
				str("\nInput: "), tests[i].input)};
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
	struct test_int tests[] = {{str("len(\"\")"), 0},
				   {str("len(\"ola\")"), 3},
				   {str("len(\"ola ola\")"), 7},
				   {str("car([10,3,1])"), 10}};
	for (int i = 0; i < arrlen(tests); i++) {
		Element res = eval_wrapper(a, tests[i].input);
		if (res.type != ELE_INT)
			return (TestResult){
			    false,
			    str_concatv(a, 5, str("Got type: "),
					type_str(res.type),
					str("\nExpected ELE_INT"),
					str("\nInput: "), tests[i].input)};
		if (res._int.value != tests[i].expected)
			return (TestResult){
			    false,
			    str_concatv(
				a, 3,
				str_fmt(a,
					"Value mismatch: got %ld expected %ld",
					res._int.value, tests[i].expected),
				str("\nInput: "), tests[i].input)};
	}

	struct test_elem {
		String input;
		Element expected;
	};
	ElemList *count5_list = elemlist(a);
	for (int i = 0; i < 5; i++)
		elem_push(count5_list,
			  (Element){.type = ELE_LIST, ._int.value = i});
	struct test_elem elem_tests[] = {
	    {str("push([0,1,2,3], 4)"),
	     {.type = ELE_LIST, ._list.items = count5_list}},
	    {str("cons([0,1], [2,3])"),
	     {.type = ELE_LIST, ._list.items = count5_list}},
	    {str("last([0, 1, 2, 3, 4])"), {.type = ELE_INT, ._int.value = 4}}};
	for (int i = 0; i < arrlen(elem_tests); i++) {
		Element res = eval_wrapper(a, elem_tests[i].input);
		if (res.type != elem_tests[i].expected.type)
			return (TestResult){false, str("Actual is not a list")};
		if (res.type == ELE_INT) {
			if (res._int.value != elem_tests[i].expected._int.value)
				return (TestResult){
				    false,
				    str_concatv(
					a, 3,
					str_fmt(
					    a,
					    "Value mismatch: got %ld expected "
					    "%ld",
					    res._int.value,
					    elem_tests[i].expected._int.value),
					str("\nInput: "), tests[i].input)};
		}
		if (res.type == ELE_LIST) {
			for (ElemNode *
				 actual = res._list.items->head,
				*exp = elem_tests[i].expected._list.items->head;
			     (actual && exp);
			     actual = actual->next, exp = exp->next) {
				if (actual->el._int.value != exp->el._int.value)
					return (TestResult){
					    false,
					    str_fmt(a, "Got %d, expected %d\n",
						    actual->el._int.value,
						    exp->el._int.value)};
			}
		}
	}
	return pass();
}

// Helpers
Element eval_wrapper(Arena *a, String input)
{
	Lexer *lexer = lex_init(a, input);
	Parser *parser = parser_init(a, lexer);
	Statement prog = parse_program_stmt(parser);
	Environment env = {.arena = a, .namespace = ns_create(a, 16)};
	fflush(stdout);
	if (parser->errors)
		printf("KO\n"), strlist_print(parser->errors),
		    fail("Parser has errors\n");
	return eval(&env, prog);
}
