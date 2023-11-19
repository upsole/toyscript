#include "tests.h"
TestResult all_tokens_test(Arena *arena);
TestResult skip_comments_test(Arena *arena);

int main(int ac, char **av)
{
	Arena *arena = arena_init(MB(1));
	u64 fail_count = 0;
	Test tests[] = {{str("TEST ALL TOKENS"), &all_tokens_test},
			{str("IGNORE COMMENTS"), &skip_comments_test}};
	if (ac < 2) {
		for (int i = 0; i < arrlen(tests); i++) {
			str_print(str_fmt(arena, "TEST LEXER %d: ", i));
			if (!run_test(arena, tests[i]))
				fail_count++, fail("\tTEST LEXER FAILURE\n");
		}
		printf("\tTEST LEXER summary: (%lu/%lu)\n\n",
		       arrlen(tests) - fail_count, arrlen(tests));
	}
	for (int i = 1; i < ac; i++) {
		int test_id = atoi(av[i]);
		if (test_id >= arrlen(tests))
			printf("Test %d not found.\n", test_id), exit(1);
		;
		if (!run_test(arena, tests[test_id]))
			fail("\t TEST LEXER FAILURE\n");
	}
	arena_free(&arena);
}

TestResult all_tokens_test(Arena *arena)
{
	Token expected[] = {
	    tok(SEMICOLON, str(";")),	tok(COMMA, str(",")),
	    tok(BANG, str("!")),	tok(STAR, str("*")),
	    tok(GT, str(">")),		tok(LT, str("<")),
	    tok(LPAREN, str("(")),	tok(RPAREN, str(")")),
	    tok(LBRACE, str("{")),	tok(RBRACE, str("}")),
	    tok(LBRACKET, str("[")),	tok(RBRACKET, str("]")),
	    tok(PLUS, str("+")),	tok(MINUS, str("-")),
	    tok(ASSIGN, str("=")),	tok(EQ, str("==")),
	    tok(NOT_EQ, str("!=")),	tok(INT, str("20")),
	    tok(IDENT, str("name")),	tok(FN, str("fn")),
	    tok(RETURN, str("return")), tok(IF, str("if")),
	    tok(ELSE, str("else")),	tok(TRUE, str("true")),
	    tok(FALSE, str("false")),	tok(STRING, str("This is a string"))};

	Lexer *lexer = lex_init(
	    arena, str(";,!*><(){}[]+-= == != 20 name fn return if else "
		       "true false \"This is a string\""));
	Token t = lex_tok(lexer);
	int i = 0;
	while (t.type != END) {
		if (i >= arrlen(expected)) {
			return (TestResult){false,
					    str("End reached before expected")};
		}
		if (expected[i].type != t.type) {
			return (TestResult){
			    false,
			    str_concatv(arena, 5, str("Expected: "),
					tok_string(expected[i].type),
					str("\nActual: "), tok_string(t.type),
					str("\nToken type mismatch"))};
		}
		if (!str_eq(expected[i].lit, t.lit)) {
			return (TestResult){
			    false, str_concatv(
				       arena, 5, str("Expected: "),
				       expected[i].lit, str("\nActual: "),
				       t.lit, str("\nToken literal mismatch"))};
		}
		t = lex_tok(lexer);
		i++;
	}
	return pass();
}

TestResult skip_comments_test(Arena *arena)
{
	Lexer *lexer =
	    lex_init(arena, str("val x = 5;# Line comment here\nprint(x);"));
	Token expected[] = {tok(VAL, str("val")),     tok(IDENT, str("x")),
			    tok(ASSIGN, str("=")),    tok(INT, str("5")),
			    tok(SEMICOLON, str(";")), tok(IDENT, str("print")),
			    tok(LPAREN, str("(")),    tok(IDENT, str("x")),
			    tok(RPAREN, str(")")),    tok(SEMICOLON, str(";"))};
	Token t = lex_tok(lexer);
	int i = 0;
	while (t.type != END) {
		if (i >= arrlen(expected)) {
			return (TestResult){false,
					    str("End reached before expected")};
		}
		if (expected[i].type != t.type) {
			return (TestResult){
			    false,
			    str_concatv(arena, 5, str("Expected: "),
					tok_string(expected[i].type),
					str("\nActual: "), tok_string(t.type),
					str("\nToken type mismatch"))};
		}
		if (!str_eq(expected[i].lit, t.lit)) {
			return (TestResult){
			    false, str_concatv(
				       arena, 5, str("Expected: "),
				       expected[i].lit, str("\nActual: "),
				       t.lit, str("\nToken literal mismatch"))};
		}
		t = lex_tok(lexer);
		i++;
	}
	return pass();
}
