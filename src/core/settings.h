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

		static FSettings& Get();

		bool Serialize(const std::filesystem::path& OutFile) const override;
		bool Deserialize(const std::filesystem::path& InFile) override;
	};

}
