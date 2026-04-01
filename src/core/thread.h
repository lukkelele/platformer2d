#pragma once

#include <cassert>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>

#include <lklog/lklog.h>

namespace platformer2d {
	enum class EThread : std::uint8_t
	{
		Main,
		Renderer,
		COUNT
	};

	namespace Enum {
		constexpr const char* ToString(const EThread type)
		{
			switch (type) {
				case EThread::Main:     return "Main";
				case EThread::Renderer: return "Renderer";
				case EThread::COUNT:    return "COUNT";
			}
			return nullptr;
		}
	}
}

namespace platformer2d::Core {

	using FThreadHandle = std::size_t;

	enum class EThreadPolicy
	{
		Detach,
		Join
	};

	template<EThreadPolicy ThreadPolicy>
	class CThread;

	struct FThreadEntry;

	namespace Thread {
		void Init();
		std::size_t GetID() noexcept;
		FThreadEntry& GetEntry(EThread type);
		CThread<EThreadPolicy::Detach>& Get(EThread type); /* @fixme: Some neat way to return CThread references without caring about policy */
	}

	template<EThreadPolicy ThreadPolicy = EThreadPolicy::Detach>
	class CThread
	{
	public:
		CThread() = default;

		template<typename TCallable, typename... TArgs>
		CThread(TCallable&& InFunc, TArgs&&... InArgs)
		{
			Task = [Func = std::forward<TCallable>(InFunc), ... Args = std::forward<TArgs>(InArgs)]() mutable
			{
				std::invoke(Func, std::move(Args)...);
			};
		}

		~CThread()
		{
			if (Policy == EThreadPolicy::Detach && (WorkerThread.joinable() || bIsRunning)) {
				LKLOG_ERROR_TAG("Thread", "Detached worker is still joinable");
				WorkerThread.join();
			} else if (Policy == EThreadPolicy::Join && (WorkerThread.joinable() || bIsRunning)) {
				LKLOG_ERROR_TAG("Thread", "Worker is still joinable");
				WorkerThread.join();
			}
		}

		CThread(CThread&&) = delete;
		CThread(const CThread&) = delete;
		CThread& operator=(CThread&&) = delete;
		CThread& operator=(const CThread&) = delete;

		template<typename TCallable, typename... TArgs>
		void Setup(TCallable&& InFunc, TArgs&&... InArgs)
		{
			Task = [Func = std::forward<TCallable>(InFunc), ... Args = std::forward<TArgs>(InArgs)]() mutable
			{
				std::invoke(Func, std::move(Args)...);
			};
		}

		void Run()
		{
			assert(Task.has_value() && "Thread has no task");
			assert(!WorkerThread.joinable() && "Thread already running");

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

	private:
		std::thread WorkerThread;
		std::optional<std::function<void()>> Task;
		std::atomic_bool bIsRunning = false;
		const EThreadPolicy Policy = ThreadPolicy;
	};

	struct FThreadEntry
	{
		std::string_view Name;
		CThread<EThreadPolicy::Detach> Thread;
	};

}
