#pragma once

#include "renderer/texture.h"

namespace platformer2d {

	struct FEditorResources
	{
		std::shared_ptr<CTexture> GearIcon = nullptr;
		std::shared_ptr<CTexture> PlusIcon = nullptr;
		std::shared_ptr<CTexture> SaveIcon = nullptr;

		void Initialize()
		{
			auto LoadTexture = [](std::string_view Path, const EImageFormat Format = EImageFormat::RGBA8,
								  const glm::vec2& Size = { 0.0f, 0.0f })
			{
				LK_VERIFY(std::filesystem::exists(Path), "Texture does not exist: ", Path);
				FTextureSpecification Spec = {
					.Path = Path.data(),
					.Format = Format,
					.SamplerWrap = ETextureWrap::Clamp,
					.SamplerFilter = ETextureFilter::Nearest,
				};
				if (Size.x > 0.0f)
				{
					Spec.Width = Size.x;
				}
				if (Size.y > 0.0f)
				{
					Spec.Height = Size.y;
				}

				return std::make_shared<CTexture>(Spec);
			};

			GearIcon = LoadTexture(TEXTURES_DIR "/editor/gear.png", EImageFormat::RGBA8, { 1.0f, 1.0f });
			PlusIcon = LoadTexture(TEXTURES_DIR "/editor/plus.png", EImageFormat::RGBA8, { 1.0f, 1.0f });
			SaveIcon = LoadTexture(TEXTURES_DIR "/editor/save.png", EImageFormat::RGBA8, { 1.0f, 1.0f });
		}

		void Destroy()
		{
			GearIcon.reset();
			PlusIcon.reset();
			SaveIcon.reset();
		}
	};

	extern FEditorResources EditorResources;

}