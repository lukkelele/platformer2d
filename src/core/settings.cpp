#include "settings.h"

#include <fstream>
#include <istream>

#include "serialization/serialization.h"

namespace platformer2d {

	static FSettings Instance;

	FSettings& FSettings::Get()
	{
		return Instance;
	}

	bool FSettings::Serialize(const std::filesystem::path& OutFile) const
	{
		const FSettings& Settings = Get();
		LK_INFO_TAG("Settings", "Serialize: {}", OutFile);
		YAML::Emitter Out;

		Out << YAML::BeginMap; /* Settings */

		Out << YAML::Key << "Window";
		Out << YAML::BeginMap;
		Out << YAML::Key << "StartMaximized" << YAML::Value << Settings.Window.bStartMaximized;
		Out << YAML::Key << "VSync" << YAML::Value << Settings.Window.bVSync;
		Out << YAML::EndMap; /* Window */

		Out << YAML::EndMap; /* Settings */

		std::ofstream File(OutFile);
		File << Out.c_str();

		return true;
	}

	bool FSettings::Deserialize(const std::filesystem::path& InFile)
	{
		/* Create file and directory if they don't exist. */
		if (!std::filesystem::exists(InFile)) {
			bool Result = false;
			if (std::filesystem::exists(InFile.parent_path())) {
				/* Create file as it is missing. */
				LK_INFO("Creating settings file");
				std::ofstream OutFile(InFile);
				if (OutFile.good()) {
					Result = true;
				}
			} else {
				/* Create directory as it is missing. */
				LK_INFO("Creating directory to store settings file");
				if (std::filesystem::create_directories(InFile.parent_path())) {
					LK_INFO("Creating settings file");
					std::ofstream OutFile(InFile);
					if (OutFile.good()) {
						Result = true;
					}
				} else {
					LK_ERROR_TAG("Settings", "Failed to create directory: {}", InFile.parent_path());
				}
			}

			return Result;
		}

		if (!std::filesystem::exists(InFile)) {
			LK_ERROR_TAG("Settings", "Filepath does not exist: {}", InFile);
			return false;
		}

		FSettings& Settings = Get();

		std::ifstream InputStream(InFile);
		std::stringstream StringStream;
		StringStream << InputStream.rdbuf();
		const std::string YamlString = StringStream.str();
		const YAML::Node Data = YAML::Load(YamlString);

		const YAML::Node& WindowNode = Data["Window"];
		if (!WindowNode.IsDefined()) {
			LK_WARN_TAG("Settings", "File is empty");
			return false;
		}

		LK_DESERIALIZE_PROPERTY(StartMaximized, Settings.Window.bStartMaximized, true, WindowNode);
		LK_DESERIALIZE_PROPERTY(VSync, Settings.Window.bVSync, true, WindowNode);
		LK_DEBUG_TAG("Settings", "StartMaximized={} VSync={}", Settings.Window.bStartMaximized, Settings.Window.bVSync);

		return true;
	}

}

