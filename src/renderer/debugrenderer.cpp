#include "debugrenderer.h"

namespace platformer2d {

	std::vector<glm::vec3> GenerateCircleVertices(float Radius, std::size_t Count);

	void CDebugRenderer::Initialize()
	{
		/* Line */
		{
			glGenVertexArrays(1, &LineVAO);
			glGenBuffers(1, &LineVBO);

			glBindVertexArray(LineVAO);
			glBindBuffer(GL_ARRAY_BUFFER, LineVBO);
			glBufferData(GL_ARRAY_BUFFER, 2 * sizeof(glm::vec2), nullptr, GL_DYNAMIC_DRAW);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), nullptr);
			glEnableVertexAttribArray(0);
		}

		/* Circle */
		{
#if 0
			LK_OpenGL_Verify(glGenVertexArrays(1, &CircleVAO));
			LK_OpenGL_Verify(glGenBuffers(1, &CircleVBO));
			LK_OpenGL_Verify(glBindVertexArray(CircleVAO));
			LK_OpenGL_Verify(glBindBuffer(GL_ARRAY_BUFFER, CircleVBO));

			LK_OpenGL_Verify(glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * CircleVertices.size(), &CircleVertices[0], GL_STATIC_DRAW));
			LK_OpenGL_Verify(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr));
			LK_OpenGL_Verify(glEnableVertexAttribArray(0));
#endif

		}
	}

	void CDebugRenderer::DrawLine(const glm::vec2& P0, const glm::vec2& P1, const glm::vec4& Color, const uint16_t LineWidth)
	{
		DrawLine({ P0.x, P0.y, 0.0f }, { P1.x, P1.y, 0.0f }, Color, LineWidth);
	}

	void CDebugRenderer::DrawLine(const glm::vec3& P0, const glm::vec3& P1, const glm::vec4& Color, const uint16_t LineWidth)
	{
		LineShader->Set("u_transform", glm::mat4(1.0f));
		LineShader->Set("u_color", Color);

		const float Vertices[2][2] = { { P0.x, P0.y }, { P1.x, P1.y } };
		LK_OpenGL_Verify(glBindBuffer(GL_ARRAY_BUFFER, LineVBO));
		LK_OpenGL_Verify(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertices), Vertices));

		LK_OpenGL_Verify(glBindVertexArray(LineVAO));
		LK_OpenGL_Verify(glLineWidth(LineWidth));
		LK_OpenGL_Verify(glDrawArrays(GL_LINES, 0, 2));
	}

	std::vector<glm::vec3> GenerateCircleVertices(const float Radius, const std::size_t Count)
	{
		const float Angle = 360.0f / Count;
		const int TriangleCount = Count - 2;

		std::vector<glm::vec3> Tmp;
		for (int Idx = 0; Idx < Count; Idx++)
		{
			const float CurrentAngle = Angle * Idx;
			const float x = Radius * std::cos(glm::radians(CurrentAngle));
			const float y = Radius * std::sin(glm::radians(CurrentAngle));
			const float z = 0.0f;

			Tmp.emplace_back(glm::vec3(x, y, z));
		}

		std::vector<glm::vec3> Vertices;
		Vertices.reserve(TriangleCount * 3);
		for (int Idx = 0; Idx < TriangleCount; Idx++)
		{
			Vertices.push_back(Tmp[0]);
			Vertices.push_back(Tmp[Idx + 1]);
			Vertices.push_back(Tmp[Idx + 2]);
		}

		return Vertices;
	}

}