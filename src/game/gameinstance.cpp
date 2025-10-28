#include "gameinstance.h"

namespace platformer2d {

	IGameInstance::IGameInstance(const FGameSpecification& InSpec)
		: CLayer(InSpec.Name)
		, Spec(InSpec)
		, ViewportWidth(InSpec.ViewportWidth)
		, ViewportHeight(InSpec.ViewportHeight)
	{
	}

}
