#include "tests.h"
bool fail(char *msg)
{
	printf("%s", msg);
	if (EXIT_ON_FAIL) exit(1);
	return true;
}

bool run_test(Arena *arena, Test test)
{
	str_print(test.title);
	TestResult res = (*test.fn)(arena);
	if (!res.passed) {
		str_print(str(" KO\n"));
		str_print(res.msg);
		str_print(str("\n"));
		return false;
	} else {
		str_print(str(" OK\n"));
		return true;
	}
	arena_reset(arena);
}

TestResult pass()
{
	return (TestResult){true, str("")};
}
