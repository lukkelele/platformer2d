#include "selectioncontext.h"

namespace platformer2d {

	void CSelectionContext::Select(const LUUID Handle)
	{
		Selected = Handle;
	}

}
