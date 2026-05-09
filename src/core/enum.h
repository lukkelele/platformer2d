#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>

#include "core.h"
#include "macros.h"
#include "platform.h"

/**
 * Most work here is thanks to magic_enum.
 * @link: https://github.com/Neargye/magic_enum
 */

namespace platformer2d::Enum::Internal {
	enum class ERangeKind : std::uint8_t
	{
		Contiguous,
		Range,
		Flags,
	};

	template<typename E>
	struct Range
	{
		static constexpr bool Defined = false;
	};

	template<auto V>
	consteval std::string_view FuncSig()
	{
		return LK_FUNCSIG;
	}

	consteval std::string_view ParseValueName(std::string_view Sig)
	{
#ifdef LK_COMPILER_MSVC
		const std::size_t End = Sig.rfind('>');
		if (End == std::string_view::npos) {
			return {};
		}

		std::size_t Start = End;
		std::size_t Depth = 1;
		while (Start > 0) {
			Start--;
			if (Sig[Start] == '>') {
				Depth++;
			} else if (Sig[Start] == '<') {
				if (--Depth == 0) {
					break;
				}
			}
		}

		if (Sig[Start] != '<') {
			return {};
		}
		std::string_view Inner = Sig.substr(Start + 1, End - Start - 1);
#else
		const std::size_t End = Sig.rfind(']');
		if (End == std::string_view::npos) {
			return {};
		}
		const std::size_t Eq = Sig.rfind(" = ", End);
		if (Eq == std::string_view::npos) {
			return {};
		}
		std::string_view Inner = Sig.substr(Eq + 3, End - Eq - 3);
#endif

		while (!Inner.empty() && (Inner.back() == ' ')) {
			Inner.remove_suffix(1);
		}
		if (Inner.empty()) {
			return {};
		}

		const char C = Inner.front();
		if ((C == '(') || (C == '-') || ((C >= '0') && (C <= '9'))) {
			return {};
		}

		const std::size_t Colons = Inner.rfind("::");
		if (Colons == std::string_view::npos) {
			return {};
		}

		return Inner.substr(Colons + 2);
	}

	template<auto V>
	consteval std::string_view ValueName()
	{
		return ParseValueName(FuncSig<V>());
	}

	template<auto V>
	consteval bool IsNamed()
	{
		return !ValueName<V>().empty();
	}

	template<typename E>
	struct Entry
	{
		E Value{};
		std::string_view Name{};
	};

	template<typename E, std::size_t Capacity>
	struct Storage
	{
		std::array<Entry<E>, Capacity> Entries{};
		std::size_t Size = 0;

		constexpr void Push(const E V, const std::string_view N)
		{
			if (Size < Capacity) {
				Entries[Size] = {V, N};
				Size++;
			}
		}
	};

	template<typename E, std::size_t... Is>
	consteval auto BuildContiguous(std::index_sequence<Is...>)
	{
		Storage<E, sizeof...(Is)> S{};
		(S.Push(static_cast<E>(Is), ValueName<static_cast<E>(Is)>()), ...);
		return S;
	}

	template<typename E, std::int64_t Offset, std::size_t Capacity, std::size_t... Is>
	consteval auto BuildRange(std::index_sequence<Is...>)
	{
		Storage<E, Capacity> S{};
		auto Try = [&S]<std::int64_t I>()
		{
			constexpr std::string_view Name = ValueName<static_cast<E>(I)>();
			if constexpr (!Name.empty() && (Name != std::string_view("COUNT"))) {
				S.Push(static_cast<E>(I), Name);
			}
		};
		(Try.template operator()<Offset + static_cast<std::int64_t>(Is)>(), ...);
		return S;
	}

	template<typename E, std::size_t Capacity, std::size_t... Is>
	consteval auto BuildFlags(std::index_sequence<Is...>)
	{
		using U = std::underlying_type_t<E>;
		Storage<E, Capacity> S{};
		auto Try = [&S]<std::size_t Bit>()
		{
			constexpr U Bits = static_cast<U>(U(1) << Bit);
			constexpr std::string_view Name = ValueName<static_cast<E>(Bits)>();
			if constexpr (!Name.empty() && (Name != std::string_view("COUNT"))) {
				S.Push(static_cast<E>(Bits), Name);
			}
		};
		(Try.template operator()<Is>(), ...);
		return S;
	}

	template<typename E>
		requires(Range<E>::Defined)
	struct Table
	{
		using U = std::underlying_type_t<E>;
		static constexpr std::size_t Count = static_cast<std::size_t>(E::COUNT);

		static consteval auto Make()
		{
			/* 'Kind' is implemented by the registration macro. */
			if constexpr (Range<E>::Kind == ERangeKind::Contiguous) {
				return BuildContiguous<E>(std::make_index_sequence<Count>{});
			} else if constexpr (Range<E>::Kind == ERangeKind::Range) {
				constexpr std::int64_t Min = Range<E>::Min;
				constexpr std::int64_t Max = Range<E>::Max;
				static_assert(Max > Min, "LK_ENUM_RANGE: Max must be greater than Min");
				constexpr std::size_t Span = static_cast<std::size_t>(Max - Min);
				return BuildRange<E, Min, Count>(std::make_index_sequence<Span>{});
			} else {
				constexpr std::size_t Bits = sizeof(U) * 8;
				return BuildFlags<E, Count>(std::make_index_sequence<Bits>{});
			}
		}

		static constexpr auto Data = Make();

		static constexpr std::string_view Find(const E Value) noexcept
		{
			for (std::size_t Idx = 0; Idx < Data.Size; Idx++) {
				if (Data.Entries[Idx].Value == Value) {
					return Data.Entries[Idx].Name;
				}
			}
			return {};
		}

		static constexpr E Find(const std::string_view Name) noexcept
		{
			for (std::size_t Idx = 0; Idx < Data.Size; Idx++) {
				if (Data.Entries[Idx].Name == Name) {
					return Data.Entries[Idx].Value;
				}
			}
			return E::COUNT;
		}
	};
}

namespace platformer2d::Enum {
	template<typename E>
		requires(Internal::Range<E>::Defined)
	[[nodiscard]] constexpr std::string_view ToString(const E Value) noexcept
	{
		return Internal::Table<E>::Find(Value);
	}

	template<typename E>
		requires(Internal::CountedEnum<E> && Internal::Range<E>::Defined)
	[[nodiscard]] constexpr E FromString(const std::string_view Name) noexcept
	{
		return Internal::Table<E>::Find(Name);
	}

	template<typename E>
		requires(Internal::Range<E>::Defined)
	[[nodiscard]] constexpr std::span<const Internal::Entry<E>> View() noexcept
	{
		using namespace Internal;
		return std::span<const Entry<E>>(Table<E>::Data.Entries.data(), Table<E>::Data.Size);
	}

	template<typename E>
		requires(Internal::Range<E>::Defined)
	[[nodiscard]] constexpr std::size_t Count() noexcept
	{
		return Internal::Table<E>::Data.Size;
	}
}

#define LK_ENUM(EnumType)                                                                                                          \
	template<>                                                                                                                     \
	struct ::platformer2d::Enum::Internal::Range<EnumType>                                                                         \
	{                                                                                                                              \
		static_assert(::std::is_enum_v<EnumType>, #EnumType " must be an enum");                                                   \
		static_assert(requires { EnumType::COUNT; }, #EnumType " requires COUNT for LK_ENUM");                                     \
		static constexpr bool Defined = true;                                                                                      \
		static constexpr ::platformer2d::Enum::Internal::ERangeKind Kind = ::platformer2d::Enum::Internal::ERangeKind::Contiguous; \
	}

#define LK_ENUM_RANGE(EnumType, MinValue, MaxValue)                                                                           \
	template<>                                                                                                                \
	struct ::platformer2d::Enum::Internal::Range<EnumType>                                                                    \
	{                                                                                                                         \
		static_assert(::std::is_enum_v<EnumType>, #EnumType " must be an enum");                                              \
		static_assert(requires { EnumType::COUNT; }, #EnumType " requires COUNT for LK_ENUM_RANGE");                          \
		static constexpr bool Defined = true;                                                                                 \
		static constexpr ::platformer2d::Enum::Internal::ERangeKind Kind = ::platformer2d::Enum::Internal::ERangeKind::Range; \
		static constexpr ::std::int64_t Min = static_cast<::std::int64_t>(MinValue);                                          \
		static constexpr ::std::int64_t Max = static_cast<::std::int64_t>(MaxValue);                                          \
	}

#define LK_ENUM_FLAGS(EnumType)                                                                                               \
	template<>                                                                                                                \
	struct ::platformer2d::Enum::Internal::Range<EnumType>                                                                    \
	{                                                                                                                         \
		static_assert(::std::is_enum_v<EnumType>, #EnumType " must be an enum");                                              \
		static_assert(requires { EnumType::COUNT; }, #EnumType " requires COUNT for LK_ENUM_FLAGS");                          \
		static constexpr bool Defined = true;                                                                                 \
		static constexpr ::platformer2d::Enum::Internal::ERangeKind Kind = ::platformer2d::Enum::Internal::ERangeKind::Flags; \
	}

