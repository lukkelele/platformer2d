#pragma once

#include <filesystem>
#include <span>
#include <string>

#include "lk_config.h"

namespace platformer2d::StringUtils {

	/**
	 * @brief Remove everything that precedes 'Prefix' in the path.
	 */
	inline std::string RemovePreceding(std::string_view Prefix, const std::string& Path)
	{
		const std::size_t Pos = Path.find(Prefix);
		if (Pos == std::string::npos) {
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

	inline std::size_t GetLongestLen(std::span<const std::string_view> Strings)
	{
		std::size_t Longest = 0;
		for (const auto& S : Strings) {
			if (S.size() > Longest) {
				Longest = S.size();
			}
		}
		return Longest;
	};

	inline std::size_t GetIndexOfLongest(std::span<const std::string_view> Strings)
	{
		std::size_t Idx = 0;
		std::size_t CurrentIdx = 0;
		std::size_t Longest = 0;
		for (const auto& S : Strings) {
			if (S.size() > Longest) {
				Longest = S.size();
				Idx = CurrentIdx;
			}
			CurrentIdx++;
		}
		return Idx;
	};

}
