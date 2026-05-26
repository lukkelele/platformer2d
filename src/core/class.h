#pragma once

#include <string_view>
#include <type_traits>

#include "platform.h"

namespace platformer2d {

	namespace Core {
		struct FClassInfo
		{
			std::string_view Name;
		};
	}

	namespace Core::Reflection {
		template<typename T>
		[[nodiscard]] constexpr std::string_view WrappedTypeName()
		{
			return LK_FUNCSIG;
		}

		[[nodiscard]] constexpr std::size_t WrappedPrefixLength()
		{
			return WrappedTypeName<void>().find("void");
		}

		[[nodiscard]] constexpr std::size_t WrappedSuffixLength()
		{
			return WrappedTypeName<void>().size() - WrappedPrefixLength() - std::string_view("void").size();
		}

		template<typename T>
		[[nodiscard]] constexpr std::string_view TypeName()
		{
			constexpr std::string_view Wrapped = WrappedTypeName<T>();
			constexpr std::size_t Prefix = WrappedPrefixLength();
			constexpr std::size_t Suffix = WrappedSuffixLength();
			std::string_view Name = Wrapped.substr(Prefix, Wrapped.size() - Prefix - Suffix);

			if (Name.starts_with("class ")) {
				Name.remove_prefix(std::string_view("class ").size());
			} else if (Name.starts_with("struct ")) {
				Name.remove_prefix(std::string_view("struct ").size());
			} else if (Name.starts_with("enum ")) {
				Name.remove_prefix(std::string_view("enum ").size());
			}
			if (const std::size_t Pos = Name.rfind("::"); Pos != std::string_view::npos) {
				Name.remove_prefix(Pos + std::string_view("::").size());
			}

			return Name;
		}

		template<typename T>
		inline constexpr FClassInfo ClassInfoStorage{TypeName<T>()};
	}

	namespace Core {
		template<typename T>
		[[nodiscard]] constexpr const FClassInfo& ClassInfoOf()
		{
			return Reflection::ClassInfoStorage<T>;
		}
	}

	template<typename TDerived, typename TBase>
	inline constexpr bool SubclassOf = std::is_base_of_v<TBase, TDerived>;
}

/* clang-format off */
#define LK_CLASS()                                                                              \
	public:                                                                                     \
		[[nodiscard]] const ::platformer2d::Core::FClassInfo& GetClass() const override         \
		{                                                                                       \
			return ::platformer2d::Core::ClassInfoOf<::std::remove_cvref_t<decltype(*this)>>(); \
		}
/* clang-format on */

