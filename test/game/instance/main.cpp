#include <stdio.h>
#include <filesystem>

#include <imgui/imgui.h>
#include <glm/glm.hpp>
#include <stb/stb_image.h>

#include "test.h"

#ifndef LK_TEST_SUITE
#error "LK_TEST_SUITE missing"
#endif

using namespace platformer2d;
using namespace platformer2d::test;

int main(int Argc, char* Argv[])
{
	spdlog::set_level(spdlog::level::debug);

	{
		CTest Test(Argc, Argv);
		Test.Run();
		Test.Destroy();
	}

	LK_INFO_TAG("Main", "Exit: {}", errno);
	return 0;
}