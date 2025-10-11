#pragma once

#include <typeindex>
#include <typeinfo>
#include <type_traits>
#include <tuple>
#include <vector>
#include <string_view>

namespace platformer2d::Core {

	template<bool bIsConst, typename TObject, typename TReturnValue, typename... TArgs>
	struct MemberFunction;

	/**
	 * Const member function.
	 */
	template<typename TObject, typename TReturnValue, typename... TArgs>
	struct MemberFunction<true, TObject, TReturnValue, TArgs...>
	{
		using type = TReturnValue(TObject::*)(TArgs...) const;
	};

	/**
	 * Member function.
	 */
	template<typename TObject, typename TReturnValue, typename... TArgs>
	struct MemberFunction<false, TObject, TReturnValue, TArgs...>
	{
		using type = TReturnValue(TObject::*)(TArgs...);
	};

}