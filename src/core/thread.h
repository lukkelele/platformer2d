#pragma once

#include <atomic>
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

	enum class EThreadState : std::uint8_t
	{
		None,
		Idle,
		Busy,
		COUNT
	};

	enum class EThreadPolicy
	{
		Detach,
		Join
	};

	class CThread;

	struct FThreadEntry;

	namespace Thread {
		void Init();
		std::size_t GetID() noexcept;
		FThreadEntry& GetEntry(EThread type);
		CThread& Get(EThread type);
	}

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

		~CThread();
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

		void Run(EThreadPolicy InPolicy = EThreadPolicy::Detach);
		bool IsRunning() const { return bIsRunning; }

	private:
		std::thread WorkerThread;
		std::optional<std::function<void()>> Task;
		std::atomic_bool bIsRunning = false;
		EThreadPolicy Policy = EThreadPolicy::Detach;
	};

	struct FThreadEntry
	{
		std::thread::id ID;
		std::string_view Name;
		CThread Thread;
	};
}

namespace platformer2d::Enum {
	constexpr const char* ToString(const EThread type)
	{
		switch (type) {
			case EThread::Main:     return "Main";
			case EThread::Renderer: return "Renderer";
			case EThread::COUNT:    return "COUNT";
		}
		return nullptr;
	}

	constexpr const char* ToString(const EThreadState state)
	{
		switch (state) {
			case EThreadState::None:  return "None";
			case EThreadState::Idle:  return "Idle";
			case EThreadState::Busy:  return "Busy";
			case EThreadState::COUNT: return "COUNT";
		}
		return nullptr;
	}
}
