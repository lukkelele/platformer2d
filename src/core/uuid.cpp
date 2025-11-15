#include "uuid.h"

#include <random>

namespace platformer2d {

	namespace {
		std::random_device RandomDevice;
		std::mt19937_64 RandomEngine(RandomDevice());
		std::uniform_int_distribution<LUUID::SizeType> UniformDistribution;
	}

	LUUID::LUUID()
		: UUID(UniformDistribution(RandomEngine))
	{
		/* Never allow an UUID to be 0. */
		while (UUID == 0)
		{
			UUID = UniformDistribution(RandomEngine);
		}
	}

	LUUID::LUUID(const SizeType InUUID)
		: UUID(InUUID)
	{
	}

}
