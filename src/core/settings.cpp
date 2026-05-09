#include "settings.h"

#include <fstream>
#include <istream>
#include <mutex>

#include "serialization/serialization.h"

namespace platformer2d {

	static std::mutex Mutex;

	FSettings& FSettings::Get()
	{
		static FSettings Instance;
		return Instance;
	}

	bool FSettings::Serialize(const std::filesystem::path& OutFile) const
	{
		const FSettings& Settings = Get();
		std::scoped_lock Lock(Mutex);
		LK_INFO_TAG("Settings", "Serialize: {}", OutFile);
		YAML::Emitter Out;

		Out << YAML::BeginMap; /* Settings */

		Out << YAML::Key << "QuickLoad" << YAML::Value << static_cast<std::size_t>(Settings.QuickLoad);

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
		FSettings& Settings = Get();
		std::scoped_lock Lock(Mutex);

		/* Create file and directory if they don't exist. */
		if (!std::filesystem::exists(InFile)) {
			bool Result = false;
			if (std::filesystem::exists(InFile.parent_path())) {
				/* Create missing settings file. */
				LK_INFO_TAG("Settings", "Creating settings file: {}", InFile);
				std::ofstream OutFile(InFile);
				Result = OutFile.good();
			} else {
				/* Create missing directory. */
				LK_INFO_TAG("Settings", "Creating directory to store settings file");
				if (std::filesystem::create_directories(InFile.parent_path())) {
					LK_INFO_TAG("Settings", "Creating settings file: {}", InFile);
					std::ofstream OutFile(InFile);
					Result = OutFile.good();
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

		std::ifstream InputStream(InFile);
		std::stringstream StringStream;
		StringStream << InputStream.rdbuf();
		const std::string YamlString = StringStream.str();
		const YAML::Node Node = YAML::Load(YamlString);

		Serialization::DeserializeProperty("QuickLoad", Settings.QuickLoad, EQuickLoad::None, Node);

		const YAML::Node& WindowNode = Node["Window"];
		if (WindowNode.IsDefined()) {
			Serialization::DeserializeProperty("StartMaximized", Settings.Window.bStartMaximized, true, WindowNode);
			Serialization::DeserializeProperty("VSync", Settings.Window.bVSync, true, WindowNode);
			LK_DEBUG_TAG("Settings", "StartMaximized={} VSync={}", Settings.Window.bStartMaximized, Settings.Window.bVSync);
		} else {
			LK_ERROR_TAG("Settings", "Missing 'Window' node in YAML data");
		}

		return true;
	}

}

