#pragma once

#include "core/core.h"
#include "itemtype.h"

namespace platformer2d {

	class IItem
	{
	public:
		virtual ~IItem() = default;
		virtual EItemType GetItemType() const = 0;
	};

}
