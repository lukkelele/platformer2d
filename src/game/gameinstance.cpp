#include "gameinstance.h"

namespace platformer2d {

	CGameInstance::CGameInstance(CGameInstance* InstanceRef, const FGameSpecification& InSpec)
		: CLayer(InSpec.Name)
		, Spec(InSpec)
		, ViewportWidth(InSpec.ViewportWidth)
		, ViewportHeight(InSpec.ViewportHeight)
	{
		Instance = InstanceRef;
		LK_VERIFY(Instance);
	}

}
