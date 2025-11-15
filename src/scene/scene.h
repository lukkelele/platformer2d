#pragma once

#include "core/core.h"
#include "actor.h"

namespace platformer2d {

	enum class ESceneState
	{
		Edit,
		Play,
		Pause,
		Simulate
	};

	class CScene
	{
	public:
		CScene(std::string_view InName);
		CScene() = delete;
		~CScene();

		std::shared_ptr<CActor> FindActor(LUUID Handle);
		std::shared_ptr<CActor> FindActor(std::string_view Name);
		bool DoesActorExist(LUUID Handle);
		bool DoesActorExist(std::string_view Name);

	private:
		LUUID ID;
		std::string Name;
		std::vector<std::shared_ptr<CActor>> Actors{};

		bool bPaused = false;
	};

	namespace Enum
	{
		inline const char* ToString(const ESceneState State)
		{
			const char* S = "";
		#define _(EnumValue) case ESceneState::EnumValue: S = #EnumValue; break
			switch (State)
			{
				_(Edit);
				_(Play);
				_(Pause);
				_(Simulate);
				default:
					LK_THROW_ENUM_ERR(State);
					break;
			}
		#undef _
			return S;
		}
	}


}
