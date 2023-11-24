#include "base.h"
#include "toyscript.h"
#include <unistd.h>
int repl();
int main(void)
{
	return repl();
}


String read_stdin(Arena *a);
priv void print_tokens(Arena *a, Lexer *l);
int repl()
{
	String input = {0};
	Lexer	*l;
	Token tok = {0};

	Arena	*a = arena(MB(4));

	str_print(str("-TOYSCRIPT REPL-\n"));

	while (1) {
		str_print(str("~ "));
		input = read_stdin(a);
		if (str_eq(input, str("exit"))) break;
		l = lexer(a, input);
		print_tokens(a, l);
		arena_reset(a);
	}
	return 0;
}

priv void print_tokens(Arena *a, Lexer *l)
{
	Token tk = lexer_token(l);
	while (tk.type != END) {
		str_print(CONCAT(a, str("|"), tk.lit, 
					str(" "),
					token_str(tk.type), str("|\n")));
		tk = lexer_token(l);
	}
}

String read_stdin(Arena *a)
{
	char *stdin_buf = arena_alloc_zero(a, KB(1));
	String input = {0};
	read(0, stdin_buf, KB(1));
	input = cstr(stdin_buf);
	return input;
}
