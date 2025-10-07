#pragma once

#include "test_base.h"

namespace platformer2d::test {

	class CTest : public CTestBase
	{
	public:
		CTest(int Argc, char* Argv[]) 
			: CTestBase(Argc, Argv) 
		{
		}
		virtual ~CTest() override {}

		virtual void Run() override {}
		virtual void Destroy() override {}
	};

}
