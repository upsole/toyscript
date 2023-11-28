#ifndef TESTS_H
# define TESTS_H
# ifndef EXIT_ON_FAIL
# define EXIT_ON_FAIL 0
# include "../base.h"
# include "../toyscript.h"
# include <stdlib.h>
# endif

typedef struct TestResult {
	bool	passed;
	String 	msg;
} TestResult;
typedef TestResult (*TestPtr)(Arena *a);
typedef struct Test {
	String	title;
	TestPtr fn;
}	Test;

bool run_test(Arena *arena, Test test);
TestResult pass();
TestResult fail(String msg);

// 	Helpers
#endif 
