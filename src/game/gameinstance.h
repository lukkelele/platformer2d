#pragma once

#include "core/core.h"
#include "core/layer.h"
#include "player.h"

namespace platformer2d {

	struct FGameSpecification
	{
		enum { VALUE_UNSET = -1 };

		std::string Name;
		glm::vec2 Gravity = { 0.0f, -9.82f };
		uint16_t ViewportWidth = SCREEN_WIDTH;
		uint16_t ViewportHeight = SCREEN_HEIGHT;

		float Zoom = 0.25f; /* Initial zoom. */

		FBodySpecification PlayerBody{};
	};

	class IGameInstance : public CLayer
	{
	public:
		IGameInstance(const FGameSpecification& InSpec);
		IGameInstance() = delete;
		virtual ~IGameInstance() = default;

		virtual void Initialize() = 0;
		virtual void Destroy() = 0;

		virtual void Tick(float DeltaTime) override = 0;
		virtual CCamera* GetActiveCamera() const = 0;

		virtual bool Serialize(const std::filesystem::path& Filepath) = 0;
		virtual bool Deserialize(const std::filesystem::path& Filepath) = 0;

		static IGameInstance* Get() { return Instance; }

	protected:
		const FGameSpecification& GetSpecification() const { return Spec; }

	protected:
		uint16_t ViewportWidth = 0;
		uint16_t ViewportHeight = 0;

		static inline IGameInstance* Instance = nullptr;
	private:
		FGameSpecification Spec{};
	};

}
