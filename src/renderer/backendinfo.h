#pragma once

#include <string>
#include <vector>

namespace platformer2d {

	struct FBackendInfo
	{
		struct
		{
			int Major = 0;
			int Minor = 0;
		} Version;
		std::vector<std::string> Extensions;
	};

}
