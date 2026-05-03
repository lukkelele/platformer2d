#include "shapes.h"

namespace platformer2d {

	/**
	 * @brief Build the point array that gets handed to b2CreateChain.
	 */
	std::vector<b2Vec2> Utility::MakeBox2DChainPoints(std::span<const glm::vec2> Points, const bool Looped, const bool Reversed)
	{
		std::vector<b2Vec2> Out;
		if (Looped) {
			Out.reserve(Points.size());
			for (const glm::vec2& P : Points) {
				Out.push_back({P.x, P.y});
			}
		} else {
			Out.reserve(Points.size() + 2);
			const glm::vec2 GhostFront = 2.0f * Points.front() - Points[1];
			Out.push_back({GhostFront.x, GhostFront.y});
			for (const glm::vec2& P : Points) {
				Out.push_back({P.x, P.y});
			}

			const std::size_t N = Points.size();
			const glm::vec2 GhostBack = 2.0f * Points[N - 1] - Points[N - 2];
			Out.push_back({GhostBack.x, GhostBack.y});
		}

		if (Reversed) {
			std::reverse(Out.begin(), Out.end());
		}

		return Out;
	}
}
