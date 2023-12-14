#include "base.h"
#include "toyscript.h"
#include <unistd.h>

priv int repl();
priv int exec_file(char *filename);
int main(int ac, char **av)
{
	if (ac < 2)
		return repl();
	return exec_file(av[1]);
}

priv int exec_file(char *filename)
{
	Arena *stdin_arena = arena(MB(1));
	String file = str_read_file(stdin_arena, filename);
	Lexer  *l = lexer(stdin_arena, file);

	Arena *program_arena = arena(GB(1));
	Parser *p = parser(program_arena, l);
	AST *program = parse_program(p);
	arena_free(&stdin_arena);

	Arena *bindings_arena = arena(MB(1));
	Namespace *bindings = ns_create(bindings_arena, 16);
	Element exit_elem = eval(program_arena, bindings, program);
	if (p->errors) {
		parser_print_errors(p);
		return 1;
	}
	if (exit_elem.type == ERR) {
		str_print(exit_elem.ERR), str_print(str("\n"));
		return 1;
	}
	return (exit_elem.type == INT) ? (i32)exit_elem.INT : 0;
}

priv String read_stdin(Arena *a);
priv int repl()
{
	String input = {0};
	Lexer	*l;
	Parser	*p;
	AST		*program;

	Arena	*stdin_arena = arena(MB(4));
	Arena	*program_arena = arena(GB(4));
	Arena	*namespace_arena = arena(GB(4));
	Namespace *ns = ns_create(namespace_arena, 16);

	str_print(str("-TOYSCRIPT REPL-\n"));

	while (1) {
		str_print(str("~ "));
		input = read_stdin(stdin_arena);
		if (str_eq(input, str("exit"))) break;
		l = lexer(stdin_arena, input);
		p = parser(program_arena, l);
		program = parse_program(p);
		Element result = eval(program_arena, ns, program);
		if (p->errors) 
			parser_print_errors(p);
		else
			str_print(to_string(stdin_arena, result)), str_print(str("\n"));
		arena_reset(stdin_arena);
	}
	return 0;
}

priv String read_stdin(Arena *a)
{
	char *stdin_buf = arena_alloc_zero(a, KB(1));
	String input = {0};
	read(0, stdin_buf, KB(1));
	input = cstr(stdin_buf);
	return input;
}
