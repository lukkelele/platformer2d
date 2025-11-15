#pragma once

#include <filesystem>
#include <string>

namespace platformer2d::StringUtils {

	/**
	 * @brief Remove everything that precedes 'Prefix' in the path.
	 */
	inline std::string RemovePreceding(std::string_view Prefix, const std::string& Path)
	{
		const std::size_t Pos = Path.find(Prefix);
		if (Pos == std::string::npos)
		{
			return Path;
		}

		return Path.substr(Pos);
	};

	inline std::string RemovePreceding(std::string_view Prefix, const std::filesystem::path& Path)
	{
		return RemovePreceding(Prefix, Path.generic_string());
	};

	inline std::string GetPathRelativeToProject(const std::string& Path)
	{
		return RemovePreceding(PROJECT_NAME, Path);
	};

	inline std::string GetPathRelativeToProject(const std::filesystem::path& Path)
	{
		return RemovePreceding(PROJECT_NAME, Path.generic_string());
	};

}
