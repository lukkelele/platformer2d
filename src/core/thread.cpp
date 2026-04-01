#include "thread.h"

#include <unordered_map>

namespace platformer2d::Core {

	static std::array<FThreadEntry, std::to_underlying(EThread::COUNT)> ThreadMap{};

	void Thread::Init()
	{
		for (std::size_t Idx = 0; Idx < std::to_underlying(EThread::COUNT); Idx++) {
			auto& Entry = ThreadMap.at(Idx);
			Entry.Name = Enum::ToString(static_cast<EThread>(Idx));
		}
	}

	std::size_t Thread::GetID() noexcept
	{
		static std::size_t ThreadIdx = 0;
		static std::mutex Mutex;
		static std::unordered_map<std::thread::id, std::size_t> ThreadIDs;

		std::scoped_lock Lock(Mutex);
		const std::thread::id ID = std::this_thread::get_id();
		auto Iter = ThreadIDs.find(ID);
		if (Iter == ThreadIDs.end()) {
			Iter = ThreadIDs.insert({ID, ThreadIdx++}).first;
		}

		return Iter->second;
	}

	FThreadEntry& Thread::GetEntry(const EThread type)
	{
		return ThreadMap.at(std::to_underlying(type));
	}

	CThread<EThreadPolicy::Detach>& Thread::Get(const EThread type)
	{
		return ThreadMap.at(std::to_underlying(type)).Thread;
	}

}
