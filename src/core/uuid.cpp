#include "uuid.h"

namespace platformer2d {

	LUUID::LUUID()
		: UUID(Generate())
	{
	}

	LUUID::LUUID(const SizeType InUUID)
		: UUID(InUUID)
	{
	}

}
