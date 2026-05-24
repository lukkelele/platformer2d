#include "test.h"

#include "core/log.h"

namespace platformer2d::test {

	namespace {
		constexpr bool TEST_INIT = false;
	}

	CTest::CTest(const int Argc, char* Argv[])
		: CTestBase(Argc, Argv, TEST_INIT)
	{
		lklog::init(lklog::level::trace);
	}

	void CTest::Run()
	{
		bRunning = true;
		const int CatchResult = Catch::Session().run(Args.Argc, Args.Argv);
		LK_INFO_TAG("Test", "Catch result: {}", CatchResult);
	}

	void CTest::Destroy()
	{
		LK_DEBUG_TAG("Test", "Destroy");
	}

}
