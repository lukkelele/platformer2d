#pragma once

#include "test_base.h"

#ifndef LK_TEST_SUITE
#error "LK_TEST_SUITE missing"
#endif

namespace platformer2d::test {

	class CTest : public CTestBase
	{
	public:
		CTest(int Argc, char* Argv[]);
		virtual ~CTest() override {}

		virtual void Run() override;
		virtual void Destroy() override;
	};

}
