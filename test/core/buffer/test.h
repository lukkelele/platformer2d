#pragma once

#include "test_base.h"

namespace platformer2d::test {

	class CTest : public CTestBase
	{
	public:
		CTest(int Argc, char* Argv[]);
		~CTest() override {}

		void Run() override;
		void Destroy() override;
	};

}
