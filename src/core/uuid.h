#pragma once

#include <cstddef>
#include <random>

#include "core/core.h"

namespace platformer2d {

	struct LUUID
	{
		using SizeType = std::uint64_t;

		LUUID();
		LUUID(const SizeType InUUID);
		LUUID(const LUUID&) = default;

		operator std::uint64_t() const { return UUID; }

	private:
		static SizeType Generate()
		{
			static std::random_device RandomDevice;
			static std::mt19937_64 RandomEngine(RandomDevice());
			static std::uniform_int_distribution<SizeType> UniformDistribution(
				1,
				std::numeric_limits<SizeType>::max());
			return UniformDistribution(RandomEngine);
		}

	private:
		std::uint64_t UUID = 0;
	};

}

namespace std {
	template<typename T>
	struct hash;

	template<>
	struct hash<platformer2d::LUUID>
	{
		std::size_t operator()(const platformer2d::LUUID& UUID) const
		{
			return static_cast<platformer2d::LUUID::SizeType>(UUID);
		}
	};
}

template<>
struct lklog::fmt::formatter<platformer2d::LUUID> : lklog::fmt::formatter<std::string>
{
	template<typename FormatContext>
	auto format(const platformer2d::LUUID UUID, FormatContext& Context) const
	{
		return lklog::fmt::format_to(Context.out(), "{}", static_cast<platformer2d::LUUID::SizeType>(UUID));
	}
};

