#pragma once

#include "core/core.h"
#include "core/layer.h"
#include "player.h"

namespace platformer2d {

	class CScene;

	struct FSceneSelectionEntry
	{
		LUUID Handle;
		CActor* Ref = nullptr;
		float Distance = 0.0f;
	};

	struct FGameSpecification
	{
		enum { VALUE_UNSET = -1 };

		std::filesystem::path LevelFilepath;

		std::string Name;
		glm::vec2 Gravity = { 0.0f, -9.82f };
		uint16_t ViewportWidth = SCREEN_WIDTH;
		uint16_t ViewportHeight = SCREEN_HEIGHT;

		float Zoom = 0.25f; /* Initial zoom. */

		FBodySpecification PlayerBody{};
	};

	class IGameInstance : public CLayer, public ISerializable<ESerializable::File>
	{
	public:
		IGameInstance(IGameInstance* InstanceRef, const FGameSpecification& InSpec);
		IGameInstance() = delete;
		virtual ~IGameInstance() = default;

		virtual void Initialize() = 0;
		virtual void Destroy() = 0;

		virtual void Tick(float DeltaTime) override = 0;
		virtual CCamera* GetActiveCamera() const = 0;
		virtual CPlayer* GetPlayer(std::size_t Idx = 0) const = 0;
		virtual std::shared_ptr<CScene> GetScene() const = 0;

		virtual uint16_t RaycastScene(std::shared_ptr<CScene> TargetScene, std::vector<FSceneSelectionEntry>& Selected) = 0;
		virtual uint16_t PickSceneAtMouse(std::shared_ptr<CScene> TargetScene, std::vector<FSceneSelectionEntry>& Selected) = 0;

		virtual bool Serialize(const std::filesystem::path& OutFile) const override = 0;
		virtual bool Deserialize(const std::filesystem::path& InFile) override = 0;

		static IGameInstance* Get() { return Instance; }

	protected:
		const FGameSpecification& GetSpecification() const { return Spec; }

	protected:
		uint16_t ViewportWidth = 0;
		uint16_t ViewportHeight = 0;
	private:
		FGameSpecification Spec{};

		static inline IGameInstance* Instance = nullptr;
	};

}
