#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>

#include "core/enum.h"
#include "renderer/sprite.h"
#include "renderer/texture.h"

namespace platformer2d {

	enum class ESpriteSheetType : std::uint8_t
	{
		Generic,
		Character,
		COUNT,
	};
	LK_ENUM(ESpriteSheetType);

	enum class ESpriteFrame : std::uint8_t
	{
		Idle,
		Walk,
		Run,
		Attack,
		Damaged,
		Jump,
		JumpAscend,
		JumpDescend,
		JumpLanding,
		Hit,
		Slash,
		Punch,
		WalkReversed,
		COUNT,
	};
	LK_ENUM(ESpriteFrame);

	struct FSpriteSheet
	{
		std::string Name;
		ESpriteSheetType Type = ESpriteSheetType::Generic;
		ETexture Texture = ETexture::White;
		glm::vec2 TileSize{0.0f, 0.0f};
		std::map<ESpriteFrame, FSpriteAnimation> Animations;

		[[nodiscard]] bool Has(const ESpriteFrame Frame) const { return Animations.contains(Frame); }
		[[nodiscard]] const FSpriteAnimation& Get(const ESpriteFrame Frame) const { return Animations.at(Frame); }

		[[nodiscard]] const FSpriteAnimation* Find(const ESpriteFrame Frame) const
		{
			const auto It = Animations.find(Frame);
			return (It != Animations.end()) ? &It->second : nullptr;
		}
	};

	struct FSpriteReader
	{
		std::optional<FSpriteSheet> Read(const std::filesystem::path& Filepath);
	};
}
