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
		LK_DECLARE_EVENT(FOnButtonPressed, CMouse, const FMouseButtonData&);
		LK_DECLARE_EVENT(FOnScrolled, CMouse, EMouseScrollDirection);
	public:
		CMouse() = delete;
		~CMouse() = delete;
		CMouse(const CMouse&) = delete;
		CMouse(CMouse&&) = delete;

		static void Initialize();
		static void Enable();
		static void Disable();

		static FMouseButtonData& UpdateButtonState(EMouseButton Button, EMouseButtonState NewState);
		static void UpdateScrollState(EMouseScrollDirection Direction);

		static float GetX();
		static float GetY();
		static std::pair<float, float> GetPos();

	private:
		CMouse& operator=(const CMouse&) = delete;
		CMouse& operator=(CMouse&&) = delete;

	public:
		static inline FOnButtonPressed OnButtonPressed;
		static inline FOnScrolled OnScrolled;
	private:
		static inline std::map<EMouseButton, FMouseButtonData> ButtonDataMap{};
	};

}