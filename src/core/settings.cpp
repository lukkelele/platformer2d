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

	const std::filesystem::path& FSettings::GetFilePath()
	{
		static const std::filesystem::path Path(CONFIG_DIR "/settings.yaml");
		return Path;
	}

	bool FSettings::Save()
	{
		return Get().Serialize(GetFilePath());
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

		Out << YAML::Key << "Graphics";
		Out << YAML::BeginMap;
		Out << YAML::Key << "ShowFPS" << YAML::Value << Settings.Graphics.bShowFPS;
		Out << YAML::Key << "ShowFrametime" << YAML::Value << Settings.Graphics.bShowFrametime;
		Out << YAML::Key << "ShowDebugStats" << YAML::Value << Settings.Graphics.bShowDebugStats;
		Out << YAML::Key << "Brightness" << YAML::Value << Settings.Graphics.Brightness;
		Out << YAML::Key << "UIScale" << YAML::Value << Settings.Graphics.UIScale;
		Out << YAML::EndMap; /* Graphics */

		Out << YAML::Key << "Input";
		Out << YAML::BeginMap;
		Out << YAML::Key << "MouseSensitivity" << YAML::Value << Settings.Input.MouseSensitivity;
		Out << YAML::Key << "ZoomSpeed" << YAML::Value << Settings.Input.ZoomSpeed;
		Out << YAML::Key << "EdgePan" << YAML::Value << Settings.Input.bEdgePan;
		Out << YAML::Key << "EdgePanSpeed" << YAML::Value << Settings.Input.EdgePanSpeed;
		Out << YAML::Key << "InvertCameraDrag" << YAML::Value << Settings.Input.bInvertCameraDrag;
		Out << YAML::EndMap; /* Input */

		Out << YAML::Key << "Gameplay";
		Out << YAML::BeginMap;
		Out << YAML::Key << "Difficulty" << YAML::Value << static_cast<std::size_t>(Settings.Gameplay.Difficulty);
		Out << YAML::Key << "ShowTutorialHints" << YAML::Value << Settings.Gameplay.bShowTutorialHints;
		Out << YAML::Key << "ScreenShake" << YAML::Value << Settings.Gameplay.bScreenShake;
		Out << YAML::Key << "MasterScale" << YAML::Value << Settings.Gameplay.MasterScale;
		Out << YAML::EndMap; /* Gameplay */

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

		/* @todo: Remove these as optional */
		const YAML::Node& GraphicsNode = Node["Graphics"];
		if (GraphicsNode.IsDefined()) {
			Serialization::DeserializeProperty<Serialization::Optional>("ShowFPS", Settings.Graphics.bShowFPS, true, GraphicsNode);
			Serialization::DeserializeProperty<Serialization::Optional>("ShowFrametime", Settings.Graphics.bShowFrametime, false, GraphicsNode);
			Serialization::DeserializeProperty<Serialization::Optional>("ShowDebugStats", Settings.Graphics.bShowDebugStats, false, GraphicsNode);
			Serialization::DeserializeProperty<Serialization::Optional>("Brightness", Settings.Graphics.Brightness, 1.0f, GraphicsNode);
			Serialization::DeserializeProperty<Serialization::Optional>("UIScale", Settings.Graphics.UIScale, 1.0f, GraphicsNode);
		}

		const YAML::Node& InputNode = Node["Input"];
		if (InputNode.IsDefined()) {
			Serialization::DeserializeProperty<Serialization::Optional>("MouseSensitivity", Settings.Input.MouseSensitivity, 1.0f, InputNode);
			Serialization::DeserializeProperty<Serialization::Optional>("ZoomSpeed", Settings.Input.ZoomSpeed, 1.0f, InputNode);
			Serialization::DeserializeProperty<Serialization::Optional>("EdgePan", Settings.Input.bEdgePan, false, InputNode);
			Serialization::DeserializeProperty<Serialization::Optional>("EdgePanSpeed", Settings.Input.EdgePanSpeed, 8.0f, InputNode);
			Serialization::DeserializeProperty<Serialization::Optional>("InvertCameraDrag", Settings.Input.bInvertCameraDrag, false, InputNode);
		}

		const YAML::Node& GameplayNode = Node["Gameplay"];
		if (GameplayNode.IsDefined()) {
			Serialization::DeserializeProperty<Serialization::Optional>("Difficulty", Settings.Gameplay.Difficulty, EDifficulty::Normal, GameplayNode);
			Serialization::DeserializeProperty<Serialization::Optional>("ShowTutorialHints", Settings.Gameplay.bShowTutorialHints, true, GameplayNode);
			Serialization::DeserializeProperty<Serialization::Optional>("ScreenShake", Settings.Gameplay.bScreenShake, true, GameplayNode);
			Serialization::DeserializeProperty<Serialization::Optional>("MasterScale", Settings.Gameplay.MasterScale, 1.0f, GameplayNode);
		}

		return true;
	}

}
