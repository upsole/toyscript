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
			str_print(str_fmt(a, "Test %d not found.\n", i)), exit(1);
		if (!run_test(a, tests[test_id]))
			str_print(str("\t TEST LEXER FAILURE\n"));
	}
	arena_free(&a);
}

TestResult all_tokens_test(Arena *arena)
{

	Token expected[] = {
		{SEMICOLON, str(";")}, {COMMA, str(",")},
		{BANG, str("!")}, {STAR, str("*")},
		{GT, str(">")}, {LT, str("<")},
		{LPAREN, str("(")}, {RPAREN, str(")")},
		{LBRACE, str("{")}, {RBRACE, str("}")},
		{LBRACKET, str("[")}, {RBRACKET, str("]")},
		{PLUS, str("+")},	{MINUS, str("-")},
		{ASSIGN, str("=")}, {EQ, str("==")},
		{NOT_EQ, str("!=")}, {TK_INT, str("20")},
	    {TK_IDENT, str("name")},	{TK_FN, str("fn")},
	    {TK_VAL, str("val")},	{TK_VAR, str("var")},
	    {TK_RETURN, str("return")}, {IF, str("if")},
	    {ELSE, str("else")},	{TRUE, str("true")},
	    {FALSE, str("false")},	{TK_STRING, str("This is a string")}
	};

	Lexer *l = lexer(
	    arena, str(";,!*><(){}[]+-= == != 20 name fn val var return if else "
		       "true false \"This is a string\""));
	Token t = lexer_token(l);
	int i = 0;
	while (t.type != END) {
		if (i >= arrlen(expected))
			return fail(str("End reached before expected"));
		if (expected[i].type != t.type) {
			return fail(CONCAT(arena, 
						str("Expected: "), token_str(t.type),
						str("\nActual: "), token_str(t.type),
						str("\nToken type missmatch")
						));
		}
		if (!str_eq(expected[i].lit, t.lit)) {
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
		{ASSIGN, str("=")}, {TK_INT, str("5")},
		{SEMICOLON, str(";")}, {TK_IDENT, str("print")},
		{LPAREN, str("(")}, {TK_IDENT, str("x")},
		{RPAREN, str(")")}, {SEMICOLON, str(";")}
	};
	Token t = lexer_token(l);
	int i = 0;
	while (t.type != END) {
		if (i >= arrlen(expected)) 
			return fail(str("End reached before expected"));
		if (expected[i].type != t.type) {
			return fail(CONCAT(arena, 
						str("Expected: "), token_str(t.type),
						str("\nActual: "), token_str(t.type),
						str("\nToken type missmatch")
						));
		}
		if (!str_eq(expected[i].lit, t.lit)) {
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
