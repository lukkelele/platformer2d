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

	public:
		static inline glm::mat4 ViewProjection = glm::mat4(1.0f);

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
			uint16_t Width = 2;
		};
		static FLineConfig LineConfig;

		static inline GLuint CircleVAO = 0;
		static inline GLuint CircleVBO = 0;
		static inline std::shared_ptr<CShader> CircleShader = nullptr;
	};

}
