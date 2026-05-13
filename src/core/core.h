#pragma once

#include "lk_config.h"

#include <array>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <string_view>
#include <source_location>
#include <span>
#include <type_traits>
#include <queue>
#include <utility>

#include <glm/glm.hpp>

#include "assert.h"
#include "enum.h"
#include "log.h"
#include "macros.h"
#include "uuid.h"

#define LK_MARK_NOT_IMPLEMENTED()                                   \
	LK_VERIFY(false, "Not implemented: {}::{}", __FILE__, __LINE__)

#define LK_THROW_ENUM_ERR(EnumValue)                                                                                              \
	LK_VERIFY(false, "{} failed with value: {}", std::source_location::current().function_name(), std::to_underlying(EnumValue));

namespace platformer2d {
	using namespace std::chrono_literals;

	class CApplication;
	class CLayerStack;

	using LRendererID = std::uint32_t;

	namespace Core {
		static const std::filesystem::path ProjectDir = std::filesystem::weakly_canonical(PROJECT_DIR);

		enum class ELayer
		{
			Runtime,
			Editor,
			COUNT
		};
		LK_ENUM(ELayer);

		struct FGlobal
		{
			bool bShouldShutdown = false;

			void AddLayer(const ELayer Layer) { LayerAddQueue.push(Layer); }
			void RemoveLayer(const ELayer Layer) { LayerRemoveQueue.push(Layer); }
			void RemoveAllLayers()
			{
				for (std::size_t EnumValue = 0; EnumValue < std::to_underlying(ELayer::COUNT); EnumValue++) {
					LayerRemoveQueue.push(static_cast<ELayer>(EnumValue));
				}
			}

			/**
			 * @brief Get the number of layers in the layerstack.
			 */
			int GetLayerStackSize() const { return LayerStackSize; }

		private:
			/* Layers to add/remove to the application layerstack. */
			std::queue<ELayer> LayerAddQueue;
			std::queue<ELayer> LayerRemoveQueue;
			int LayerStackSize = 0; /* Managed by CLayerStack. */

			friend class ::platformer2d::CApplication;
			friend class ::platformer2d::CLayerStack;
		};

		extern FGlobal Global;

		const char* GetPlatformName();

		/**
		 * @brief Get relative path from the project directory.
		 */
		std::filesystem::path GetRelativeFromProject(const std::filesystem::path& Input);

		int ParseSvgPath(std::string_view, const glm::vec2& Offset,
			std::span<glm::vec2> Points, float Scale, bool ReverseOrder);

		/**
		 * @brief Run asynchronously.
		 */
		template<typename F, typename... TArgs>
		static void RunDetachedAfter(const std::chrono::steady_clock::duration& Delay, F&& Func, TArgs&&... Args)
		{
			std::thread(
				[Delay, Func = std::forward<F>(Func), ... Args = std::forward<TArgs>(Args)]()
			{
				std::this_thread::sleep_for(Delay);
				Func(std::forward<TArgs>(Args)...);
			}).detach();
		}
	}

	enum class EDirection
	{
		Up,
		Down,
		Left,
		Right,
		COUNT
	};
	LK_ENUM(EDirection);

	enum class EQuickLoad : std::uint8_t
	{
		None,
		Editor,
		COUNT
	};
	LK_ENUM(EQuickLoad);

}
