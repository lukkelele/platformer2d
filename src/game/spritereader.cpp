#include "spritereader.h"

#include <array>
#include <fstream>
#include <sstream>
#include <utility>

#include "core/log.h"
#include "serialization/serialization.h"

namespace platformer2d {
	static constexpr std::array<ESpriteFrame, 2> CHARACTER_REQUIRED_ANIMS = {
		ESpriteFrame::Idle,
		ESpriteFrame::Walk,
	};

	static void DeserializeAnimations(FSpriteSheet& Sheet, const YAML::Node& Node, const std::uint16_t DefaultRow)
	{
		for (const auto& Pair : Node) {
			const std::string Key = Pair.first.as<std::string>();
			const auto Frame = Enum::FromString<ESpriteFrame>(Key);
			if (Frame == ESpriteFrame::COUNT) {
				LK_WARN_TAG("SpriteReader", R"(Unknown animation: "{}" - skipping)", Key);
				continue;
			}

			FSpriteAnimation Anim;
			std::uint16_t Row = DefaultRow;
			std::vector<std::uint16_t> Indices;

			const YAML::Node& Value = Pair.second;
			if (Value.IsSequence()) {
				Indices.reserve(Value.size());
				for (const auto& IndexNode : Value) {
					Indices.push_back(IndexNode.as<std::uint16_t>());
				}
			} else if (Value.IsMap()) {
				Serialization::DeserializeProperty<Serialization::Optional>("Row", Row, DefaultRow, Value);
				Serialization::DeserializeProperty<Serialization::Optional>("TicksPerFrame", Anim.TicksPerFrame, std::uint16_t{1}, Value);

				const YAML::Node FramesNode = Value["Frames"];
				if (FramesNode.IsSequence()) {
					Indices.reserve(FramesNode.size());
					for (const auto& IndexNode : FramesNode) {
						Indices.push_back(IndexNode.as<std::uint16_t>());
					}
				}
			} else {
				LK_WARN_TAG("SpriteReader", R"(Animation "{}" has invalid form)", Key);
				continue;
			}

			if (Indices.empty()) {
				LK_WARN_TAG("SpriteReader", R"(Animation "{}" has no frames)", Key);
				continue;
			}

			Anim.Frames.reserve(Indices.size());
			for (const std::uint16_t X : Indices) {
				Anim.Frames.push_back({X, Row});
			}
			Anim.StartTileX = Indices.front();
			Anim.StartTileY = Row;
			Anim.FrameCount = Indices.size();
			Sheet.Animations.emplace(Frame, std::move(Anim));
		}
	}

	std::optional<FSpriteSheet> FSpriteReader::Read(const std::filesystem::path& Filepath)
	{
		if (!std::filesystem::exists(Filepath)) {
			LK_ERROR_TAG("SpriteReader", R"(File does not exist: "{}")", Filepath.generic_string());
			return std::nullopt;
		}

		LK_DEBUG_TAG("SpriteReader", R"(Read: "{}")", Filepath);
		YAML::Node Root;
		try {
			std::ifstream Stream(Filepath);
			std::stringstream Buffer;
			Buffer << Stream.rdbuf();
			Root = YAML::Load(Buffer.str());
		} catch (const std::exception& Exception) {
			LK_ERROR_TAG("SpriteReader", R"(Failed to parse "{}": {})", Filepath.generic_string(), Exception.what());
			return std::nullopt;
		}

		FSpriteSheet Sheet;

		Serialization::DeserializeProperty("Name", Sheet.Name, std::string{}, Root);

		std::string TypeStr;
		Serialization::DeserializeProperty("Type", TypeStr, std::string{}, Root);
		if (!TypeStr.empty()) {
			const auto ParsedType = Enum::FromString<ESpriteSheetType>(TypeStr);
			if (ParsedType == ESpriteSheetType::COUNT) {
				LK_ERROR_TAG("SpriteReader", R"(Unknown sheet type: "{}")", TypeStr);
				return std::nullopt;
			}
			Sheet.Type = ParsedType;
		}

		std::string TexStr;
		Serialization::DeserializeProperty("Texture", TexStr, std::string{}, Root);
		if (!TexStr.empty()) {
			const auto ParsedTex = Enum::FromString<ETexture>(TexStr);
			if (ParsedTex == ETexture::COUNT) {
				LK_ERROR_TAG("SpriteReader", R"(Unknown texture: "{}")", TexStr);
				return std::nullopt;
			}
			Sheet.Texture = ParsedTex;
		}

		Serialization::DeserializeProperty("TileSize", Sheet.TileSize, glm::vec2{0.0f, 0.0f}, Root);

		std::uint16_t DefaultRow = 0;
		Serialization::DeserializeProperty<Serialization::Optional>("Row", DefaultRow, std::uint16_t{0}, Root);

		const YAML::Node AnimsNode = Root["Animations"];
		if (!AnimsNode || !AnimsNode.IsMap()) {
			LK_ERROR_TAG("SpriteReader", R"(Missing or invalid "Animations" map in "{}")", Filepath.generic_string());
			return std::nullopt;
		}
		DeserializeAnimations(Sheet, AnimsNode, DefaultRow);

		if (Sheet.Type == ESpriteSheetType::Character) {
			/* Validate that the frames exist from the read sprite. */
			for (const ESpriteFrame Required : CHARACTER_REQUIRED_ANIMS) {
				if (!Sheet.Animations.contains(Required)) {
					LK_ERROR_TAG("SpriteReader", R"(Character sheet "{}" missing required animation: {})",
						Sheet.Name, Enum::ToString(Required));
					return std::nullopt;
				}
			}
		}

		LK_DEBUG_TAG("SpriteReader", R"(Loaded "{}" Type={} Texture={} TileSize=({}, {}) Anims={})",
			Sheet.Name, Enum::ToString(Sheet.Type), Enum::ToString(Sheet.Texture),
			Sheet.TileSize.x, Sheet.TileSize.y, Sheet.Animations.size());

		return Sheet;
	}
}
