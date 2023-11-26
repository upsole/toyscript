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

compile_demo()
{
	$cc $args main.c -o demo.out
}

case $1 in
	r|run)
		compile;
		[[ $? -eq 0 ]] && ./toyscript;
		[[ $? -eq 0 ]] && vim-local-rc;;
	*)
		compile_demo;
		[[ $? -eq 0 ]] && ./demo.out;
		[[ $? -eq 0 ]] && vim-local-rc;;
esac
