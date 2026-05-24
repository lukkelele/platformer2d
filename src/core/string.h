#pragma once

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <span>
#include <string>

#include "lk_config.h"

namespace platformer2d::StringUtils {

	inline std::string ToLower(const std::string_view Str)
	{
		std::string S{Str};
		std::transform(S.begin(), S.end(), S.begin(), [](unsigned char C)
		{
			return std::tolower(C);
		});
		return S;
	};

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
		return GetPathRelativeToProject(Path.generic_string());
	};

	inline std::string GetPathRelativeToAssetsDir(const std::string& Path)
	{
		constexpr std::string_view Prefix = "/assets/";
		const std::size_t Pos = Path.find(Prefix);
		if (Pos == std::string::npos) {
			return Path;
		}
		return Path.substr(Pos + 1); /* Add 1 to remove preceding '/' */
	};

	inline std::string GetPathRelativeToAssetsDir(const std::filesystem::path& Path)
	{
		return GetPathRelativeToAssetsDir(Path.generic_string());
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
