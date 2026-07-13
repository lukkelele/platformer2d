#include "debugrenderer.h"

#include <box2d/box2d.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_common.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>

#include "core/math/math.h"
#include "renderer.h"
#include "physics/body.h"
#include "physics/physicsworld.h"
#include "scene/actor.h"
#include "scene/scene.h"

namespace platformer2d {
	CDebugRenderer::FStringConf CDebugRenderer::StringConf;
	CDebugRenderer::FCircleConf CDebugRenderer::CircleConf;
	CDebugRenderer::FCircleSolidConf CDebugRenderer::CircleSolidConf;
	CDebugRenderer::FPointConf CDebugRenderer::PointConf;
	CDebugRenderer::FPolygonConf CDebugRenderer::PolygonConf;
	CDebugRenderer::FPolygonSolidConf CDebugRenderer::PolygonSolidConf;
	CDebugRenderer::FCapsuleSolidConf CDebugRenderer::CapsuleSolidConf;
	CDebugRenderer::FTransformConf CDebugRenderer::TransformConf;
	CDebugRenderer::FSegmentConf CDebugRenderer::SegmentConf;
	CDebugRenderer::FLineConfig CDebugRenderer::LineConfig;

	static constexpr glm::vec4 QuadVertexPositions[4] = {
		{-0.50f, -0.50f, 0.0f, 1.0f},
		{-0.50f,  0.50f, 0.0f, 1.0f},
		{ 0.50f,  0.50f, 0.0f, 1.0f},
		{ 0.50f, -0.50f, 0.0f, 1.0f}
    };

	static glm::vec4 Decodeb2HexColor(const b2HexColor Hex, float A = 1.0f);

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
				2, 3, 0 /* Triangle 2 */
			};
			LK_OpenGL_Verify(glGenBuffers(1, &QuadEBO));
			LK_OpenGL_Verify(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, QuadEBO));
			LK_OpenGL_Verify(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(QuadIndices), QuadIndices, GL_STATIC_DRAW));

			QuadShader = std::make_shared<CShader>(SHADERS_DIR "/debug_quad.shader");
			QuadShader->Set("u_viewproj", ViewProjection);
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
			LineShader->Set("u_viewproj", ViewProjection);
			LineShader->Set("u_color", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
		}

		/* Circle */
		{
			LK_OpenGL_Verify(glGenVertexArrays(1, &CircleVAO));
			LK_OpenGL_Verify(glGenBuffers(1, &CircleVBO));
			LK_OpenGL_Verify(glBindVertexArray(CircleVAO));
			LK_OpenGL_Verify(glBindBuffer(GL_ARRAY_BUFFER, CircleVBO));

			LK_OpenGL_Verify(glBufferData(GL_ARRAY_BUFFER, 4 * 2 * sizeof(glm::vec2), nullptr, GL_DYNAMIC_DRAW));
			LK_OpenGL_Verify(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), nullptr));
			LK_OpenGL_Verify(glEnableVertexAttribArray(0));
			LK_OpenGL_Verify(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), nullptr));
			LK_OpenGL_Verify(glEnableVertexAttribArray(1));

			CircleShader = std::make_shared<CShader>(SHADERS_DIR "/debug_circle.shader");
			CircleShader->Set("u_viewproj", ViewProjection);
			CircleShader->Set("u_color", FColor::White);
			CircleShader->Set("u_thickness", 2.0f);
		}

		static b2DebugDraw D = b2DefaultDebugDraw();
		DebugDraw = &D;
		D.drawBodyNames = true;

		D.DrawStringFcn = [](const b2Vec2 P, const char* Text, const b2HexColor HexColor, void* const Ctx)
		{
			CDebugRenderer::Box2D_DrawString(P, Text, HexColor, Ctx);
		};

		D.DrawCircleFcn = [](const b2Vec2 Center, const float Radius, const b2HexColor HexColor, void* const Ctx)
		{
			CDebugRenderer::Box2D_DrawCircle(Center, Radius, HexColor, Ctx);
		};

		D.DrawSolidCircleFcn = [](const b2Transform T, const float Radius, const b2HexColor HexColor, void* const Ctx)
		{
			CDebugRenderer::Box2D_DrawSolidCircle(T, Radius, HexColor, Ctx);
		};

		D.DrawPointFcn = [](const b2Vec2 Center, const float Size, const b2HexColor HexColor, void* const Ctx)
		{
			CDebugRenderer::Box2D_DrawPoint(Center, Size, HexColor, Ctx);
		};

		D.DrawPolygonFcn = [](const b2Vec2* const Vertices, const int Count, const b2HexColor HexColor, void* const Ctx)
		{
			CDebugRenderer::Box2D_DrawPolygon(Vertices, Count, HexColor, Ctx);
		};

		D.DrawSolidPolygonFcn = [](const b2Transform T, const b2Vec2* Vertices, const int Count,
									const float Radius, const b2HexColor HexColor, void* const Ctx)
		{
			CDebugRenderer::Box2D_DrawSolidPolygon(T, Vertices, Count, Radius, HexColor, Ctx);
		};

		D.DrawSolidCapsuleFcn = [](const b2Vec2 InP0, const b2Vec2 InP1, const float Radius,
									const b2HexColor HexColor, void* const Ctx)
		{
			CDebugRenderer::Box2D_DrawSolidCapsule(InP0, InP1, Radius, HexColor, Ctx);
		};

		D.DrawTransformFcn = [](const b2Transform T, void* const Ctx)
		{
			CDebugRenderer::Box2D_DrawTransform(T, Ctx);
		};

		D.DrawSegmentFcn = [](const b2Vec2 InP0, const b2Vec2 InP1, const b2HexColor HexColor, void* const Ctx)
		{
			CDebugRenderer::Box2D_DrawSegment(InP0, InP1, HexColor, Ctx);
		};
	}

	void CDebugRenderer::Destroy()
	{
	}

	void CDebugRenderer::SetDrawBounds(const glm::vec2& CameraPos, const glm::vec2& CameraHalfSize)
	{
		LK_ASSERT(DebugDraw);
		if (!DebugDraw) {
			return;
		}
		const glm::vec2 Padding = {0.25f, 0.25f};
		const glm::vec2 HalfSize = CameraHalfSize + Padding;
		DebugDraw->drawingBounds = {
			.lowerBound = b2Vec2(
				CameraPos.x - HalfSize.x,
				CameraPos.y - HalfSize.y),
			.upperBound = b2Vec2(
				CameraPos.x + HalfSize.x,
				CameraPos.y + HalfSize.y)};
	}

	void CDebugRenderer::SetUserData(void* Ctx)
	{
		LK_ASSERT(DebugDraw);
		DebugDraw->context = Ctx;
	}

	void CDebugRenderer::Draw(const std::shared_ptr<CActor> Actor)
	{
		LK_ASSERT(Actor);
		const auto& TC = Actor->GetComponent<FTransformComponent>();
		CRenderer::DrawQuad(
			TC.Translation,
			TC.Scale,
			Actor->GetTexture(),
			Actor->GetColor(),
			glm::degrees(TC.GetRotation2D()),
			Actor->IsOutlineEnabled() ? Actor->GetOutlineThickness() : 0.0f,
			Actor->GetOutlineColor());
	}

	void CDebugRenderer::Draw(const CBody* const Body, const glm::vec4& Color, const glm::vec4& OutlineColor, const float OutlineThickness)
	{
		LK_ASSERT(Body);
		CRenderer::DrawQuad(
			Body->GetPosition(),
			glm::vec3(Body->GetSize(), 0.0f),
			*CRenderer::GetTexture(ETexture::White),
			Color,
			glm::degrees(Body->GetRotation()),
			OutlineThickness,
			OutlineColor);
	}

	void CDebugRenderer::DrawOutline(const CBody* const Body, const glm::vec4& OutlineColor, const float OutlineThickness)
	{
		LK_ASSERT(Body && (OutlineThickness > 0.0f) && (OutlineColor != FColor::Transparent));
		CRenderer::DrawQuad(
			Body->GetPosition(),
			glm::vec3(Body->GetSize(), 0.0f),
			*CRenderer::GetTexture(ETexture::White),
			FColor::Transparent,
			glm::degrees(Body->GetRotation()),
			OutlineThickness,
			OutlineColor);
	}

	void CDebugRenderer::DrawQuad(const glm::vec2& Pos, const glm::vec2& Size, const glm::vec4& Color, const float RotationDeg)
	{
		const glm::mat4 Transform = glm::translate(glm::mat4(1.0f), {Pos.x, Pos.y, 0.0f})
			* glm::rotate(glm::mat4(1.0f), glm::radians(RotationDeg), glm::vec3(0.0f, 0.0f, 1.0f))
			* glm::scale(glm::mat4(1.0f), {Size.x, Size.y, 1.0f});

		glm::vec2 Vertices[4] = {};
		for (std::size_t Idx = 0; Idx < 4; Idx++) {
			Vertices[Idx] = Transform * QuadVertexPositions[Idx];
		}

		QuadShader->Bind();
		QuadShader->Set("u_viewproj", ViewProjection);
		QuadShader->Set("u_color", Color);
		LK_OpenGL_Verify(glBindBuffer(GL_ARRAY_BUFFER, QuadVBO));
		LK_OpenGL_Verify(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertices), Vertices));

		LK_OpenGL_Verify(glBindVertexArray(QuadVAO));
		LK_OpenGL_Verify(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr));

		LK_OpenGL_Verify(glBindVertexArray(0));
		QuadShader->Unbind();
	}

	void CDebugRenderer::DrawLine(const glm::vec2& P0, const glm::vec2& P1, const glm::vec4& Color, const std::uint16_t LineWidth)
	{
		DrawLine({P0.x, P0.y, 0.0f}, {P1.x, P1.y, 0.0f}, Color, LineWidth);
	}

	void CDebugRenderer::DrawLine(const glm::vec3& P0, const glm::vec3& P1, const glm::vec4& Color, const std::uint16_t LineWidth)
	{
		LineShader->Set("u_viewproj", ViewProjection);
		LineShader->Set("u_color", Color);

		const float Vertices[2][2] = {
			{P0.x, P0.y},
			{P1.x, P1.y}
        };
		LK_OpenGL_Verify(glBindBuffer(GL_ARRAY_BUFFER, LineVBO));
		LK_OpenGL_Verify(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertices), Vertices));

		LK_OpenGL_Verify(glBindVertexArray(LineVAO));
		LK_OpenGL_Verify(glLineWidth(LineWidth));
		LK_OpenGL_Verify(glDrawArrays(GL_LINES, 0, 2));
	}

	void CDebugRenderer::DrawCapsule(const glm::vec2& P0, const glm::vec2& P1, const float Radius, const glm::vec4& Color)
	{
		DrawCapsule({P0.x, P0.y, 0.0f}, {P1.x, P1.y, 0.0f}, Radius, Color);
	}

	void CDebugRenderer::DrawCapsule(const glm::vec3& P0, const glm::vec3& P1, const float Radius, const glm::vec4& Color)
	{
		const glm::vec2 Axis = P1 - P0;
		const float Length = glm::length(Axis);
		if (Length < 1e-6f) {
			LK_WARN("Length < 1e-6");
			return;
		}

		const glm::vec2 A = Axis / Length;
		const glm::vec2 N = glm::normalize(Math::Perp(A));
		const glm::vec2 OffsetVec2 = N * Radius;
		const glm::vec4 Offset = {OffsetVec2.x, OffsetVec2.y, 0.0f, 0.0f};

		const glm::mat4 Transform = glm::translate(glm::mat4(1.0f), P0)
			* glm::scale(glm::mat4(1.0f), {Radius * 2.0f, Radius * 2.0f, 1.0f});

		const glm::vec2 V0 = Transform * (glm::vec4(P0.x, P0.y, P0.z, 0.0f) + Offset);
		const glm::vec2 V1 = Transform * (glm::vec4(P1.x, P1.y, P1.z, 0.0f) + Offset);
		const glm::vec2 V2 = Transform * (glm::vec4(P1.x, P1.y, P1.z, 0.0f) - Offset);
		const glm::vec2 V3 = Transform * (glm::vec4(P0.x, P0.y, P0.z, 0.0f) - Offset);

		const glm::vec2 Quad[4] = {V0, V1, V2, V3};

		LK_OpenGL_Verify(glBindVertexArray(QuadVAO));
		LK_OpenGL_Verify(glBindBuffer(GL_ARRAY_BUFFER, QuadVBO));
		LK_OpenGL_Verify(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Quad), Quad));

		QuadShader->Bind();
		QuadShader->Set("u_color", Color);
		QuadShader->Set("u_viewproj", glm::mat4(1.0f));
		LK_OpenGL_Verify(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, QuadEBO));
		LK_OpenGL_Verify(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0));
		QuadShader->Unbind();
	}

	void CDebugRenderer::DrawCircle(const glm::mat4& Transform, const float Radius, const glm::vec4& Color)
	{
		const glm::vec3 LocalP0 = {0.0f, -0.5f, 0.0f};
		const glm::vec3 LocalP1 = {0.0f, 0.5f, 0.0f};

		const glm::vec4 WorldP0 = Transform * glm::vec4(LocalP0, 1.0f);
		const glm::vec4 WorldP1 = Transform * glm::vec4(LocalP1, 1.0f);

		const glm::vec2 P0 = {WorldP0.x, WorldP0.y};
		const glm::vec2 P1 = {WorldP1.x, WorldP1.y};

		const glm::vec2 Axis = P1 - P0;
		const float Length = glm::length(Axis);
		if (Length < 1e-6f) {
			LK_WARN("Length < 1e-6");
			return;
		}

		const glm::vec2 A = Axis / Length;
		const glm::vec2 N = glm::normalize(Math::Perp(A));
		const glm::vec2 Offset = N * Radius;

		const glm::vec2 V0 = P0 + Offset;
		const glm::vec2 V1 = P1 + Offset;
		const glm::vec2 V2 = P1 - Offset;
		const glm::vec2 V3 = P0 - Offset;

		const glm::vec2 Quad[4] = {V0, V1, V2, V3};

		LK_OpenGL_Verify(glBindVertexArray(QuadVAO));
		LK_OpenGL_Verify(glBindBuffer(GL_ARRAY_BUFFER, QuadVBO));
		LK_OpenGL_Verify(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Quad), Quad));

		QuadShader->Bind();
		QuadShader->Set("u_color", Color);
		QuadShader->Set("u_viewproj", glm::mat4(1.0f));
		LK_OpenGL_Verify(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, QuadEBO));
		LK_OpenGL_Verify(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0));
		QuadShader->Unbind();
	}

	void CDebugRenderer::DrawRayHit(const FRayCast& RayCast, const float T, const std::uint16_t LineWidth, const glm::vec4& LineColor,
		const float Radius, const glm::vec4& CircleColor)
	{
		const glm::vec3 Origin = RayCast.Pos;
		const glm::vec3 Dir = RayCast.Dir;
		const glm::vec3 HitPos = Origin + Dir * T;
		LK_DEBUG_TAG("DebugRenderer", "RayHit: Origin={} Dir={} HitPos={} LineWidth={} Radius={}", Origin, Dir, HitPos, LineWidth, Radius);
		CRenderer::Submit([=]()
		{
			CRenderer::DrawLine(Origin, HitPos, LineColor, LineWidth);
			CRenderer::DrawCircleFilled(HitPos, Radius, CircleColor);
		});
	}

	void CDebugRenderer::EnableAllDrawOptions()
	{
		StringConf.bDraw = true;
		CircleConf.bDraw = true;
		CircleSolidConf.bDraw = true;
		PointConf.bDraw = true;
		PolygonConf.bDraw = true;
		PolygonSolidConf.bDraw = true;
		CapsuleSolidConf.bDraw = true;
		TransformConf.bDraw = true;
		SegmentConf.bDraw = true;
	}

	void CDebugRenderer::DisableAllDrawOptions()
	{
		StringConf.bDraw = false;
		CircleConf.bDraw = false;
		CircleSolidConf.bDraw = false;
		PointConf.bDraw = false;
		PolygonConf.bDraw = false;
		PolygonSolidConf.bDraw = false;
		CapsuleSolidConf.bDraw = false;
		TransformConf.bDraw = false;
		SegmentConf.bDraw = false;
	}

	void CDebugRenderer::Box2D_DrawString(const b2Vec2 P, const char* const Text, const b2HexColor HexColor, void* const Ctx)
	{
		if (!StringConf.bDraw) {
			return;
		}
		const glm::vec3 P3 = {P.x, P.y, 1.0f};
		std::string S{Text};
		const glm::vec4 Color = Decodeb2HexColor(HexColor);
		const float Size = StringConf.Size;
		CRenderer::Submit([=]()
		{
			CRenderer::DrawText(S, P3, Size, Color);
		});
	}

	void CDebugRenderer::Box2D_DrawCircle(const b2Vec2 Center, const float Radius, const b2HexColor HexColor, void* const Ctx)
	{
		if (!CircleConf.bDraw) {
			return;
		}
		const glm::vec3 P0 = {Center.x, Center.y, 0.0f};
		const glm::vec4 Color = Decodeb2HexColor(HexColor);
		const glm::vec3 Rot = {0.0f, 0.0f, 0.0f};
		CRenderer::Submit([=]()
		{
			CRenderer::DrawCircle(P0, Rot, Radius, Color);
		});
	}

	void CDebugRenderer::Box2D_DrawSolidCircle(const b2Transform Transform, const float Radius, const b2HexColor HexColor, void* const Ctx)
	{
		if (!CircleSolidConf.bDraw) {
			return;
		}
		const glm::vec4 Color = Decodeb2HexColor(HexColor);
		const glm::mat4 T = Math::ToMat4(Transform);
		CRenderer::Submit([=]()
		{
			CRenderer::DrawCircle(T, Color);
		});
	}

	void CDebugRenderer::Box2D_DrawPoint(const b2Vec2 Center, const float Size, const b2HexColor HexColor, void* const Ctx)
	{
		if (!PointConf.bDraw) {
			return;
		}

		const glm::vec3 P0 = {Center.x, Center.y, 0.0f};
		const glm::vec4 Color = Decodeb2HexColor(HexColor);
		const float PointSize = PointConf.Size;
		CRenderer::Submit([=]()
		{
#if 0 /* @fixme: Assumes valid scene, kept for testing */
			if (Ctx) {
				auto& Scene = *static_cast<CScene*>(Ctx);

				/* @fixme */
				auto Actors = Scene.GetActors();
				std::size_t Idx = 0;
				for (const auto& A : Actors) {
					CRenderer::DrawTextScreen(A->GetName(), {300, 200 + (Idx * 30)}, 22);
					Idx++;
				}
			}
#endif
			CRenderer::DrawCircleFilled(P0, PointSize, Color);
		});
	}

	void CDebugRenderer::Box2D_DrawPolygon(const b2Vec2* const Vertices, const int Count, const b2HexColor HexColor, void* const Ctx)
	{
		if (!PolygonConf.bDraw) {
			return;
		}
		constexpr float Rot = 0.0f;
		const glm::vec4 Color = Decodeb2HexColor(HexColor, PolygonConf.Alpha);

		/* Quad */
		if (Count == 4) {
			/* AABB */
			const b2Vec2& V0 = Vertices[0];
			const b2Vec2& V1 = Vertices[1];
			const b2Vec2& V2 = Vertices[2];
			const b2Vec2& V3 = Vertices[3];
			const float MinX = std::min(std::min(V0.x, V1.x), std::min(V2.x, V3.x));
			const float MinY = std::min(std::min(V0.y, V1.y), std::min(V2.y, V3.y));
			const float MaxX = std::max(std::max(V0.x, V1.x), std::max(V2.x, V3.x));
			const float MaxY = std::max(std::max(V0.y, V1.y), std::max(V2.y, V3.y));
			const glm::vec2 Pos = {(MinX + MaxX) * 0.50f, (MinY + MaxY) * 0.50f};
			const glm::vec2 Size = {(MaxX - MinX), (MaxY - MinY)};

			CRenderer::Submit([=]()
			{
				CRenderer::DrawQuad(Pos, Size, Color, Rot);
			});
		} else {
			LK_WARN_TAG("DebugRenderer", "DrawPolygon: Count not supported {}", Count);
		}
	}

	void CDebugRenderer::Box2D_DrawSolidPolygon(const b2Transform T, const b2Vec2* const Vertices, const int Count,
		const float Radius, b2HexColor HexColor, void* Ctx)
	{
		if (!PolygonSolidConf.bDraw) {
			return;
		}
		const float Rot = std::atan2(T.q.s, T.q.c);
		const glm::vec4 Color = Decodeb2HexColor(HexColor, PolygonSolidConf.Alpha);

		/* Quad */
		if (Count == 4) {
			const glm::vec2 Pos = {T.p.x, T.p.y};
			const glm::vec2 E0 = {(Vertices[1].x - Vertices[0].x), (Vertices[1].y - Vertices[0].y)};
			const glm::vec2 E1 = {(Vertices[2].x - Vertices[1].x), (Vertices[2].y - Vertices[1].y)};
			const glm::vec2 Size = {glm::length(E0), glm::length(E1)};
			CRenderer::Submit([=]()
			{
				CRenderer::DrawQuad(Pos, Size, Color, Rot);
			});
		} else {
			LK_WARN_TAG("DebugRenderer", "DrawSolidPolygon: Count not supported {}", Count);
		}
	}

	void CDebugRenderer::Box2D_DrawSolidCapsule(const b2Vec2 InP0, const b2Vec2 InP1, const float Radius,
		const b2HexColor HexColor, void* const Ctx)
	{
		if (!CapsuleSolidConf.bDraw) {
			return;
		}
		const glm::vec3 P0 = {InP0.x, InP0.y, 0.0f};
		const glm::vec3 P1 = {InP1.x, InP1.y, 0.0f};
		const glm::vec4 Color = Decodeb2HexColor(HexColor);
		CRenderer::Submit([=]()
		{
			CRenderer::DrawCircle(P0, P1, Radius, Color);
		});
	}

	void CDebugRenderer::Box2D_DrawTransform(const b2Transform T, void* const Ctx)
	{
		if (!TransformConf.bDraw) {
			return;
		}
		const glm::mat4 ConvT = Math::ToMat4(T);
		const float Scale = TransformConf.Scale;
		const glm::vec4 Color = TransformConf.Color;
		CRenderer::Submit([=]()
		{
			CRenderer::DrawTransform(ConvT, Scale, Color);
		});
	}

	void CDebugRenderer::Box2D_DrawSegment(const b2Vec2 InP0, const b2Vec2 InP1, const b2HexColor HexColor, void* const Ctx)
	{
		if (!SegmentConf.bDraw) {
			return;
		}
		const glm::vec3 P0 = {InP0.x, InP0.y, 0.0f};
		const glm::vec3 P1 = {InP1.x, InP1.y, 0.0f};
		const glm::vec4 Color = Decodeb2HexColor(HexColor);
		const std::uint16_t LineW = SegmentConf.LineWidth;
		CRenderer::Submit([=]()
		{
			CDebugRenderer::DrawLine(P0, P1, Color, LineW);
		});
	}

	glm::vec4 Decodeb2HexColor(const b2HexColor Hex, const float A)
	{
		float R = 0.0f;
		float G = 0.0f;
		float B = 0.0f;
		const std::uint32_t U = static_cast<std::uint32_t>(Hex);
		const std::uint32_t R8 = (U >> 16) & 0xFF;
		const std::uint32_t G8 = (U >> 8) & 0xFF;
		const std::uint32_t B8 = (U) & 0xFF;

		R = static_cast<float>(R8) / 255.0f;
		G = static_cast<float>(G8) / 255.0f;
		B = static_cast<float>(B8) / 255.0f;
		return glm::vec4(R, G, B, A);
	}

}
