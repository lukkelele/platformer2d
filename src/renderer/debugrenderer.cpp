#include "debugrenderer.h"

#include <box2d/box2d.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_common.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>

#include "renderer.h"
#include "physics/physicsworld.h"

namespace platformer2d {

	namespace
	{
		constexpr inline glm::vec4 QuadVertexPositions[4] = {
			{ -0.50f, -0.50f, 0.0f, 1.0f },
			{ -0.50f,  0.50f, 0.0f, 1.0f },
			{  0.50f,  0.50f, 0.0f, 1.0f },
			{  0.50f, -0.50f, 0.0f, 1.0f }
		};

		std::vector<glm::vec3> GenerateCircleVertices(float Radius, std::size_t Count);
	}

	glm::vec4 Decodeb2HexColor(const b2HexColor Hex);

	void CDebugRenderer::Initialize()
	{
		/* Quad */
		{
			LK_OpenGL_Verify(glGenVertexArrays(1, &QuadVAO));
			LK_OpenGL_Verify(glGenBuffers(1, &QuadVBO));

			LK_OpenGL_Verify(glBindVertexArray(QuadVAO));
			LK_OpenGL_Verify(glBindBuffer(GL_ARRAY_BUFFER, QuadVBO));
			LK_OpenGL_Verify(glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(glm::vec2), nullptr, GL_DYNAMIC_DRAW));
			LK_OpenGL_Verify(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), nullptr));
			LK_OpenGL_Verify(glEnableVertexAttribArray(0));

			constexpr uint32_t QuadIndices[6] = {
				0, 1, 2, /* Triangle 1 */
				2, 3, 0  /* Triangle 2 */
			};
			LK_OpenGL_Verify(glGenBuffers(1, &QuadEBO));
			LK_OpenGL_Verify(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, QuadEBO));
			LK_OpenGL_Verify(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(QuadIndices), QuadIndices, GL_STATIC_DRAW));

			QuadShader = std::make_shared<CShader>(SHADERS_DIR "/debug_quad.shader");
			QuadShader->Set("u_proj", glm::mat4(1.0f));
			QuadShader->Set("u_color", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
		}

		/* Line */
		{
			LK_OpenGL_Verify(glGenVertexArrays(1, &LineVAO));
			LK_OpenGL_Verify(glGenBuffers(1, &LineVBO));

			LK_OpenGL_Verify(glBindVertexArray(LineVAO));
			LK_OpenGL_Verify(glBindBuffer(GL_ARRAY_BUFFER, LineVBO));
			LK_OpenGL_Verify(glBufferData(GL_ARRAY_BUFFER, 2 * sizeof(glm::vec2), nullptr, GL_DYNAMIC_DRAW)); 
			LK_OpenGL_Verify(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), nullptr));
			LK_OpenGL_Verify(glEnableVertexAttribArray(0));

			LineShader = std::make_shared<CShader>(SHADERS_DIR "/debug_line.shader");
			LineShader->Set("u_transform", glm::mat4(1.0f));
			QuadShader->Set("u_color", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
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

		b2DebugDraw DebugDraw{};
		DebugDraw.drawBounds = true;
		DebugDraw.drawContacts = true;
		//DebugDraw.drawContactForces = true;
		DebugDraw.drawShapes = true;
		//DebugDraw.drawMass = true;
		//DebugDraw.drawFrictionForces = true;
		//DebugDraw.drawContactNormals = true;

		DebugDraw.DrawCircleFcn = [](b2Vec2 Center, float Radius, b2HexColor HexColor, void* Ctx)
		{
			const glm::vec3 P0 = { Center.x, Center.y, 0.0f };
			//const glm::vec4 Color = { 0.50f, 1.0f, 1.0f, 1.0f };
			const glm::vec4 Color = Decodeb2HexColor(HexColor);
			//LK_WARN("DrawCircleFcn: ({}, {})", P0.x, P0.y);
			CRenderer::DrawCircle(P0, { 0.0f, 0.0f, 0.0f }, Radius, Color);
		};

		DebugDraw.DrawPointFcn = [](b2Vec2 Center, float Size, b2HexColor HexColor, void* Ctx)
		{
			const glm::vec3 P0 = { Center.x, Center.y, 0.0f };
			const glm::vec4 Color = { 0.0f, 1.0f, 1.0f, 1.0f };
			//LK_WARN("DrawPointFcn: ({}, {})", P0.x, P0.y);
			CRenderer::DrawCircle(P0, { 0.0f, 0.0f, 0.0f }, Size , Color);
		};

		DebugDraw.DrawPolygonFcn = [](const b2Vec2* Vertices, const int Count, b2HexColor HexColor, void* Ctx)
		{
			//LK_WARN("DrawPolygonFcn: Count={}", Count);
			constexpr float Rot = 0.0f;
			const glm::vec4 Color = Decodeb2HexColor(HexColor);

			/* Quad */
			if (Count == 4)
			{
				/* AABB */
				const b2Vec2& V0 = Vertices[0];
				const b2Vec2& V1 = Vertices[1];
				const b2Vec2& V2 = Vertices[2];
				const b2Vec2& V3 = Vertices[3];
				const float MinX = std::min(std::min(V0.x, V1.x), std::min(V2.x, V3.x));
				const float MinY = std::min(std::min(V0.y, V1.y), std::min(V2.y, V3.y));
				const float MaxX = std::max(std::max(V0.x, V1.x), std::max(V2.x, V3.x));
				const float MaxY = std::max(std::max(V0.y, V1.y), std::max(V2.y, V3.y));
				const glm::vec2 Pos  = { (MinX + MaxX) * 0.50f, (MinY + MaxY) * 0.50f };
				const glm::vec2 Size = { (MaxX - MinX), (MaxY - MinY) };

				CDebugRenderer::DrawQuad(Pos, Size, Color, Rot);
			}
#if 0
			for (int Idx = 0; Idx < Count; Idx++)
			{
				const b2Vec2& V = Vertices[Idx];
				const glm::vec2 Pos = { V.x, V.y };
				CDebugRenderer::DrawQuad(Pos, { 0.50f, 0.50f }, Color, Rot);
			}
#endif
		};

		DebugDraw.DrawSegmentFcn = [](b2Vec2 InP0, b2Vec2 InP1, b2HexColor HexColor, void* Ctx)
		{
			const glm::vec3 P0 = { InP0.x, InP0.y, 0.0f };
			const glm::vec3 P1 = { InP1.x, InP1.y, 0.0f };
			const glm::vec4 Color = Decodeb2HexColor(HexColor);
			CDebugRenderer::DrawLine(P0, P1, Color, 4);
		};

		DebugDraw.DrawSolidPolygonFcn = [](b2Transform Transform, const b2Vec2* Vertices, int Count,
										   float Radius, b2HexColor HexColor, void* Ctx)
		{
			LK_WARN("DrawSolidPolygon: Count={} Radius={}", Count, Radius);
			constexpr float Rot = 0.0f;
			const glm::vec4 Color = Decodeb2HexColor(HexColor);
			for (int Idx = 0; Idx < Count; Idx++)
			{
				const b2Vec2& V = Vertices[Idx];
				const glm::vec2 Pos = { V.x, V.y };
				//CDebugRenderer::DrawQuad(Pos, { 0.50f, 0.50f }, Color, Rot);
			}
		};

		DebugDraw.DrawSolidCapsuleFcn = [](b2Vec2 InP0, b2Vec2 InP1, float Radius, b2HexColor HexColor, void* Ctx)
		{
			//LK_WARN("DrawSolidCapsule: P0({}, {}) P1({}, {}) Radius={}", InP0.x, InP0.y, InP1.x, InP1.y, Radius);
			const glm::vec3 P0 = { InP0.x, InP0.y, 0.0f };
			const glm::vec3 P1 = { InP1.x, InP1.y, 0.0f };
			const glm::vec4 Color = Decodeb2HexColor(HexColor);
			CRenderer::DrawCircle(P0, P1, Radius, Color);
		};

		CPhysicsWorld::InitDebugDraw(DebugDraw);
	}

	void CDebugRenderer::DrawQuad(const glm::vec2& Pos, const glm::vec2& Size, const glm::vec4& Color, const float RotationDeg)
	{
		const glm::mat4 Transform = glm::translate(glm::mat4(1.0f), { Pos.x, Pos.y, 0.0f })
            * glm::scale(glm::mat4(1.0f), { Size.x, Size.y, 1.0f });

		glm::vec2 Vertices[4] = {};
		for (std::size_t Idx = 0; Idx < 4; Idx++)
		{
			Vertices[Idx] = Transform * QuadVertexPositions[Idx];
		}

		QuadShader->Bind();
		QuadShader->Set("u_color", Color);
		LK_OpenGL_Verify(glBindBuffer(GL_ARRAY_BUFFER, QuadVBO));
		LK_OpenGL_Verify(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertices), Vertices));

		LK_OpenGL_Verify(glBindVertexArray(QuadVAO));
		LK_OpenGL_Verify(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr));

		LK_OpenGL_Verify(glBindVertexArray(0));
		QuadShader->Unbind();
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

	glm::vec4 Decodeb2HexColor(const b2HexColor Hex)
	{
		float R = 0.0f;
		float G = 0.0f;
		float B = 0.0f;
		float A = 0.0f;
		const uint32_t U = static_cast<uint32_t>(Hex);
		const uint32_t R8 = (U >> 16) & 0xFF;
		const uint32_t G8 = (U >> 8)  & 0xFF;
		const uint32_t B8 = (U)       & 0xFF;

		R = static_cast<float>(R8) / 255.0f;
		G = static_cast<float>(G8) / 255.0f;
		B = static_cast<float>(B8) / 255.0f;
		A = 1.0f;

		return glm::vec4(R, G, B, A);
	}

}