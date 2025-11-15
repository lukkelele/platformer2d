#pragma once

#include "core/layer.h"
#include "game/gameinstance.h"
#include "renderer/texture.h"
#include "scene/scene.h"

namespace platformer2d::Level {

	class CTestLevel : public IGameInstance
	{
	public:
		CTestLevel();
		virtual ~CTestLevel() = default;

		virtual void Initialize() override;
		virtual void Destroy() override;

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		virtual void Tick(float DeltaTime) override;
		virtual CCamera* GetActiveCamera() const override;
		virtual CPlayer* GetPlayer(std::size_t Idx = 0) const override;
		virtual std::shared_ptr<CScene> GetScene() const override { return Scene; }

		virtual uint16_t RaycastScene(std::shared_ptr<CScene> TargetScene, std::vector<FSceneSelectionEntry>& Selected) override;
		virtual uint16_t PickSceneAtMouse(std::shared_ptr<CScene> TargetScene, std::vector<FSceneSelectionEntry>& Selected) override;

		virtual void RenderUI() override;

		virtual bool Serialize(const std::filesystem::path& OutFile) const override;
		virtual bool Deserialize(const std::filesystem::path& InFile) override;

	private:
		void CreatePlayer();
		void CreatePlatform();
		void CreateTerrain();

		void Tick_Objects(float DeltaTime);

		void UI_Level();
		void UI_Player();
		void UI_TextureModifier();
		void UI_TextureDropDown(std::size_t& SelectedIdx);

		void DrawBackground() const;
		void DrawClouds() const;

		void OnWindowResized(uint16_t InWidth, uint16_t InHeight);

		void DeserializeActors(const YAML::Node& ActorsNode);

	private:
		std::unique_ptr<CPlayer> Player = nullptr;
		std::shared_ptr<CScene> Scene = nullptr;

		std::vector<FSceneSelectionEntry> SelectionData;
	};

}
