#pragma once

#include <unordered_map>

#include "core/core.h"

namespace platformer2d {

	class CAssetManager
	{
	public:
		CAssetManager();
		virtual ~CAssetManager() = default;

		static CAssetManager& Get();

		void Initialize();

	private:
		bool bInitialized = false;
	};

}
