#pragma once

#include "renderer/texture.h"

namespace platformer2d {

	struct FEditorResources
	{
		std::shared_ptr<CTexture> GearIcon = nullptr;
		std::shared_ptr<CTexture> PauseIcon = nullptr;
		std::shared_ptr<CTexture> PlayIcon = nullptr;
		std::shared_ptr<CTexture> PlusIcon = nullptr;
		std::shared_ptr<CTexture> SaveIcon = nullptr;
		struct
		{
			std::shared_ptr<CTexture> MoveIcon = nullptr;
			std::shared_ptr<CTexture> RotateIcon = nullptr;
			std::shared_ptr<CTexture> ScaleIcon = nullptr;
		} Gizmo;


		void Initialize()
		{
			auto LoadTexture = [](std::string_view Path, const EImageFormat Format = EImageFormat::RGBA8,
								  const glm::vec2& Size = { 1.0f, 1.0f }, const bool Inverted = false)
			{
				LK_VERIFY(std::filesystem::exists(Path), "Texture does not exist: ", Path);
				FTextureSpecification Spec = {
					.Path = Path.data(),
					.Name = std::filesystem::path(Path).stem().generic_string(),
					.bInvert = Inverted,
					.Format = Format,
					.SamplerWrap = ETextureWrap::Clamp,
					.SamplerFilter = ETextureFilter::Nearest,
				};
				if (Size.x > 0.0f) {
					Spec.Width = Size.x;
				}
				if (Size.y > 0.0f) {
					Spec.Height = Size.y;
				}

				return std::make_shared<CTexture>(Spec);
			};

			constexpr bool INVERT_COLOR = true;
			GearIcon = LoadTexture(TEXTURES_DIR "/editor/gear.png", EImageFormat::RGBA8, { 1.0f, 1.0f });
			PauseIcon = LoadTexture(TEXTURES_DIR "/editor/pause.png", EImageFormat::RGBA8, { 1.0f, 1.0f }, INVERT_COLOR);
			PlayIcon = LoadTexture(TEXTURES_DIR "/editor/play.png", EImageFormat::RGBA8, { 1.0f, 1.0f }, INVERT_COLOR);
			PlusIcon = LoadTexture(TEXTURES_DIR "/editor/plus.png", EImageFormat::RGBA8, { 1.0f, 1.0f });
			SaveIcon = LoadTexture(TEXTURES_DIR "/editor/save.png", EImageFormat::RGBA8, { 1.0f, 1.0f });

			Gizmo.MoveIcon = LoadTexture(TEXTURES_DIR "/editor/move.png", EImageFormat::RGBA8, { 1.0f, 1.0f }, INVERT_COLOR);
			Gizmo.RotateIcon = LoadTexture(TEXTURES_DIR "/editor/rotate.png", EImageFormat::RGBA8, { 1.0f, 1.0f }, INVERT_COLOR);
			Gizmo.ScaleIcon = LoadTexture(TEXTURES_DIR "/editor/scale.png", EImageFormat::RGBA8, { 1.0f, 1.0f }, INVERT_COLOR);
		}

		void Destroy()
		{
			GearIcon.reset();
			PauseIcon.reset();
			PlusIcon.reset();
			PlayIcon.reset();
			SaveIcon.reset();
			Gizmo.MoveIcon.reset();
			Gizmo.RotateIcon.reset();
			Gizmo.ScaleIcon.reset();
		}
	};

	extern FEditorResources EditorResources;
}