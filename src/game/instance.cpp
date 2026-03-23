#include "instance.h"

#include "core/window.h"

namespace platformer2d {

	CGameInstance::CGameInstance(CGameInstance* InstanceRef, const FGameSpecification& InSpec)
		: CLayer(InSpec.InstanceName)
		, Spec(InSpec)
	{
		Instance = InstanceRef;
		LK_VERIFY(Instance, "Invalid game instance reference");

		UpdateViewportBounds();
	}

	CGameInstance::~CGameInstance()
	{
		Instance = nullptr;
	}

	glm::vec2 CGameInstance::GetMouseInViewportSpace()
	{
		auto [MouseX, MouseY] = CMouse::GetPos();
		MouseX -= ViewportBounds[0].x;
		MouseY -= ViewportBounds[0].y;
		const float ViewportWidth = ViewportBounds[1].x - ViewportBounds[0].x;
		const float ViewportHeight = ViewportBounds[1].y - ViewportBounds[0].y;

		return glm::vec2(
			(MouseX / static_cast<float>(ViewportWidth)) * 2.0f - 1.0f,
			((MouseY / static_cast<float>(ViewportHeight)) * 2.0f - 1.0f) * -1.0f);
	}

	glm::vec2 CGameInstance::GetMouseInWorldSpace(const CCamera& Camera)
	{
		const glm::vec2 MousePos = GetMouseInViewportSpace();
		if ((MousePos.x < -1.0f) || (MousePos.x > 1.0f) || (MousePos.y < -1.0f) || (MousePos.y > 1.0f)) {
			return glm::vec2(std::numeric_limits<float>::quiet_NaN());
		}

		const glm::vec4 ClipPos = glm::vec4(MousePos.x, MousePos.y, 0.0f, 1.0f);
		const glm::mat4 InvViewProj = glm::inverse(Camera.GetProjectionMatrix() * Camera.GetViewMatrix());
		glm::vec4 WorldPos = InvViewProj * ClipPos;
		if (WorldPos.w != 0.0f) {
			WorldPos /= WorldPos.w;
		}

		return WorldPos;
	}

	void CGameInstance::UpdateViewportBounds()
	{
		ViewportBounds[0] = {0.0f, 0.0f};
		if (CWindow* Window = CWindow::Get(); Window != nullptr) {
			ViewportBounds[1] = Window->GetSize();
		} else {
			LK_WARN_TAG("GameInstance", "Cannot update viewport bounds");
			ViewportBounds[1] = {0.0f, 0.0f};
		}
	}

}

