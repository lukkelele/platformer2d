#include <stdio.h>

#include "test.h"

using namespace platformer2d;
using namespace platformer2d::test;

int main(int Argc, char* Argv[])
{
	{
		CTest Test(Argc, Argv);
		Test.Run();
		Test.Destroy();
	}

	LK_INFO_TAG("Main", "Exit: {}", LK_TEST_NAME);
	return 0;
}
