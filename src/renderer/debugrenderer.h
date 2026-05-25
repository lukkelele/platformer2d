#pragma once

#include <glm/glm.hpp>

#include "color.h"
#include "physics/ray.h"
#include "shader.h"
#include "texture.h"
#include "uniformbuffer.h"

namespace platformer2d {

	class CActor;
	class CBody;

	class CDebugRenderer
	{
	public:
		CDebugRenderer() = delete;
		~CDebugRenderer() = delete;
		CDebugRenderer(const CDebugRenderer&) = delete;
		CDebugRenderer(CDebugRenderer&&) = delete;

		CDebugRenderer& operator=(const CDebugRenderer&) = delete;
		CDebugRenderer& operator=(CDebugRenderer&&) = delete;

		static void Initialize();
		static void Destroy();

		static void SetDrawBounds(const glm::vec2& CameraPos, const glm::vec2& CameraHalfSize);
		static void SetUserData(void* Ctx);

		static void Draw(const std::shared_ptr<CActor> Actor);
		static void Draw(const CBody* Body, const glm::vec4& Color = FColor::Magenta, const glm::vec4& OutlineColor = FColor::Transparent, float OutlineThickness = 0.0f);
		static void DrawOutline(const CBody* Body, const glm::vec4& OutlineColor = FColor::Magenta, float OutlineThickness = 6.0f);

		static void DrawQuad(const glm::vec2& Pos, const glm::vec2& Size, const glm::vec4& Color, float RotationDeg = 0.0f);

		static void DrawLine(const glm::vec2& P0, const glm::vec2& P1, const glm::vec4& Color, uint16_t LineWidth = 8);
		static void DrawLine(const glm::vec3& P0, const glm::vec3& P1, const glm::vec4& Color, uint16_t LineWidth = 8);

		static void DrawCapsule(const glm::vec2& P0, const glm::vec2& P1, float Radius, const glm::vec4& Color);
		static void DrawCapsule(const glm::vec3& P0, const glm::vec3& P1, float Radius, const glm::vec4& Color);

		static void DrawCircle(const glm::mat4& Transform, float Radius, const glm::vec4& Color);

		static void DrawRayHit(const FRayCast& RayCast, float T, uint16_t LineWidth = 9,
			const glm::vec4& LineColor = FColor::Convert(RGBA32::Magenta),
			float Radius = 0.030f, const glm::vec4& CircleColor = FColor::Red);

		static void EnableAllDrawOptions();
		static void DisableAllDrawOptions();

	private:
		static void Box2D_DrawString(b2Vec2 P, const char* Text, b2HexColor HexColor, void* Ctx);
		static void Box2D_DrawCircle(b2Vec2 Center, float Radius, b2HexColor HexColor, void* Ctx);
		static void Box2D_DrawSolidCircle(b2Transform T, float Radius, b2HexColor HexColor, void* Ctx);
		static void Box2D_DrawPoint(b2Vec2 Center, float Size, b2HexColor HexColor, void* Ctx);
		static void Box2D_DrawPolygon(const b2Vec2* Vertices, int Count, b2HexColor HexColor, void* Ctx);
		static void Box2D_DrawSolidPolygon(b2Transform T, const b2Vec2* Vertices, int Count, float Radius, b2HexColor HexColor, void* Ctx);
		static void Box2D_DrawSolidCapsule(b2Vec2 InP0, b2Vec2 InP1, float Radius, b2HexColor HexColor, void* Ctx);
		static void Box2D_DrawTransform(b2Transform T, void* Ctx);
		static void Box2D_DrawSegment(b2Vec2 InP0, b2Vec2 InP1, b2HexColor HexColor, void* Ctx);

	public:
		static inline b2DebugDraw* DebugDraw = nullptr;
		static inline glm::mat4 ViewProjection = glm::mat4(1.0f);

		struct FStringConf
		{
			bool bDraw = true;
			float Size = 0.070f;
		} static inline StringConf;

		struct FCircleConf
		{
			bool bDraw = true;
		} static inline CircleConf;

		struct FCircleSolidConf
		{
			bool bDraw = true;
		} static inline CircleSolidConf;

		struct FPointConf
		{
			bool bDraw = true;
			float Size = 0.010f;
		} static inline PointConf;

		struct FPolygonConf
		{
			bool bDraw = true;
			float Alpha = 1.0f;
		} static inline PolygonConf;

		struct FPolygonSolidConf
		{
			bool bDraw = true;
			float Alpha = 1.0f;
		} static inline PolygonSolidConf;

		struct FCapsuleSolidConf
		{
			bool bDraw = true;
			float Alpha = 1.0f;
		} static inline CapsuleSolidConf;

		struct FTransformConf
		{
			bool bDraw = true;
			float Scale = 1.0f;
			glm::vec4 Color = FColor::Magenta;
		} static inline TransformConf;

		struct FSegmentConf
		{
			bool bDraw = true;
			std::uint16_t LineWidth = 2;
		} static inline SegmentConf;

	private:
		static inline GLuint QuadVAO = 0;
		static inline GLuint QuadVBO = 0;
		static inline GLuint QuadEBO = 0;
		static inline std::shared_ptr<CShader> QuadShader = nullptr;

		static inline GLuint LineVAO = 0;
		static inline GLuint LineVBO = 0;
		static inline std::shared_ptr<CShader> LineShader = nullptr;
		struct FLineConfig
		{
			std::uint16_t Width = 2;
		};
		static FLineConfig LineConfig;

		static inline GLuint CircleVAO = 0;
		static inline GLuint CircleVBO = 0;
		static inline std::shared_ptr<CShader> CircleShader = nullptr;
	};

}
