#pragma once

#include "core/core.h"

namespace platformer2d {

	class CSelectionContext
	{
	public:
		using THandle = uint32_t;
	public:
		CSelectionContext() = default;
		~CSelectionContext() = default;

		static void Select(THandle Handle);
		static inline THandle GetSelected() { return Selected; }
		static bool IsSelected(const THandle Handle) { return (Handle == Selected); }

	private:
		CSelectionContext(const CSelectionContext&) = delete;
		CSelectionContext(CSelectionContext&&) = delete;

	private:
		static inline THandle Selected = 0;
	};

}
