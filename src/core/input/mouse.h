#pragma once

#include "core/core.h"
#include "core/delegate.h"

#include "mousecodes.h"

namespace platformer2d {

	struct FMouseButtonData
	{
		EMouseButton Button = EMouseButton::None;
		EMouseButtonState State = EMouseButtonState::None;
		EMouseButtonState OldState = EMouseButtonState::None;
	};

	class CMouse
	{
	public:
		LK_DECLARE_EVENT(FOnButtonEvent, CMouse, const FMouseButtonData&);
		LK_DECLARE_EVENT(FOnScrollEvent, CMouse, EMouseScrollDirection);

	public:
		CMouse() = delete;
		~CMouse() = delete;
		CMouse(CMouse&&) = delete;
		CMouse(const CMouse&) = delete;

		CMouse& operator=(CMouse&&) = delete;
		CMouse& operator=(const CMouse&) = delete;

		static void Initialize();
		static void Enable();
		static void Disable();

		[[nodiscard]] static bool IsDown(EMouseButton Button);
		[[nodiscard]] static EMouseButtonState GetState(EMouseButton Button);
		static FMouseButtonData& UpdateButtonState(EMouseButton Button, EMouseButtonState NewState);
		static void UpdateScrollState(EMouseScrollDirection Direction);

		[[nodiscard]] static float GetX();
		[[nodiscard]] static float GetY();
		[[nodiscard]] static std::pair<float, float> GetPos(); /* @todo: Change to glm::vec2 */

	public:
		static inline FOnButtonEvent OnButtonEvent;
		static inline FOnScrollEvent OnScrollEvent;

	private:
		static inline std::map<EMouseButton, FMouseButtonData> ButtonDataMap{};
	};

}
