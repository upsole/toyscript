#!/usr/bin/env bash
### 
cflags="-Wall -Werror -Wimplicit-fallthrough "
dev="-Wno-unused-variable -Wno-unused-function -g3"
src="base.c lexer.c"
args="$cflags $dev $src -DDEV_DEBUG"
### 

compile()
{
	gcc $args toyscript.c -o toyscript
}

compile_demo()
{
	gcc $args main.c -o demo.out
}

case $1 in
	r|run)
		compile;
		[[ $? -eq 0 ]] && ./toyscript;
		[[ $? -eq 0 ]] && vim-local-rc;;
	*)
		compile_demo;
		./demo.out;
		[[ $? -eq 0 ]] && vim-local-rc;;
esac
