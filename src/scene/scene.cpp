#include "scene.h"

namespace platformer2d {

	CScene::CScene(std::string_view InName)
		: Name(InName)
	{
		LK_ASSERT(!Name.empty(), "Scene name missing");
		LK_DEBUG_TAG("Scene", "Created: {}", Name);
	}

	CScene::~CScene()
	{
		LK_DEBUG_TAG("Scene", "Release: {}", ID);
	}

	std::shared_ptr<CActor> CScene::FindActor(const FActorHandle Handle)
	{
		auto IsHandleEqual = [Handle](const std::shared_ptr<CActor>& Actor)
		{
			return (Handle == Actor->GetHandle());
		};
		auto Iter = std::find_if(Actors.begin(), Actors.end(), IsHandleEqual);
		return (Iter != Actors.end()) ? *Iter : nullptr;
	}

	std::shared_ptr<CActor> CScene::FindActor(std::string_view Name)
	{
		auto IsNameEqual = [Name](const std::shared_ptr<CActor>& Actor)
		{
			return (Name == Actor->GetName());
		};
		auto Iter = std::find_if(Actors.begin(), Actors.end(), IsNameEqual);
		return (Iter != Actors.end()) ? *Iter : nullptr;
	}

	bool CScene::DoesActorExist(const FActorHandle Handle)
	{
		return FindActor(Handle) != nullptr;
	}

	bool CScene::DoesActorExist(std::string_view Name)
	{
		return FindActor(Name) != nullptr;
	}


}
