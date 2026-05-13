#include "runtimelayer.h"

#include "core/string.h"
#include "renderer/renderer.h"
#include "renderer/ui/ui.h"
#include "physics/physicsworld.h"
#include "serialization/serialization.h"

namespace platformer2d {

	namespace {
		const FGameSpecification GameSpec = {
			/* clang-format off */
			.InstanceName = "Runtime",
			.LevelFilepath = std::filesystem::path(LEVELS_DIR "/testlevel.yaml"),
			.Player = {
				.ActorSpec = FActorSpecification(ETexture::Player),
				.BodySpec = {
					.Type = EBodyType::Dynamic,
					.Shape = FCapsule{
						.P0 = { 0.0f, -0.02f },
						.P1 = { 0.0f,  0.02f },
						.Radius = 0.10f,
					},
					.Position = { 0.0f, 0.50f },
					.Friction = 0.620f,
					.Density = 0.60f,
					.LinearDamping = 0.560f,
					.Flags = EBodyFlag::EBodyFlag_SensorEvents,
					.MotionLock = EMotionLock_Z,
				},
			}
			/* clang-format on */
		};
	}

	CRuntimeLayer::CRuntimeLayer()
		: CGameInstance(this, GameSpec)
	{
		LK_TRACE_TAG("RuntimeLayer", "Instance created");
	}

	CRuntimeLayer::~CRuntimeLayer()
	{
		LK_TRACE_TAG("RuntimeLayer", "Destructor");
	}

	void CRuntimeLayer::RenderUI()
	{
		if (!UI::BeginViewport()) {
			return;
		}

		const ImVec2 WindowSize = ImGui::GetWindowSize();
		ViewportWidth = WindowSize.x;
		ViewportHeight = WindowSize.y;
		UI_ViewportTexture();

		UI::EndViewport();
	}

	bool CRuntimeLayer::Serialize(const std::filesystem::path& OutFile) const
	{
		/** @todo: Player-specific data about checkpoints and progress should only be persistent. */
		LK_WARN_TAG("RuntimeLayer", "[TODO] Serialize: {}", OutFile);
		return true;
	}

	bool CRuntimeLayer::Deserialize(const std::filesystem::path& InFile)
	{
		LK_INFO_TAG("RuntimeLayer", "Deserialize: {}", StringUtils::GetPathRelativeToProject(InFile));
		LK_ASSERT(std::filesystem::exists(InFile), "Filepath does not exist: {}", InFile);
		if (!std::filesystem::exists(InFile)) {
			LK_ERROR_TAG("RuntimeLayer", "Filepath does not exist: {}", InFile);
			return false;
		}

		std::ifstream InputStream(InFile);
		std::stringstream StringStream;
		StringStream << InputStream.rdbuf();
		const std::string YamlString = StringStream.str();

		const YAML::Node Data = YAML::Load(YamlString);
		Serialization::DeserializeProperty("Gravity", LevelData.Gravity, glm::vec2(0.0f, -5.0f), Data);
		Serialization::DeserializeProperty("PlayerSpawn", LevelData.PlayerSpawn, glm::vec2(0.0f, 0.0f), Data);
		Serialization::DeserializeProperty("CameraZoom", LevelData.SceneLoadCameraZoom, 0.40f, Data);

		const YAML::Node& SceneNode = Data["Scene"];
		LK_ASSERT(!SceneNode.IsNull());
		if (SceneNode.IsNull()) {
			LK_ERROR_TAG("RuntimeLayer", "Scene node is missing in YAML");
			return false;
		}

		const std::filesystem::path SceneFilepath = SceneNode.as<std::filesystem::path>();
		LK_INFO_TAG("RuntimeLayer", "Scene to open: {}", StringUtils::GetPathRelativeToProject(SceneFilepath));
		SceneToOpen = SceneFilepath;

		return !SceneToOpen.empty();
	}

	void CRuntimeLayer::UI_ViewportTexture()
	{
		const ImVec2 WindowSize = {static_cast<float>(ViewportWidth), static_cast<float>(ViewportHeight)};
		std::shared_ptr<CFramebuffer> Framebuffer = CRenderer::GetViewportFramebuffer();
		std::shared_ptr<CTexture> ViewportTexture = Framebuffer->GetImage(0);

		ImGui::Image(
			static_cast<ImTextureID>(ViewportTexture->GetID()),
			WindowSize,
			ImVec2(0, 1),
			ImVec2(1, 0),
			ImVec4(1, 1, 1, 1),
			ImVec4(1, 1, 1, 0));
	}

}

