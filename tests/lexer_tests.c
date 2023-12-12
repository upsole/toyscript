#include "tests.h"
TestResult all_tokens_test(Arena *arena);
TestResult skip_comments_test(Arena *arena);

int main(int ac, char **av)
{
	Arena *a = arena(MB(1));
	u64 fail_count = 0;

	Test tests[] = {{str("TEST ALL TOKENS"), &all_tokens_test},
			{str("IGNORE COMMENTS"), &skip_comments_test}};
	if (ac < 2) {
		for (int i = 0; i < arrlen(tests); i++) {
			str_print(str_fmt(a, "TEST LEXER %d: ", i));
			if (!run_test(a, tests[i]))
				fail_count++, str_print(str("\tTEST LEXER FAILURE\n"));
		}
		str_print(str_fmt(a, "\tTEST LEXER summary: (%lu/%lu)\n\n",
					arrlen(tests) - fail_count, arrlen(tests)));
	}
	for (int i = 1; i < ac; i++) {
		int test_id = atoi(av[i]);
		if (test_id >= arrlen(tests))
			str_print(str_fmt(a, "Test %d not found.\n", test_id)), exit(1);
		if (!run_test(a, tests[test_id]))
			str_print(str("\t TEST LEXER FAILURE\n"));
	}
	arena_free(&a);
}

TestResult all_tokens_test(Arena *arena)
{

	Token expected[] = {
		{TK_NIL, str("NIL")}, {TK_WHILE, str("while")},
		{TK_SEMICOLON, str(";")}, {TK_COMMA, str(",")},
		{TK_BANG, str("!")}, {TK_STAR, str("*")},
		{TK_GT, str(">")}, {TK_LT, str("<")},
		{TK_LPAREN, str("(")}, {TK_RPAREN, str(")")},
		{TK_LBRACE, str("{")}, {TK_RBRACE, str("}")},
		{TK_LBRACKET, str("[")}, {TK_RBRACKET, str("]")},
		{TK_PLUS, str("+")},	{TK_MINUS, str("-")},
		{TK_ASSIGN, str("=")}, {TK_EQ, str("==")},
		{TK_NOT_EQ, str("!=")}, {TK_INT, str("20")},
	    {TK_IDENT, str("name")},	{TK_FN, str("fn")},
	    {TK_VAL, str("val")},	{TK_VAR, str("var")},
	    {TK_RETURN, str("return")}, {TK_IF, str("if")},
	    {TK_ELSE, str("else")},	{TK_TRUE, str("true")},
	    {TK_FALSE, str("false")}, {TK_STRING, str("This is a string")}
	};

	Lexer *l = lexer(
	    arena, str("NIL while;,!*><(){}[]+-= == != 20 name fn val var return if else "
		       "true false \"This is a string\""));
	Token t = lexer_token(l);
	int i = 0;
	while (t.type != TK_END) {
		if (TEST(i >= arrlen(expected)))
			return fail(str("End reached before expected"));
		if (TEST(expected[i].type != t.type)) {
			return fail(CONCAT(arena, 
						str("Expected: "), token_str(t.type),
						str("\nActual: "), token_str(t.type),
						str("\nToken type missmatch")
						));
		}
		if (TEST(!str_eq(expected[i].lit, t.lit))) {
			return fail(CONCAT(arena,
						str("Expected: "), expected[i].lit,
						str("\nActual: "), t.lit,
						str("\nToken literal missmatch")
						));
		}
		t = lexer_token(l);
		i++;
	}
	return pass();
}

TestResult skip_comments_test(Arena *arena)
{
	Lexer *l =
	    lexer(arena, str("val x = 5;# Line comment here\nprint(x);"));

	Token expected[] = {
		{TK_VAL, str("val")}, {TK_IDENT, str("x")},
		{TK_ASSIGN, str("=")}, {TK_INT, str("5")},
		{TK_SEMICOLON, str(";")}, {TK_IDENT, str("print")},
		{TK_LPAREN, str("(")}, {TK_IDENT, str("x")},
		{TK_RPAREN, str(")")}, {TK_SEMICOLON, str(";")}
	};
	Token t = lexer_token(l);
	int i = 0;
	while (t.type != TK_END) {
		if (TEST(i >= arrlen(expected))) 
			return fail(str("End reached before expected"));
		if (TEST(expected[i].type != t.type)) {
			return fail(CONCAT(arena, 
						str("Expected: "), token_str(t.type),
						str("\nActual: "), token_str(t.type),
						str("\nToken type missmatch")
						));
		}
		if (TEST(!str_eq(expected[i].lit, t.lit))) {
			return fail(CONCAT(arena,
						str("Expected: "), expected[i].lit,
						str("\nActual: "), t.lit,
						str("\nToken literal missmatch")
						));
		}
		t = lexer_token(l);
		i++;
	}
	return pass();
}
