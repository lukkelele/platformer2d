#include "keyboard.h"

#include "core/window.h"

namespace platformer2d {
	
	static GLFWwindow* ActiveWindow = nullptr;

	void CKeyboard::Initialize()
	{
		ActiveWindow = CWindow::Get()->GetGlfwWindow();
		LK_ASSERT(ActiveWindow, "No active window");
	}

	void CKeyboard::Update()
	{
		/* Update held data. */
		for (auto& [Key, HeldData] : KeyHeldMap)
		{
			KeyDataMap[Key].RepeatCount++;
			HeldData.second = std::chrono::steady_clock::now();
		}
	}

	void CKeyboard::SetActiveContext(GLFWwindow* Context)
	{
		ActiveWindow = Context;
	}

	bool CKeyboard::IsKeyDown(const EKey Key)
	{
		if (!ActiveWindow)
		{
			return false;
		}

		const int KeyState = glfwGetKey(ActiveWindow, static_cast<int32_t>(Key));
		return ((KeyState == GLFW_PRESS) || (KeyState == GLFW_REPEAT));
	}

	FKeyData& CKeyboard::GetKeyData(const EKey Key)
	{
		LK_ASSERT(KeyDataMap.contains(Key), "Key '{}' is not in the map", Enum::ToString(Key));
		return KeyDataMap[Key];
	}

	std::size_t CKeyboard::GetPressedKeys(std::vector<EKey>& InKeys)
	{
		InKeys.clear();
		InKeys.reserve(KeyDataMap.size());
		for (const auto& [Key, KeyData] : KeyDataMap)
		{
			if ((KeyData.State == EKeyState::Pressed) || (KeyData.State == EKeyState::Held))
			{
				InKeys.emplace_back(Key);
			}
		}

		InKeys.shrink_to_fit();
		return InKeys.size();
	}

	FKeyData& CKeyboard::UpdateKeyState(const EKey Key, const EKeyState NewState)
	{
		FKeyData& KeyData = KeyDataMap[Key];
		KeyData.Key = Key;
		KeyData.OldState = KeyData.State;
		KeyData.State = NewState;

		if (NewState == EKeyState::Pressed)
		{
			KeyData.RepeatCount = 0;
		}
		else if (NewState == EKeyState::Released)
		{
			/* Remove any held key data whenever key is released. */
			std::erase_if(KeyHeldMap, [Key](const auto& CurrentKey) { return (Key == CurrentKey.first); });
		}
		/* Insert timestamp once on the initial held event. */
		else if ((NewState == EKeyState::Held) && (KeyData.RepeatCount <= 1))
		{
			using namespace std::chrono;
			KeyHeldMap.insert({ Key, { steady_clock::now(), steady_clock::now() } });
		}

		return KeyData;
	}

}