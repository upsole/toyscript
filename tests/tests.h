#ifndef TESTS_H
# define TESTS_H
# ifndef EXIT_ON_FAIL
# define EXIT_ON_FAIL 0
# include "../toyscript.h"
# include "../eval.h"
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

// 	Helpers
bool fail(char *msg);
#endif 
