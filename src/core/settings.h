#pragma once

#include "core/core.h"
#include "serialization/serializable.h"

namespace platformer2d {

	struct FSettings : public ISerializable<ESerializable::File>
	{
		struct
		{
			bool bStartMaximized = true;
			bool bVSync = true;
		} Window;

		static FSettings& Get();

		virtual bool Serialize(const std::filesystem::path& OutFile) const override;
		virtual bool Deserialize(const std::filesystem::path& InFile) override;
	};

}
