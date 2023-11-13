#include "toyscript.h"
#include "eval.h"

int repl();
int exec_file(char *filename);

int main(int ac, char **av)
{
	if (ac < 2) {
		return repl();
	}
	return exec_file(av[1]);
}

int exec_file(char *filename)
{
	Arena *a = arena_init(MB(1));
	String file = str_read_file(a, filename);
	Lexer *lexer = lex_init(a, file);
	Parser *parser = parser_init(a, lexer);
	Statement prog = parse_program_stmt(parser);
	if (parser->errors) {
		printf("! Parsing errors:\n");
		strlist_print(parser->errors);
		return 1;
	}
	Environment env = {a, ns_create(a, 16)};
	Element res = eval(&env, prog);
	if (res.type == ELE_ERR) {
		str_print(res._error.msg);
		str_print(str("\n"));
		return 1;
	}
	return (res.type == ELE_INT) ? res._int.value : 0;
}

String read_input(Arena *a);
int repl()
{
	String input = {0};
	Arena *a = arena_init(MB(4));
	Lexer *lex;
	Parser *parser;
	Statement prog = {0};
	Environment env = {.arena = a, .namespace = ns_create(a, 16)};

	printf("TOYSCRIPT REPL~\n");
	while (1) {
		str_print(str("|> "));
		fflush(stdout);
		input = read_input(a);
		if (str_eq(input, str("exit\n"))) break;
		lex = lex_init(a, input);
		parser = parser_init(a, lex);
		prog = parse_program_stmt(parser);
		if (parser->errors) {
			printf("! Parsing errors:\n");
			strlist_print(parser->errors);
		} else {
			element_print(a, eval(&env, prog));
			str_print(str("\n"));
		}
		/* arena_reset(a); XXX Notice how coupling arena to namespace
		 * prevents us from releasing here... What if namespace gets
		 * it's own arena (can also grow table in that case)' */
	}
	arena_free(&a);
	printf("Bye!\n");
	return 0;
}

String read_input(Arena *a)
{
	char *stdin_buf = arena_alloc(a, KB(1));
	String input = {0};
	read(0, stdin_buf, KB(1));
	input = str_c(stdin_buf);
	return input;
}
