#include "test.h"

#include "game/spritereader.h"

using namespace platformer2d;

TEST_CASE("Nonexistent file returns nullopt", "[spritereader]")
{
	FSpriteReader Reader;
	const std::filesystem::path Path = test::CTestBase::GetAssetsDirectory() / "textures/sprites/__does_not_exist__.lsprite";
	const std::optional<FSpriteSheet> Result = Reader.Read(Path);
	REQUIRE(!Result.has_value());
}

TEST_CASE("Parse Test.lsprite", "[spritereader]")
{
	FSpriteReader Reader;
	const std::filesystem::path Path = test::CTestBase::GetAssetsDirectory() / "textures/sprites/Test.lsprite";
	std::optional<FSpriteSheet> Result = Reader.Read(Path);
	REQUIRE(Result.has_value());

	const FSpriteSheet& Sheet = *Result;
	REQUIRE(Sheet.Name == "Test");
	REQUIRE(Sheet.Type == ESpriteSheetType::Character);
	REQUIRE(Sheet.Texture == ETexture::Player);
	REQUIRE(Sheet.TileSize.x == 32.0f);
	REQUIRE(Sheet.TileSize.y == 32.0f);

	REQUIRE(Sheet.Has(ESpriteFrame::Idle));
	REQUIRE(Sheet.Has(ESpriteFrame::Walk));
	REQUIRE(Sheet.Has(ESpriteFrame::Run));

	const FSpriteAnimation& Walk = Sheet.Get(ESpriteFrame::Walk);
	REQUIRE(Walk.Count() == 4);
	REQUIRE(Walk.Frames[0] == FSpriteCoord{1, 2});
	REQUIRE(Walk.Frames[1] == FSpriteCoord{2, 2});
	REQUIRE(Walk.Frames[2] == FSpriteCoord{3, 2});
	REQUIRE(Walk.Frames[3] == FSpriteCoord{4, 2});
	REQUIRE(Walk.TicksPerFrame == 5);

	const FSpriteAnimation& Idle = Sheet.Get(ESpriteFrame::Idle);
	REQUIRE(Idle.Count() == 2);
	REQUIRE(Idle.Frames[0] == FSpriteCoord{9, 2});
	REQUIRE(Idle.Frames[1] == FSpriteCoord{10, 2});

	const FSpriteAnimation& Run = Sheet.Get(ESpriteFrame::Run);
	REQUIRE(Run.Count() == 4);
	REQUIRE(Run.Frames.front() == FSpriteCoord{5, 2});
	REQUIRE(Run.Frames.back() == FSpriteCoord{8, 2});
}

TEST_CASE("Parse Player.lsprite", "[spritereader]")
{
	FSpriteReader Reader;
	const std::filesystem::path Path = test::CTestBase::GetAssetsDirectory() / "textures/sprites/Player.lsprite";
	std::optional<FSpriteSheet> Result = Reader.Read(Path);
	REQUIRE(Result.has_value());

	const FSpriteSheet& Sheet = *Result;
	REQUIRE(Sheet.Type == ESpriteSheetType::Character);
	REQUIRE(Sheet.Texture == ETexture::Player);

	REQUIRE(Sheet.Has(ESpriteFrame::Idle));
	REQUIRE(Sheet.Has(ESpriteFrame::Walk));
	REQUIRE(Sheet.Has(ESpriteFrame::Jump));
	REQUIRE(Sheet.Has(ESpriteFrame::JumpAscend));
	REQUIRE(Sheet.Has(ESpriteFrame::JumpDescend));
	REQUIRE(Sheet.Has(ESpriteFrame::JumpLanding));
	REQUIRE(Sheet.Has(ESpriteFrame::Hit));
	REQUIRE(Sheet.Has(ESpriteFrame::Slash));
	REQUIRE(Sheet.Has(ESpriteFrame::Punch));
	REQUIRE(Sheet.Has(ESpriteFrame::WalkReversed));

	const FSpriteAnimation& Punch = Sheet.Get(ESpriteFrame::Punch);
	REQUIRE(Punch.Frames.size() == 2);
	REQUIRE(Punch.Frames[0] == FSpriteCoord{13, 2});
	REQUIRE(Punch.Frames[1] == FSpriteCoord{11, 2});
}

TEST_CASE("GetFrame cycles through frames", "[spritereader]")
{
	FSpriteAnimation Anim;
	Anim.Frames = {
		{0, 2},
        {1, 2},
        {2, 2},
        {3, 2}
    };
	Anim.TicksPerFrame = 5;

	REQUIRE(Anim.GetFrame(0) == FSpriteCoord{0, 2});
	REQUIRE(Anim.GetFrame(4) == FSpriteCoord{0, 2});
	REQUIRE(Anim.GetFrame(5) == FSpriteCoord{1, 2});
	REQUIRE(Anim.GetFrame(15) == FSpriteCoord{3, 2});
	REQUIRE(Anim.GetFrame(20) == FSpriteCoord{0, 2});
}

