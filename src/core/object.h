#pragma once

#include "class.h"

namespace platformer2d {

	class LObject
	{
	public:
		virtual ~LObject() = default;

		[[nodiscard]] virtual const Core::FClassInfo& GetClass() const { return Core::ClassInfoOf<LObject>(); }

		template<typename T>
		[[nodiscard]] bool IsClass() const { return &GetClass() == &Core::ClassInfoOf<T>(); }
	};

}

