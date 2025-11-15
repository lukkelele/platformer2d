#include "assetmanager.h"

namespace platformer2d {

	CAssetManager::CAssetManager()
	{
	}

	CAssetManager& CAssetManager::Get()
	{
		static CAssetManager Instance;
		return Instance;
	}

	void CAssetManager::Initialize()
	{
		LK_VERIFY(bInitialized == false, "Initialize called multiple times");
		LK_TRACE_TAG("AssetManager", "Initialize");

		bInitialized = true;
	}

}
