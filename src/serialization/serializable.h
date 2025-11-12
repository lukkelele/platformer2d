#pragma once

#include "core/core.h"
#include "yamlserialization.h"

namespace platformer2d {

	class ISerializable
	{
	public:
		virtual ~ISerializable() = default;

		virtual void Serialize(YAML::Emitter& Out) = 0;
	};

}
