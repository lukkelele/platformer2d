#pragma once

namespace platformer2d {

	class CGameInstance;

	class IGameSystem
	{
	protected:
		IGameSystem() = default;

	public:
		IGameSystem(IGameSystem&&) = delete;
		IGameSystem(const IGameSystem&) = delete;
		virtual ~IGameSystem() = default;

		IGameSystem& operator=(IGameSystem&&) = delete;
		IGameSystem& operator=(const IGameSystem&) = delete;

		virtual void Initialize(CGameInstance& Owner) = 0;
		virtual void Shutdown() = 0;

	protected:
		CGameInstance* OwnerRef = nullptr;
	};

}

