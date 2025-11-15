#include "math.h"

namespace platformer2d::Math {

	float Randomize(const float Min, const float Max)
	{
		static std::mt19937 Engine(std::random_device{}());
		std::uniform_real_distribution<float> Dist(Min, Max);
		return Dist(Engine);
	}

	bool DecomposeTransform(const glm::mat4& Transform, glm::vec3& Translation, glm::quat& Rotation, glm::vec3& Scale)
	{
		using T = float;
		glm::mat4 LocalMatrix(Transform);
		if (glm::epsilonEqual(LocalMatrix[3][3], static_cast<T>(0), glm::epsilon<T>()))
		{
			return false;
		}

		Translation = glm::vec3(LocalMatrix[3]);
		LocalMatrix[3] = glm::vec4(0, 0, 0, LocalMatrix[3].w);

		glm::vec3 Row[3] = {};
		/* Scale and shear. */
		for (glm::length_t i = 0; i < 3; i++)
		{
			for (glm::length_t j = 0; j < 3; j++)
			{
				Row[i][j] = LocalMatrix[i][j];
			}
		}

		/* Compute the X-scale and normalize the first row. */
		Scale.x = glm::length(Row[0]);
		Row[0] = Math::Scale(Row[0], static_cast<T>(1));

		/* Compute the Y-scale and normalize the second row. */
		Scale.y = glm::length(Row[1]);
		Row[1] = Math::Scale(Row[1], static_cast<T>(1));

		/* Get the Z-scale and normalize the third row. */
		Scale.z = glm::length(Row[2]);
		Row[2] = Math::Scale(Row[2], static_cast<T>(1));

		/* Get the rotation as a quaternion. */
		int i = 0;
		int j = 0;
		int k = 0;
		T SquareRoot, Trace = Row[0].x + Row[1].y + Row[2].z;
		if (Trace > static_cast<T>(0))
		{
			SquareRoot = std::sqrt(Trace + static_cast<T>(1));
			Rotation.w = static_cast<T>(0.50) * SquareRoot;
			SquareRoot = static_cast<T>(0.50) / SquareRoot;

			Rotation.x = SquareRoot * (Row[1].z - Row[2].y);
			Rotation.y = SquareRoot * (Row[2].x - Row[0].z);
			Rotation.z = SquareRoot * (Row[0].y - Row[1].x);
		}
		else
		{
			static int Next[3] = { 1, 2, 0 };
			i = 0;
			if (Row[1].y > Row[0].x)
			{
				i = 1;
			}
			if (Row[2].z > Row[i][i])
			{
				i = 2;
			}

			j = Next[i];
			k = Next[j];

			SquareRoot = sqrt(Row[i][i] - Row[j][j] - Row[k][k] + static_cast<T>(1.0));

			Rotation[i] = static_cast<T>(0.50) * SquareRoot;
			SquareRoot = static_cast<T>(0.50) / SquareRoot;
			Rotation[j] = SquareRoot * (Row[i][j] + Row[j][i]);
			Rotation[k] = SquareRoot * (Row[i][k] + Row[k][i]);
			Rotation.w = SquareRoot * (Row[j][k] - Row[k][j]);
		}

		return true;
	}

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