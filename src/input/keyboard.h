#pragma once

#include <chrono>
#include <map>
#include <vector>

#include "keycodes.h"

struct GLFWwindow;

namespace platformer2d {

	struct FKeyData
	{
		EKey Key{};
		EKeyState State = EKeyState::None;
		EKeyState OldState = EKeyState::None;
		int RepeatCount = 0;
	};

	class CKeyboard
	{
	public:
		CKeyboard() = delete;
		~CKeyboard() = default;
		CKeyboard(const CKeyboard&) = delete;
		CKeyboard(CKeyboard&&) = delete;

		static void Initialize();
		static void Update();
		static void TransitionPressedKeys();
		static void SetActiveContext(GLFWwindow* Context);

		static bool IsKeyDown(EKey Key);
		static bool IsKeyHeld(EKey Key);

		static FKeyData& GetKeyData(EKey Key);
		static std::size_t GetPressedKeys(std::vector<EKey>& InKeys);
		static FKeyData& UpdateKeyState(EKey Key, EKeyState NewState);

		template<typename TDuration>
		static TDuration GetKeyHeldTime(const EKey Key)
		{
			using namespace std::chrono;
			if (!KeyHeldMap.contains(Key))
			{
				return 0s;
			}

			return duration_cast<TDuration>(KeyHeldMap.at(Key).second - KeyHeldMap.at(Key).first);
		}

	private:
		CKeyboard& operator=(const CKeyboard&) = delete;
		CKeyboard& operator=(CKeyboard&&) = delete;

	private:
		static inline std::map<EKey, FKeyData> KeyDataMap{};

		using FKeyHeldData = std::pair<std::chrono::steady_clock::time_point, std::chrono::steady_clock::time_point>;
		static inline std::map<EKey, FKeyHeldData> KeyHeldMap{};
	};

}
