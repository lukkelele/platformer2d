#pragma once

#include "core/core.h"

namespace platformer2d {

	struct LUUID
	{
		using SizeType = uint64_t;

		LUUID();
		LUUID(const SizeType InUUID);
		LUUID(const LUUID&) = default;

		operator uint64_t() const { return UUID; }

	private:
		uint64_t UUID = 0;
	};

}

namespace std 
{
	template<typename T> 
	struct hash;

	template<>
	struct hash<::platformer2d::LUUID>
	{
		std::size_t operator()(const ::platformer2d::LUUID& uuid) const
		{
			return (uint64_t)uuid;
		}
	};
}

template<>
struct LK_FMT_LIB::formatter<platformer2d::LUUID> : LK_FMT_LIB::formatter<std::string>
{
	template<typename FormatContext>
    auto format(const platformer2d::LUUID UUID, FormatContext& Context) const
    {
        return LK_FMT_LIB::format_to(Context.out(), "{}", static_cast<platformer2d::LUUID::SizeType>(UUID));
    }
};
