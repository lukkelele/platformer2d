#include <bit>
#include <cstdint>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "core/enum.h"
#include "core/log.h"

namespace Test::Enum {
	enum class EColor : std::uint8_t
	{
		Red,
		Green,
		Blue,
		COUNT,
	};
	LK_ENUM(EColor);

	enum class EShape : std::uint16_t
	{
		Triangle = 4,
		Square = 8,
		Hex = 24,
		COUNT = 3,
	};
	LK_ENUM_RANGE(EShape, 4, 24);

	enum class EPermission : std::uint32_t
	{
		Read = LK_BIT(0),
		Write = LK_BIT(1),
		Execute = LK_BIT(2),
		Admin = LK_BIT(3),
		COUNT = 4,
	};
	LK_ENUM_FLAGS(EPermission);
}

using namespace platformer2d;
using namespace Test::Enum;

TEST_CASE("FuncSig prints the compiler signature", "[enum][funcsig]")
{
	LKLOG_PRINTLN("");
	using namespace platformer2d::Enum::Internal;

	/* clang-format off */
	LK_INFO_TAG("Enum", "--- Compiler signature for valid named values ---");
	LK_INFO_TAG("Enum", "FuncSig<EColor::Red>()        = {}", FuncSig<EColor::Red>());
	LK_INFO_TAG("Enum", "FuncSig<EColor::Green>()      = {}", FuncSig<EColor::Green>());
	LK_INFO_TAG("Enum", "FuncSig<EColor::Blue>()       = {}", FuncSig<EColor::Blue>());
	LK_INFO_TAG("Enum", "FuncSig<EShape::Triangle>()   = {}", FuncSig<EShape::Triangle>());
	LK_INFO_TAG("Enum", "FuncSig<EPermission::Admin>() = {}", FuncSig<EPermission::Admin>());

	LK_INFO_TAG("Enum", "--- Compiler signature for an unnamed cast value ---");
	LK_INFO_TAG("Enum", "FuncSig<(EColor)99>()         = {}", FuncSig<static_cast<EColor>(99)>());
	LK_INFO_TAG("Enum", "FuncSig<(EShape)7>()          = {}", FuncSig<static_cast<EShape>(7)>());

	REQUIRE(!FuncSig<EColor::Red>().empty());
	REQUIRE(!FuncSig<static_cast<EColor>(99)>().empty());
	/* clang-format on */
}

TEST_CASE("ParseValueName extracts identifier from funcsig", "[enum][parse]")
{
	LKLOG_PRINTLN("");
	using namespace Enum::Internal;

	LK_INFO_TAG("Enum", "-- EColor::Green --");
	{
		constexpr std::string_view Sig = FuncSig<EColor::Green>();
		constexpr std::string_view Name = ParseValueName(Sig);
		LK_INFO_TAG("Enum", "Input:    {}", Sig);
		LK_INFO_TAG("Enum", "Parsed:   '{}'", Name);
		REQUIRE(Name == "Green");
	}
	LKLOG_PRINTLN("");

	LK_INFO_TAG("Enum", "-- EShape::Hex --");
	{
		constexpr std::string_view Sig = FuncSig<EShape::Hex>();
		constexpr std::string_view Name = ParseValueName(Sig);
		LK_INFO_TAG("Enum", "Input:    {}", Sig);
		LK_INFO_TAG("Enum", "Parsed:   '{}'", Name);
		REQUIRE(Name == "Hex");
	}
	LKLOG_PRINTLN("");

	LK_INFO_TAG("Enum", "-- EColor::99 [Invalid] --");
	{
		constexpr std::string_view Sig = FuncSig<static_cast<EColor>(99)>();
		constexpr std::string_view Name = ParseValueName(Sig);
		LK_INFO_TAG("Enum", "Input:    {}", Sig);
		LK_INFO_TAG("Enum", "Parsed:   '{}' (empty -> not a valid named value)", Name);
		REQUIRE(Name.empty());
	}
}

TEST_CASE("ValueName composes FuncSig + ParseValueName", "[enum][value-name]")
{
	LKLOG_PRINTLN("");
	using namespace Enum::Internal;

	/* clang-format off */
	LK_INFO_TAG("Enum", "ValueName<EColor::Red>()   = '{}'", ValueName<EColor::Red>());
	LK_INFO_TAG("Enum", "ValueName<EColor::Green>() = '{}'", ValueName<EColor::Green>());
	LK_INFO_TAG("Enum", "ValueName<EColor::Blue>()  = '{}'", ValueName<EColor::Blue>());
	LK_INFO_TAG("Enum", "ValueName<(EColor)99>()    = '{}' (empty)", ValueName<static_cast<EColor>(99)>());

	STATIC_REQUIRE(ValueName<EColor::Red>()   == "Red");
	STATIC_REQUIRE(ValueName<EColor::Green>() == "Green");
	STATIC_REQUIRE(ValueName<EColor::Blue>()  == "Blue");
	STATIC_REQUIRE(ValueName<static_cast<EColor>(99)>().empty());
	STATIC_REQUIRE(IsNamed<EColor::Red>());
	STATIC_REQUIRE(!IsNamed<static_cast<EColor>(99)>());
	/* clang-format on */
}

TEST_CASE("LK_ENUM builds a contiguous table from 0..COUNT-1", "[enum][contiguous]")
{
	LKLOG_PRINTLN("");
	REQUIRE(Enum::Count<EColor>() == 3);

	/* clang-format off */
	REQUIRE(Enum::ToString(EColor::Red)   == "Red");
	REQUIRE(Enum::ToString(EColor::Green) == "Green");
	REQUIRE(Enum::ToString(EColor::Blue)  == "Blue");

	REQUIRE(Enum::FromString<EColor>("Red")           == EColor::Red);
	REQUIRE(Enum::FromString<EColor>("Green")         == EColor::Green);
	REQUIRE(Enum::FromString<EColor>("Blue")          == EColor::Blue);
	REQUIRE(Enum::FromString<EColor>("NotARealColor") == EColor::COUNT);
	/* clang-format on */

	LK_INFO_TAG("Enum", "--- EColor Table ---");
	for (const auto& [V, N] : Enum::View<EColor, Enum::Internal::Entry<EColor>>()) {
		LK_INFO_TAG("Enum", "  {:<6} = {}", N, std::to_underlying(V));
	}

	STATIC_REQUIRE(Enum::Count<EColor>() == 3);
	STATIC_REQUIRE(Enum::ToString(EColor::Green) == "Green");
	STATIC_REQUIRE(Enum::FromString<EColor>("Blue") == EColor::Blue);
}

TEST_CASE("LK_ENUM_RANGE only finds values within [Min, Max)", "[enum][range]")
{
	LKLOG_PRINTLN("");
	REQUIRE(Enum::Count<EShape>() == 3);

	/* clang-format off */
	REQUIRE(Enum::ToString(EShape::Triangle) == "Triangle");
	REQUIRE(Enum::ToString(EShape::Square)   == "Square");
	REQUIRE(Enum::ToString(EShape::Hex)      == "Hex");

	REQUIRE(Enum::FromString<EShape>("Triangle") == EShape::Triangle);
	REQUIRE(Enum::FromString<EShape>("Square")   == EShape::Square);
	REQUIRE(Enum::FromString<EShape>("Hex")      == EShape::Hex);
	REQUIRE(Enum::FromString<EShape>("Pentagon") == EShape::COUNT);
	/* clang-format on */

	LK_INFO_TAG("Enum", "--- EShape Table (scanned over [0, 32)) ---");
	for (const auto& [V, N] : Enum::View<EShape, Enum::Internal::Entry<EShape>>()) {
		LK_INFO_TAG("Enum", "  {:<10} = {}", N, std::to_underlying(V));
	}

	const auto Span = Enum::View<EShape>();
	REQUIRE(Span.size() == 3);
	for (const auto& Entry : Span) {
		REQUIRE(Entry != "COUNT");
	}
}

TEST_CASE("LK_ENUM_FLAGS finds named power-of-two values", "[enum][flags]")
{
	LKLOG_PRINTLN("");
	REQUIRE(Enum::Count<EPermission>() == 4);

	/* clang-format off */
	REQUIRE(Enum::ToString(EPermission::Read)    == "Read");
	REQUIRE(Enum::ToString(EPermission::Write)   == "Write");
	REQUIRE(Enum::ToString(EPermission::Execute) == "Execute");
	REQUIRE(Enum::ToString(EPermission::Admin)   == "Admin");

	REQUIRE(Enum::FromString<EPermission>("Read")  == EPermission::Read);
	REQUIRE(Enum::FromString<EPermission>("Admin") == EPermission::Admin);
	REQUIRE(Enum::FromString<EPermission>("Nope")  == EPermission::COUNT);
	/* clang-format on */

	LK_INFO_TAG("Enum", "--- EPermission Table (over bits 0..31) ---");
	for (const auto& [V, N] : Enum::View<EPermission, Enum::Internal::Entry<EPermission>>()) {
		const auto U = std::to_underlying(V);
		LK_INFO_TAG("Enum", "  {:<8} = bit {:<2} (value {:#x})", N, std::countr_zero(U), U);
	}
}

TEST_CASE("ToString and FromString round-trip", "[enum][roundtrip]")
{
	LKLOG_PRINTLN("");
	for (const auto& [V, N] : Enum::View<EColor, Enum::Internal::Entry<EColor>>()) {
		const auto S = Enum::ToString(V);
		const auto Back = Enum::FromString<EColor>(S);
		LK_INFO_TAG("Enum", "EColor:      {:<5} -> '{}' -> {}", std::to_underlying(V), S, std::to_underlying(Back));
		REQUIRE(Back == V);
	}

	LKLOG_PRINTLN("");
	for (const auto& [V, N] : Enum::View<EShape, Enum::Internal::Entry<EShape>>()) {
		const auto S = Enum::ToString(V);
		const auto Back = Enum::FromString<EShape>(S);
		LK_INFO_TAG("Enum", "EShape:      {:<5} -> '{}' -> {}", std::to_underlying(V), S, std::to_underlying(Back));
		REQUIRE(Back == V);
	}

	LKLOG_PRINTLN("");
	for (const auto& [V, N] : Enum::View<EPermission, Enum::Internal::Entry<EPermission>>()) {
		const auto S = Enum::ToString(V);
		const auto Back = Enum::FromString<EPermission>(S);
		LK_INFO_TAG("Enum", "EPermission: {:<5} -> '{}' -> {}", std::to_underlying(V), S, std::to_underlying(Back));
		REQUIRE(Back == V);
	}
}

TEST_CASE("Tables live in static storage and are evaluated at compile time", "[enum][constexpr]")
{
	/* clang-format off */
	STATIC_REQUIRE(Enum::Count<EColor>()      == 3);
	STATIC_REQUIRE(Enum::Count<EShape>()      == 3);
	STATIC_REQUIRE(Enum::Count<EPermission>() == 4);

	STATIC_REQUIRE(Enum::ToString(EColor::Red)        == "Red");
	STATIC_REQUIRE(Enum::ToString(EShape::Hex)        == "Hex");
	STATIC_REQUIRE(Enum::ToString(EPermission::Admin) == "Admin");

	STATIC_REQUIRE(Enum::FromString<EColor>("Blue")       == EColor::Blue);
	STATIC_REQUIRE(Enum::FromString<EShape>("Triangle")   == EShape::Triangle);
	STATIC_REQUIRE(Enum::FromString<EPermission>("Write") == EPermission::Write);

	STATIC_REQUIRE(Enum::FromString<EColor>("Bogus")      == EColor::COUNT);
	STATIC_REQUIRE(Enum::FromString<EShape>("Bogus")      == EShape::COUNT);
	STATIC_REQUIRE(Enum::FromString<EPermission>("Bogus") == EPermission::COUNT);
	/* clang-format on */
}
