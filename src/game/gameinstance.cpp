#include "gameinstance.h"

namespace platformer2d {

	CGameInstance::CGameInstance(const FGameSpecification& InSpec)
		: CLayer(InSpec.Name)
		, Spec(InSpec)
		, ViewportWidth(InSpec.ViewportWidth)
		, ViewportHeight(InSpec.ViewportHeight)
	{
	}

}
