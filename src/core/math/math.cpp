#include "math.h"

namespace platformer2d::Math {

	glm::vec3 ConvertScreenToWorld(const glm::vec3& Point, const glm::vec3& Center,
								   const float Width, const float Height, const float Zoom)
	{
		const float Ratio = Width / Height;
		const float U = Point.x / Width;
		const float V = (Height - Point.y) / Height;

		const glm::vec3 Extents = { Zoom * Ratio, Zoom, 0.0f };
		const glm::vec3 Lower = Center - Extents;
		const glm::vec3 Upper = Center + Extents;

		return glm::vec3(
			(1.0f - U) * Lower.x + U * Upper.x,
			(1.0f - V) * Lower.y + V * Upper.y,
			0.0f
		);
	}

	glm::vec3 ConvertWorldToScreen(const glm::vec3& Point, const glm::vec3& Center,
								   const float Width, const float Height, const float Zoom)
	{
		const float Ratio = Width / Height;
		const glm::vec3 Extents = { Zoom * Ratio, Zoom, 0.0f };
		const glm::vec3 Lower = Center - Extents;
		const glm::vec3 Upper = Center + Extents;

		const float U = (Point.x - Lower.x) / (Upper.x - Lower.x);
		const float V = (Point.y - Lower.y) / (Upper.y - Lower.y);

		return glm::vec3(U * Width, (1.0f - V) * Height, 0.0f);
	}

}