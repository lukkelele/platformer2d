#include "selectioncontext.h"

namespace platformer2d {

	void CSelectionContext::Select(const THandle Handle)
	{
		Selected = Handle;
	}

}