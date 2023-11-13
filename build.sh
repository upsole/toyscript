CFLAGS="-Wall -Werror -g3 -Wno-unused-function"
SRC="base.c lexer.c parser.c ast.c stdout.c eval.c"
CC="clang"

compile_tests_parser()
{
	${CC} ${CFLAGS} ${SRC} tests/parser_tests.c tests/test_utils.c -o tests/test_toyscript
}

compile_tests_lexer()
{
	${CC} ${CFLAGS} ${SRC} tests/lexer_tests.c tests/test_utils.c -o tests/test_toyscript
}


compile_tests_eval()
{
	${CC} ${CFLAGS} ${SRC} tests/eval_tests.c tests/test_utils.c -o tests/test_toyscript
}

run_all_tests()
{
	compile_tests_lexer;
	[[ $? -eq 0 ]] && ./tests/test_toyscript
	compile_tests_parser;
	[[ $? -eq 0 ]] && ./tests/test_toyscript
	compile_tests_eval;
	[[ $? -eq 0 ]] && ./tests/test_toyscript
}

compile_main()
{
	${CC} ${CFLAGS} ${SRC} toyscript.c -o toyscript
}

case ${1} in
	parser|p)
		compile_tests_parser
		[[ $? -eq 0 ]] && ./tests/test_toyscript ${@:2};;
	lexer|l)
		compile_tests_lexer
		[[ $? -eq 0 ]] && ./tests/test_toyscript ${@:2};;
	eval|e)
		compile_tests_eval
		[[ $? -eq 0 ]] && ./tests/test_toyscript ${@:2};;
	tests|t)
		run_all_tests;;
	*)
		compile_main;;
esac
