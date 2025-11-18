#pragma once

#include "core/core.h"

namespace platformer2d {

	class CSelectionContext
	{
	public:
		CSelectionContext() = default;
		~CSelectionContext() = default;

		static void Select(LUUID Handle);
		static LUUID GetSelected() { return Selected; }
		static bool IsSelected(const LUUID Handle) { return (Handle == Selected); }
		static bool AnySelected() { return (Selected != 0); }

	private:
		CSelectionContext(const CSelectionContext&) = delete;
		CSelectionContext(CSelectionContext&&) = delete;

	private:
		static inline LUUID Selected = 0;
	};

}
