#ifndef TESTS_H
# define TESTS_H

# ifdef NO_EXIT_ON_FAIL
# 	define TEST(ex) (ex)
# else
# 	define TEST(ex) (assert(!(ex)), ex)
# endif

# include "../base.h"
# include "../toyscript.h"
# include <stdlib.h>

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

// COMPARATORS
bool astlist_eq(ASTList *l1, ASTList *l2);
bool ast_eq(AST *node1, AST *node2);
// 	Helpers
#endif 
