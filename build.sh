#!/usr/bin/env bash
### 
cflags="-Wall -Werror -Wimplicit-fallthrough "
dev="-Wno-unused-variable -Wno-unused-function -Wno-unused-value -g3"
src="base.c lexer.c ast.c evaluator.c"
args="$cflags $dev $src -DDEV_DEBUG"
cc=gcc
### 

compile()
{
	$cc $args toyscript.c -o toyscript
}

compile_tests_lexer()
{
	$cc $args tests/lexer_tests.c tests/test_utils.c -o tests/lexer_tests.out
}

# compile_tests_parser()
# {
# }

# compile_tests_eval()
# {
# }

compile_demo()
{
	$cc $args main.c -o demo.out
}

test_launcher()
{
	case $1 in
		l|lexer)
			compile_tests_lexer;
			[[ $? -eq 0 ]] && ./lexer_tests.out ${@:2};;
		*)
			compile_tests_lexer;
			[[ $? -eq 0 ]] && ./tests/lexer_tests.out;
			# compile_tests_parser;
			# [[ $? -eq 0 ]] && ./tests/parser_test;
			# compile_tests_eval;
			# [[ $? -eq 0 ]] && ./tests/eval_test;;
	esac
}

case $1 in
	t|test) 
		test_launcher ${@:2};;
	demo)
		compile_demo;
		[[ $? -eq 0 ]] && ./demo.out;
		[[ $? -eq 0 ]] && vim-local-rc;;
	r|run)
		compile;
		[[ $? -eq 0 ]] && ./toyscript ${@:2};
		[[ $? -eq 0 ]] && vim-local-rc;;
	"")
		compile;;
	*)
		echo "Read build.sh for available commands";;
esac
