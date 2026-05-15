#pragma once

#include "core/core.h"
#include "serialization/serializable.h"

namespace platformer2d {

	struct FSettings : public ISerializable<ESerializable::File>
	{
		EQuickLoad QuickLoad = EQuickLoad::None;

		struct
		{
			bool bStartMaximized = true;
			bool bVSync = true;
		} Window;

		struct
		{
			bool bShowFPS = true;
			bool bShowFrametime = false;
			bool bShowDebugStats = false;
			float Brightness = 1.0f;
			float UIScale = 1.0f;
		} Graphics;

		struct
		{
			float MouseSensitivity = 1.0f;
			float ZoomSpeed = 1.0f;
			bool bEdgePan = false;
			float EdgePanSpeed = 8.0f;
			bool bInvertCameraDrag = false;
		} Input;

		struct
		{
			EDifficulty Difficulty = EDifficulty::Normal;
			bool bShowTutorialHints = true;
			bool bScreenShake = true;
			float MasterScale = 1.0f;
		} Gameplay;

		static FSettings& Get();
		static const std::filesystem::path& GetFilePath();
		static bool Save();

		bool Serialize(const std::filesystem::path& OutFile) const override;
		bool Deserialize(const std::filesystem::path& InFile) override;
	};

}
