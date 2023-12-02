#!/usr/bin/env bash
### 
cflags="-Wall -Werror -Wimplicit-fallthrough "
dev="-Wno-unused-function -Wno-unused-value -g3"
src="base.c lexer.c ast.c evaluator.c"
exit_on_fail=""
args="$cflags $dev $src -DDEV_DEBUG"
cc=gcc
### 

compile()
{
	$cc $args toyscript.c -o toyscript
}

compile_tests_lexer()
{
	$cc $args $exit_on_fail tests/lexer_tests.c tests/test_utils.c -o tests/lexer_tests.out;
}

compile_tests_parser()
{
	$cc $args $exit_on_fail tests/parser_tests.c tests/test_utils.c -o tests/parser_tests.out;
}

compile_tests_evaluator()
{
	$cc $args $exit_on_fail tests/evaluator_tests.c tests/test_utils.c -o tests/evaluator_tests.out;
}

compile_demo()
{
	$cc $args main.c -o demo.out
}

test_launcher()
{
	case $1 in
		l|lexer)
			case $2 in
				no_assert|na)
					exit_on_fail="-DNO_EXIT_ON_FAIL";
					compile_tests_lexer;
					[[ $? -eq 0 ]] && ./tests/lexer_tests.out ${@:3};;
				*)
					compile_tests_lexer;
					[[ $? -eq 0 ]] && ./tests/lexer_tests.out ${@:2};;
			esac;;
		p|parser)
			case $2 in
				no_assert|na)
					exit_on_fail="-DNO_EXIT_ON_FAIL";
					compile_tests_parser;
					[[ $? -eq 0 ]] && ./tests/parser_tests.out ${@:3};;
				*)
					compile_tests_parser;
					[[ $? -eq 0 ]] && ./tests/parser_tests.out ${@:2};;
			esac;;
		e|eval)
			case $2 in
				no_assert|na)
					exit_on_fail="-DNO_EXIT_ON_FAIL";
					compile_tests_evaluator;
					[[ $? -eq 0 ]] && ./tests/evaluator_tests.out ${@:3};;
				*)
					compile_tests_evaluator;
					[[ $? -eq 0 ]] && ./tests/evaluator_tests.out ${@:2};;
			esac;;
		*)
			case $1 in
				no_assert|na)
					exit_on_fail="-DNO_EXIT_ON_FAIL";;
			esac
			compile_tests_lexer;
			[[ $? -eq 0 ]] && ./tests/lexer_tests.out;
			compile_tests_parser;
			[[ $? -eq 0 ]] && ./tests/parser_tests.out;
			compile_tests_evaluator;
			[[ $? -eq 0 ]] && ./tests/evaluator_tests.out;;
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
