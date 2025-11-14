#pragma once

#include "core/core.h"
#include "yaml.h"

namespace platformer2d {

	class ISerializable
	{
	public:
		virtual ~ISerializable() = default;

		virtual void Serialize(YAML::Emitter& Out) = 0;
	};

}
