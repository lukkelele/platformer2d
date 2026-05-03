#pragma once

#include "core/core.h"

namespace platformer2d {

	class CSelectionContext
	{
	public:
		CSelectionContext() = default;
		~CSelectionContext() = default;
		CSelectionContext(CSelectionContext&&) = delete;
		CSelectionContext(const CSelectionContext&) = delete;

		CSelectionContext& operator=(CSelectionContext&&) = delete;
		CSelectionContext& operator=(const CSelectionContext&) = delete;

		static void Select(LUUID Handle);
		static LUUID GetSelected() { return Selected; }
		static bool IsSelected(const LUUID Handle) { return (Handle == Selected); }
		static bool IsAnySelected() { return (Selected != 0); }

	private:
		static inline LUUID Selected = 0;
	};

}
