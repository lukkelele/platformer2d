#include "core/core.h"
#include "core/math/math.h"

#include "test.h"

using namespace platformer2d;

/**
 * @todo Move all the unit tests to a single file that can
 * be executed on github pipeline
 */

TEST_CASE("b2Vec2 from glm::vec2", "[math]")
{
	const glm::vec2 V = { 10.0f, 6.0f };
	{
		b2Vec2 B = { 0.0f, 0.0f };
		Math::Convert(B, V);
		REQUIRE((B.x == V.x) & (B.y == V.y));
	}
	{
		const b2Vec2 B = Math::Convert(V);
		REQUIRE((B.x == V.x) & (B.y == V.y));
	}
}

TEST_CASE("glm::vec2 from b2Vec2", "[math]")
{
	const b2Vec2 B = { 14.0f, 194.0f };
	{
		glm::vec2 V = { 0.0f, 0.0f };
		Math::Convert(V, B);
		REQUIRE((V.x == B.x) & (V.y == B.y));
	}
	{
		const glm::vec2 V = Math::Convert<glm::vec2>(B);
		REQUIRE((V.x == B.x) & (V.y == B.y));
	}
}
