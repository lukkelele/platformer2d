#include "keyboard.h"

#include "core/log.h"
#include "core/window.h"

namespace platformer2d {
	
	namespace
	{
		GLFWwindow* ActiveWindow = nullptr;
	}

	void CKeyboard::Initialize()
	{
		ActiveWindow = CWindow::Get()->GetGlfwWindow();
		LK_VERIFY(ActiveWindow);
	}

	void CKeyboard::Update()
	{
		/* Held keys. */
		for (auto& [Key, HeldData] : KeyHeldMap)
		{
			KeyDataMap[Key].RepeatCount++;
			HeldData.second = std::chrono::steady_clock::now();
		}
	}

	void CKeyboard::TransitionPressedKeys()
	{
		for (const auto& [Key, KeyData] : KeyDataMap)
		{
			if (KeyData.State == EKeyState::Pressed)
			{
				LK_TRACE("Transition: {} -> Held", Key);
				UpdateKeyState(Key, EKeyState::Held);
			}
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

	bool CKeyboard::IsKeyHeld(const EKey Key)
	{
		return ((KeyDataMap.find(Key) != KeyDataMap.end()) && (KeyDataMap[Key].State == EKeyState::Held));
	}

	bool CKeyboard::IsAnyKeysDown(std::span<const EKey> Keys)
	{
		if (!ActiveWindow)
		{
			return false;
		}

		for (const EKey Key : Keys)
		{
			const int KeyState = glfwGetKey(ActiveWindow, static_cast<int32_t>(Key));
			if ((KeyState == GLFW_PRESS) || (KeyState == GLFW_REPEAT))
			{
				return true;
			}
		}

		return false;
	}

	bool CKeyboard::IsAnyKeysDown(std::span<const EKey> Keys, std::vector<EKey>& Result)
	{
		if (!ActiveWindow)
		{
			return false;
		}

		Result.clear();
		for (const EKey Key : Keys)
		{
			const int KeyState = glfwGetKey(ActiveWindow, static_cast<int32_t>(Key));
			if ((KeyState == GLFW_PRESS) || (KeyState == GLFW_REPEAT))
			{
				Result.push_back(Key);
			}
		}

		return !Result.empty();
	}

	bool CKeyboard::IsAnyKeysDown(const EKey* KeysArray, const std::size_t N)
	{
		LK_ASSERT(KeysArray && (N > 0));
		if (!ActiveWindow)
		{
			return false;
		}

		for (int Idx = 0; Idx < N; Idx++)
		{
			const int KeyState = glfwGetKey(ActiveWindow, static_cast<int32_t>(KeysArray[Idx]));
			if ((KeyState == GLFW_PRESS) || (KeyState == GLFW_REPEAT))
			{
				return true;
			}
		}

		return false;
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
		LK_TRACE("{}: {} -> {}", Key, KeyData.OldState, KeyData.State);

		if (NewState == EKeyState::Pressed)
		{
			KeyData.RepeatCount = 0;
		}
		else if (NewState == EKeyState::Released)
		{
			/* Remove held key data whenever key is released. */
			std::erase_if(KeyHeldMap, [Key](const auto& CurrentKey) { return (Key == CurrentKey.first); });
		}
		else if ((NewState == EKeyState::Held) && (KeyData.RepeatCount <= 1))
		{
			using namespace std::chrono;
			/* Insert timestamp once on the initial held event. */
			KeyHeldMap.insert({ Key, { steady_clock::now(), steady_clock::now() } });
		}

		OnKeyPressed.Broadcast(KeyData);

		return KeyData;
	}

}