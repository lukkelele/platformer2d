#pragma once

#include "core/layer.h"
#include "game/gameinstance.h"
#include "renderer/texture.h"

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
		virtual void RenderUI() override;

		virtual std::shared_ptr<CActor> FindActor(FActorHandle Handle) override;
		virtual std::shared_ptr<CActor> FindActor(std::string_view Name) override;
		virtual bool DoesActorExist(FActorHandle Handle) override;
		virtual bool DoesActorExist(std::string_view Name) override;
		bool DeleteActor(FActorHandle Handle);

		virtual bool Serialize(const std::filesystem::path& Filepath) override;
		virtual bool Deserialize(const std::filesystem::path& Filepath) override;

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
	};

}
