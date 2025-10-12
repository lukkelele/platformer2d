#pragma once

#include <glm/glm.hpp>

#include "core/core.h"
#include "core/log.h"
#include "shader.h"
#include "texture.h"
#include "imguilayer.h"

namespace platformer2d {

	class CRenderer
	{
	public:
		CRenderer() = delete;
		~CRenderer() = delete;
		CRenderer(const CRenderer&) = delete;
		CRenderer(CRenderer&&) = delete;

		CRenderer& operator=(const CRenderer&) = delete;
		CRenderer& operator=(CRenderer&&) = delete;

		static void Initialize();
		static void Destroy();
		static void BeginFrame();
		static void EndFrame();

		static void DrawQuad(const glm::vec2& Pos, const glm::vec2& Size, 
							 const glm::vec4& Color = {1.0f, 1.0f, 1.0f, 1.0f}, float RotationDeg = 0.0f);

		static void SetClearColor(const glm::vec4& InClearColor) { ClearColor = InClearColor; }

	private:
		inline static glm::vec4 ClearColor{ 0.20f, 0.20f, 0.20f, 1.0f };
		inline static std::unique_ptr<CImGuiLayer> ImGuiLayer = nullptr;
	};

}