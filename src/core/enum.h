#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>

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

	/* clang-format off */
	/**
	 * Tags are used for sorting out issues with ADL (argument-dependent lookup) when
	 * using the LK_ENUM macros in other namespaces than platformer2d.
	 */
	struct UnregisteredTag {};
	struct ContiguousTag {};
	struct FlagsTag {};
	struct RangeTag
	{
		std::int64_t Min;
		std::int64_t Max;
	};

	template<typename T>
	consteval UnregisteredTag lk_enum_traits(T*) noexcept { return {}; }

	consteval ERangeKind TagToKind(UnregisteredTag) { return ERangeKind::Contiguous; }
	consteval ERangeKind TagToKind(ContiguousTag)   { return ERangeKind::Contiguous; }
	consteval ERangeKind TagToKind(RangeTag)        { return ERangeKind::Range; }
	consteval ERangeKind TagToKind(FlagsTag)        { return ERangeKind::Flags; }

	consteval std::int64_t TagMin(RangeTag T) { return T.Min; }
	template<typename T>
	consteval std::int64_t TagMin(T)          { return 0; } /* Unused for all tags except RangeTag. */

	consteval std::int64_t TagMax(RangeTag T) { return T.Max; }
	template<typename T>
	consteval std::int64_t TagMax(T)          { return 0; } /* Unused for all tags except RangeTag. */
	/* clang-format on */

	template<typename E>
	struct Range
	{
	private:
		/* The trait solves the ADL issue. */
		static constexpr auto Trait = lk_enum_traits(static_cast<E*>(nullptr));
		using TraitType = decltype(Trait);

	public:
		static constexpr bool Defined = !std::is_same_v<TraitType, UnregisteredTag>;
		static constexpr ERangeKind Kind = TagToKind(Trait);
		static constexpr std::int64_t Min = TagMin(Trait);
		static constexpr std::int64_t Max = TagMax(Trait);
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

		static consteval auto MakeNames()
		{
			std::array<std::string_view, Count> Arr{};
			for (std::size_t Idx = 0; Idx < Data.Size; Idx++) {
				Arr[Idx] = Data.Entries[Idx].Name;
			}
			return Arr;
		}
		static constexpr auto Names = MakeNames();

		static consteval auto MakeValues()
		{
			std::array<E, Count> Arr{};
			for (std::size_t Idx = 0; Idx < Data.Size; Idx++) {
				Arr[Idx] = Data.Entries[Idx].Value;
			}
			return Arr;
		}
		static constexpr auto Values = MakeValues();

		/** @brief Calculate largest name of all enum entries, used for the null-terminated array. */
		static consteval std::size_t CalcMaxNameLen() noexcept
		{
			std::size_t Max = 0;
			for (std::size_t Idx = 0; Idx < Data.Size; Idx++) {
				if (Data.Entries[Idx].Name.size() > Max) {
					Max = Data.Entries[Idx].Name.size();
				}
			}
			return Max;
		}
		static constexpr std::size_t MaxNameLen = CalcMaxNameLen();

		/** @brief Null-terminated enum names. */
		static consteval auto MakeNamesNT() noexcept
		{
			std::array<std::array<char, MaxNameLen + 1>, Count> Arr{};
			for (std::size_t I = 0; I < Data.Size; I++) {
				std::string_view Name = Data.Entries[I].Name;
				for (std::size_t J = 0; J < Name.size(); J++) {
					Arr[I][J] = Name[J];
				}
			}
			return Arr;
		}
		static constexpr auto NamesNT = MakeNamesNT();

		/**
		 * @todo: The for-loop should be replaced with a O(1) lookup.
		 * Not a problem right now as the enum entries rarely exceed a count
		 * of 5 but still, I'd like this to perform as good as possible.
		 * Should be possible to use arrays to store indices based on the range type
		 * and then map the indices with the enum values.
		 */
		static constexpr std::string_view Find(const E Value) noexcept
		{
			for (std::size_t Idx = 0; Idx < Data.Size; Idx++) {
				if (Data.Entries[Idx].Value == Value) {
					return Data.Entries[Idx].Name;
				}
			}
			return {};
		}

		/**
		 * @todo: The for-loop should be replaced with a O(1) lookup.
		 * Not a problem right now as the enum entries rarely exceed a count
		 * of 5 but still, I'd like this to perform as good as possible.
		 * Should be possible to use arrays to store indices based on the range type
		 * and then map the indices with the enum values.
		 */
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
	namespace Internal {
		template<typename T>
		concept CountedEnum = std::is_enum_v<T> && requires {
			T::COUNT;
		};

		/* Unused for now. The goal is to be able evaluate existance of Enum::ToString for enum T and that it is constexpr. */
		template<typename T>
		concept HasToString = std::is_enum_v<T> && requires(T Value) {
			{ ToString(Value) } -> std::convertible_to<std::string_view>;
			{ std::bool_constant<(ToString(T{}), true)>{} };
		};

		template<typename T>
		concept StringConvertable = CountedEnum<T>;

		template<CountedEnum T, std::predicate<T> Predicate>
		constexpr T FindIf(Predicate Pred)
		{
			for (std::underlying_type_t<T> Idx = 0; Idx < std::to_underlying(T::COUNT); Idx++) {
				const T Value = static_cast<T>(Idx);
				if (Pred(Value)) {
					return Value;
				}
			}
			return T::COUNT;
		}
	}

	template<typename T = std::string_view, typename E>
		requires(Internal::Range<E>::Defined)
	[[nodiscard]] constexpr auto ToString(const E Value) noexcept
	{
		static_assert(std::is_same_v<T, std::string_view> || std::is_same_v<T, const char*>, "Enum::ToString only supports std::string_view and const char*");
		if constexpr (std::is_same_v<T, std::string_view>) {
			return Internal::Table<E>::Find(Value);
		} else if constexpr (std::is_same_v<T, const char*>) {
			for (std::size_t Idx = 0; Idx < Internal::Table<E>::Data.Size; Idx++) {
				if (Internal::Table<E>::Data.Entries[Idx].Value == Value) {
					return static_cast<const char*>(Internal::Table<E>::NamesNT[Idx].data());
				}
			}
			return static_cast<const char*>(nullptr);
		}
	}

	template<typename E>
		requires(Internal::CountedEnum<E> && Internal::Range<E>::Defined)
	[[nodiscard]] constexpr E FromString(const std::string_view Name) noexcept
	{
		return Internal::Table<E>::Find(Name);
	}

	template<typename E, typename T = E>
		requires(Internal::Range<E>::Defined)
	[[nodiscard]] constexpr auto View() noexcept
	{
		using namespace Internal;
		if constexpr (std::is_same_v<T, E>) {
			return std::span<const E>(Table<E>::Values.data(), Table<E>::Data.Size);
		} else if constexpr (std::is_same_v<T, std::string_view>) {
			return std::span<const std::string_view>(Table<E>::Names.data(), Table<E>::Data.Size);
		} else if constexpr (std::is_same_v<T, const char*>) {
			/**
			 * @todo: The static storage for EnumNames breaks constexpr evaluation.
			 * It is possible to store this array similar to how NamesNT is stored but I
			 * fear the return type would require a const cast due to std::span element
			 * becoming 'const char* const'. And yeah, I don't like that.
			 * Might test it out sometime in the future, or just drop this constexpr dream of mine :)
			 */
			static std::array<const char*, Table<E>::Count> EnumNames = []() noexcept
			{
				std::array<const char*, Table<E>::Count> Arr{};
				for (std::size_t Idx = 0; Idx < Table<E>::Data.Size; Idx++) {
					Arr[Idx] = Table<E>::NamesNT[Idx].data();
				}
				return Arr;
			}();
			return std::span<const char*>(EnumNames.data(), Table<E>::Data.Size);
		} else if constexpr (std::is_same_v<T, Entry<E>>) {
			return std::span<const Entry<E>>(Table<E>::Data.Entries.data(), Table<E>::Data.Size);
		} else {
			static_assert(false, "Unsupported type T");
		}
	}

	template<typename E>
		requires(Internal::Range<E>::Defined)
	[[nodiscard]] constexpr std::size_t Count() noexcept
	{
		return Internal::Table<E>::Data.Size;
	}
}

/* clang-format off */
#define LK_ENUM(EnumType)                                                                  \
	static_assert(::std::is_enum_v<EnumType>, #EnumType " must be an enum");               \
	static_assert(requires { EnumType::COUNT; }, #EnumType " requires COUNT for LK_ENUM"); \
	[[maybe_unused]] consteval auto lk_enum_traits(EnumType*) noexcept                     \
		-> ::platformer2d::Enum::Internal::ContiguousTag { return {}; }

#define LK_ENUM_RANGE(EnumType, MinValue, MaxValue)                                              \
	static_assert(::std::is_enum_v<EnumType>, #EnumType " must be an enum");                     \
	static_assert(requires { EnumType::COUNT; }, #EnumType " requires COUNT for LK_ENUM_RANGE"); \
	[[maybe_unused]] consteval auto lk_enum_traits(EnumType*) noexcept                           \
		-> ::platformer2d::Enum::Internal::RangeTag                                              \
	{                                                                                            \
		return ::platformer2d::Enum::Internal::RangeTag{                                         \
			.Min = static_cast<::std::int64_t>(MinValue),                                        \
			.Max = static_cast<::std::int64_t>(MaxValue + 1),                                    \
		};                                                                                       \
	}

#define LK_ENUM_FLAGS(EnumType)                                                                  \
	static_assert(::std::is_enum_v<EnumType>, #EnumType " must be an enum");                     \
	static_assert(requires { EnumType::COUNT; }, #EnumType " requires COUNT for LK_ENUM_FLAGS"); \
	[[maybe_unused]] consteval auto lk_enum_traits(EnumType*) noexcept                           \
		-> ::platformer2d::Enum::Internal::FlagsTag { return {}; }
/* clang-format on */

