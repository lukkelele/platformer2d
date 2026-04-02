#include "thread.h"

#include <unordered_map>

namespace platformer2d {

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

	CThread& Thread::Get(const EThread type)
	{
		return ThreadMap.at(std::to_underlying(type)).Thread;
	}

	CThread::~CThread()
	{
		if (Policy == EThreadPolicy::Detach && (WorkerThread.joinable() || bIsRunning)) {
			LKLOG_ERROR_TAG("Thread", "Detached worker is still joinable");
			WorkerThread.join();
		} else if (Policy == EThreadPolicy::Join && (WorkerThread.joinable() || bIsRunning)) {
			LKLOG_ERROR_TAG("Thread", "Worker is still joinable");
			WorkerThread.join();
		}
	}

	void CThread::Run(const EThreadPolicy InPolicy)
	{
		assert(Task.has_value() && "Thread has no task");
		assert(!WorkerThread.joinable() && "Thread already running");
		Policy = InPolicy;

		WorkerThread = std::thread([this]
		{
			bIsRunning = true;
			LKLOG_TRACE_TAG("Thread", "{}: Executing task", Thread::GetID());
			(*Task)();
			bIsRunning = false;
		});

		switch (Policy) {
			case EThreadPolicy::Detach:
				WorkerThread.detach();
				break;
			case EThreadPolicy::Join:
				WorkerThread.join();
				break;
		}
	}

}
