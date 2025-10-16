#pragma once

#include <string>
#include <vector>

namespace platformer2d {

	struct FBackendInfo
	{
		struct {
			int Major;
			int Minor;
		} Version;
		std::vector<std::string> Extensions;
	};

}
