#include "gameinstance.h"

namespace platformer2d {

	IGameInstance::IGameInstance(IGameInstance* InstanceRef, const FGameSpecification& InSpec)
		: CLayer(InSpec.Name)
		, Spec(InSpec)
		, ViewportWidth(InSpec.ViewportWidth)
		, ViewportHeight(InSpec.ViewportHeight)
	{
		Instance = InstanceRef;
		LK_VERIFY(Instance);
	}

}
