/******************************************************************
 * @file
 * @description CDelegate and CMulticastDelegate
 * 
 * Implementations for single and multicast delegates.
 *
 * The creation of a delegate should use the following macros:
 *  - LK_DECLARE_DELEGATE
 *  - LK_DECLARE_MULTICAST_DELEGATE
 *  - LK_DECLARE_DELEGATE_RET
 *  - LK_DECLARE_EVENT
 *******************************************************************/
#pragma once

#include <memory>
#include <vector>

#include "assert.h"
#include "core.h"
#include "template.h"

#define LK_DECLARE_DELEGATE(DelegateName, ...) \
	using DelegateName = ::platformer2d::Core::CDelegate<#DelegateName, void __VA_OPT__(,__VA_ARGS__)>

#define LK_DECLARE_DELEGATE_RET(DelegateName, ReturnValue, ...) \
	using DelegateName = ::platformer2d::Core::CDelegate<#DelegateName, ReturnValue __VA_OPT__(,__VA_ARGS__)>

#define LK_DECLARE_MULTICAST_DELEGATE(DelegateName, ...) \
	using DelegateName = ::platformer2d::Core::CMulticastDelegate<__VA_ARGS__>; \
	using DelegateName ## _DelegateType = ::platformer2d::Core::CMulticastDelegate<__VA_ARGS__>::TDelegate

#define LK_DECLARE_EVENT(EventName, EventOwner, ...) \
	class EventName : public ::platformer2d::Core::CMulticastDelegate<__VA_ARGS__> \
	{ \
	private: \
		friend class EventOwner; \
		using ::platformer2d::Core::CMulticastDelegate<__VA_ARGS__>::Broadcast; \
		using ::platformer2d::Core::CMulticastDelegate<__VA_ARGS__>::RemoveAll; \
		using ::platformer2d::Core::CMulticastDelegate<__VA_ARGS__>::Remove; \
	};

namespace platformer2d::Core {

	namespace DelegateCore
	{
		static constexpr std::size_t BUFSIZE_NAME = 100;

		/**
		 * Allocation size for the max bound objects on the stack.
		 * Larger sizes will result in heap allocation instead.
		 */
		static constexpr int INLINE_ALLOCATION_SIZE = 48;

		static void* (*Alloc)(std::size_t Size) = [](std::size_t Size) 
		{ 
			return std::malloc(Size); 
		};

		static void(*Free)(void* InHeapPointer) = [](void* InHeapPointer) 
		{ 
			std::free(InHeapPointer); 
		};
	}

	namespace DelegateMemory
	{
		using FAllocateCallback = void*(*)(std::size_t Size);
		using FFreeCallback = void(*)(void* HeapPointer);

		inline void SetAllocationCallbacks(FAllocateCallback AllocateCallback, FFreeCallback FreeCallback)
		{
			DelegateCore::Alloc = AllocateCallback;
			DelegateCore::Free = FreeCallback;
		}
	}

	/**
	 * @class IDelegateBase
	 * @brief Base delegate interface.
	 */
	class IDelegateBase
	{
	public:
		IDelegateBase() = default;
		virtual ~IDelegateBase() noexcept = default;

		virtual const void* GetOwner() const { return nullptr; }
		virtual void Clone(void* Destination) = 0;
	};

	/** 
	 * @class IDelegate 
	 * @brief Delegate interface.
	 */
	template<typename TReturnValue, typename... TArgs>
	class IDelegate : public IDelegateBase
	{
	public:
		virtual TReturnValue Execute(TArgs&&... Args) = 0;
	};


	/** 
	 * @class CStaticDelegate
	 */
	template<typename TReturnValue, typename... Args2>
	class CStaticDelegate;

	/** 
	 * @class CStaticDelegate
	 * @brief Delegate specialization.
	 */
	template<typename TReturnValue, typename... TArgs, typename... Args2>
	class CStaticDelegate<TReturnValue(TArgs...), Args2...> : public IDelegate<TReturnValue, TArgs...>
	{
	public:
		using DelegateFunction = TReturnValue(*)(TArgs..., Args2...);

		CStaticDelegate(DelegateFunction InFunction, Args2&&... InPayload)
			: Function(InFunction)
			, Payload(std::forward<Args2>(InPayload)...)
		{
		}

		CStaticDelegate(DelegateFunction InFunction, const std::tuple<Args2...>& InPayload)
			: Function(InFunction)
			, Payload(InPayload)
		{
		}

		virtual inline TReturnValue Execute(TArgs&&... Args) override
		{
			return Execute_Internal(std::forward<TArgs>(Args)..., std::index_sequence_for<Args2...>());
		}

		virtual void Clone(void* Destination) override
		{
			new (Destination) CStaticDelegate(Function, Payload);
		}

	private:
		template<std::size_t... Is>
		inline TReturnValue Execute_Internal(TArgs&&... Args, std::index_sequence<Is...>)
		{
			return Function(std::forward<TArgs>(Args)..., std::get<Is>(Payload)...);
		}

		DelegateFunction Function;
		std::tuple<Args2...> Payload;
	};


	/**
	 * @class CRawDelegate
	 */
	template<bool bIsConst, typename T, typename TReturnValue, typename... Args2>
	class CRawDelegate;

	/**
	 * @class CRawDelegate
	 * @brief Template specilization.
	 */
	template<bool bIsConst, typename T, typename TReturnValue, typename... TArgs, typename... Args2>
	class CRawDelegate<bIsConst, T, TReturnValue(TArgs...), Args2...> : public IDelegate<TReturnValue, TArgs...>
	{
	public:
		using DelegateFunction = typename Core::MemberFunction<bIsConst, T, TReturnValue, TArgs..., Args2...>::type;

		CRawDelegate(T* InObjectRef, DelegateFunction InFunction, Args2&&... InPayload)
			: ObjectRef(InObjectRef)
			, Function(InFunction)
			, Payload(std::forward<Args2>(InPayload)...)
		{
			LK_ASSERT(InObjectRef, "Passed object is nullptr");
			LK_ASSERT(InFunction, "Passed function is invalid");
		}

		CRawDelegate(T* InObjectRef, DelegateFunction InFunction, const std::tuple<Args2...>& InPayload)
			: ObjectRef(InObjectRef)
			, Function(InFunction)
			, Payload(InPayload)
		{
			LK_ASSERT(InObjectRef, "Passed object is nullptr");
			LK_ASSERT(InFunction, "Passed function is invalid");
		}

		virtual inline TReturnValue Execute(TArgs&&... Args) override
		{
			return Execute_Internal(std::forward<TArgs>(Args)..., std::index_sequence_for<Args2...>());
		}

		virtual const void* GetOwner() const override { return ObjectRef; }

		virtual void Clone(void* Destination) override
		{
			new (Destination) CRawDelegate(ObjectRef, Function, Payload);
		}

	private:
		template<std::size_t... Is>
		inline TReturnValue Execute_Internal(TArgs&&... Args, std::index_sequence<Is...>)
		{
			return (ObjectRef->*Function)(std::forward<TArgs>(Args)..., std::get<Is>(Payload)...);
		}

		T* ObjectRef = nullptr;
		DelegateFunction Function;
		std::tuple<Args2...> Payload;
	};


	/**
	 * @class CLambdaDelegate
	 */
	template<typename TLambda, typename TReturnValue, typename... TArgs>
	class CLambdaDelegate;

	/**
	 * @class CLambdaDelegate
	 * @brief Template specialization.
	 */
	template<typename TLambda, typename TReturnValue, typename... TArgs, typename... Args2>
	class CLambdaDelegate<TLambda, TReturnValue(TArgs...), Args2...> : public IDelegate<TReturnValue, TArgs...>
	{
	public:
		explicit CLambdaDelegate(TLambda&& InLambda, Args2&&... InPayload)
			: Lambda(std::forward<TLambda>(InLambda))
			, Payload(std::forward<Args2>(InPayload)...)
		{
		}

		explicit CLambdaDelegate(const TLambda& InLambda, const std::tuple<Args2...>& InPayload)
			: Lambda(InLambda)
			, Payload(InPayload)
		{
		}

		inline TReturnValue Execute(TArgs&&... Args) override
		{
			return Execute_Internal(std::forward<TArgs>(Args)..., std::index_sequence_for<Args2...>());
		}

		virtual void Clone(void* Destination) override
		{
			new (Destination) CLambdaDelegate(Lambda, Payload);
		}

	private:
		template<std::size_t... Is>
		inline TReturnValue Execute_Internal(TArgs&&... Args, std::index_sequence<Is...>)
		{
			return (TReturnValue)((Lambda)(std::forward<TArgs>(Args)..., std::get<Is>(Payload)...));
		}

		TLambda Lambda;
		std::tuple<Args2...> Payload;
	};


	/**
	 * @class CSharedPtrDelegate
	 * @brief Shared pointer template specialization.
	 */
	template<bool bIsConst, typename T, typename TReturnValue, typename... TArgs>
	class CSharedPtrDelegate;

	template<bool bIsConst, typename TReturnValue, typename T, typename... TArgs, typename... Args2>
	class CSharedPtrDelegate<bIsConst, T, TReturnValue(TArgs...), Args2...> : public IDelegate<TReturnValue, TArgs...>
	{
	public:
		using DelegateFunction = typename Core::MemberFunction<bIsConst, T, TReturnValue, TArgs..., Args2...>::type;

		CSharedPtrDelegate(std::shared_ptr<T> InObjectRef, 
						   const DelegateFunction InFunction, 
						   Args2&&... InPayload)
			: ObjectRef(InObjectRef)
			, Function(InFunction)
			, Payload(std::forward<Args2>(InPayload)...)
		{
		}

		CSharedPtrDelegate(std::weak_ptr<T> InObjectRef, 
						   const DelegateFunction InFunction, 
						   const std::tuple<Args2...>& InPayload)
			: ObjectRef(InObjectRef)
			, Function(InFunction)
			, Payload(InPayload)
		{
		}

		virtual inline TReturnValue Execute(TArgs&&... Args) override
		{
			return Execute_Internal(std::forward<TArgs>(Args)..., std::index_sequence_for<Args2...>());
		}

		virtual const void* GetOwner() const override
		{
			return (ObjectRef.expired() ? nullptr : ObjectRef.lock().get());
		}

		virtual void Clone(void* Destination) override
		{
			new (Destination) CSharedPtrDelegate(ObjectRef, Function, Payload);
		}

	private:
		template<std::size_t... Is>
		TReturnValue Execute_Internal(TArgs&&... Args, std::index_sequence<Is...>)
		{
			if (ObjectRef.expired())
			{
				return TReturnValue();
			}
			else
			{
				std::shared_ptr<T> Object = ObjectRef.lock();
				return (Object.get()->*Function)(std::forward<TArgs>(Args)..., std::get<Is>(Payload)...);
			}
		}

		std::weak_ptr<T> ObjectRef;
		DelegateFunction Function;
		std::tuple<Args2...> Payload;
	};


	/**
	 * @struct FDelegateHandle
	 * @brief Unique identifier for delegate objects.
	 * Used in event systems and to more easily register and manage callbacks.
	 */
	struct FDelegateHandle
	{
	public:
		constexpr FDelegateHandle() noexcept : ID(NullID) {}
		explicit FDelegateHandle(bool) noexcept : ID(GetNewID()) {}
		~FDelegateHandle() noexcept = default;
		FDelegateHandle(const FDelegateHandle& Other) = default;
		FDelegateHandle& operator=(const FDelegateHandle& Other) = default;

		FDelegateHandle(FDelegateHandle&& Other) noexcept
			: ID(Other.ID)
		{
			Other.Reset();
		}

		FDelegateHandle& operator=(FDelegateHandle&& Other) noexcept
		{
			ID = Other.ID;
			Other.Reset();

			return *this;
		}

		inline operator bool() const noexcept { return IsValid(); }
		inline bool operator==(const FDelegateHandle& Other) const noexcept { return (ID == Other.ID); }
		inline bool operator<(const FDelegateHandle& Other) const noexcept { return (ID < Other.ID); }

		inline bool IsValid() const noexcept { return (ID != NullID); }
		inline void Reset() noexcept { ID = NullID; }

		inline static constexpr unsigned int NullID = std::numeric_limits<unsigned int>::max();

	private:
		unsigned int ID = 0;

		/** Count of the created and assigned ID's. */
		inline static unsigned int IDCounter = 0;

		static inline int GetNewID()
		{
			const unsigned int Output = FDelegateHandle::IDCounter++;
			if (FDelegateHandle::IDCounter == NullID)
			{
				FDelegateHandle::IDCounter = 0;
			}

			return Output;
		}
	};

	/**
	 * @class CInlineAllocator
	 * @brief Memory allocator that makes use of small memory allocations by 
	 * using a fixed-size stack buffer whenever the requested memory is
	 * small. Utilizes heap for larger allocations.
	 */
	template<size_t MaxStackSize>
	class CInlineAllocator
	{
	public:
		constexpr CInlineAllocator() noexcept
			: Size(0)
		{
			static_assert(MaxStackSize > sizeof(void*), "The stack size is too small");
		}

		~CInlineAllocator() noexcept { Free(); }

		CInlineAllocator(const CInlineAllocator& Other)
			: Size(0)
		{
			if (Other.HasAllocation())
			{
				/* Deep copy. */
				std::memcpy(Allocate(Other.Size), Other.GetAllocation(), Other.Size);
			}
			Size = Other.Size;
		}

		CInlineAllocator(CInlineAllocator&& Other) noexcept
			: Size(Other.Size)
		{
			Other.Size = 0;
			if (Size > MaxStackSize)
			{
				std::swap(HeapPointer, Other.HeapPointer);
			}
			else
			{
				std::memcpy(Buffer, Other.Buffer, Size);
			}
		}

		FORCEINLINE CInlineAllocator& operator=(const CInlineAllocator& Other)
		{
			if (Other.HasAllocation())
			{
				/* Deep copy. */
				std::memcpy(Allocate(Other.Size), Other.GetAllocation(), Other.Size);
			}
			Size = Other.Size;
			return *this;
		}

		FORCEINLINE CInlineAllocator& operator=(CInlineAllocator&& Other) noexcept
		{
			Free();

			Size = Other.Size;
			Other.Size = 0;

			if (Size > MaxStackSize)
			{
				std::swap(HeapPointer, Other.HeapPointer);
			}
			else
			{
				std::memcpy(Buffer, Other.Buffer, Size);
			}

			return *this;
		}

		/** 
		 * @brief Allocate memory of given size.
		 * Allocates on the heap if the size is larger than the max allowed stack size.
		 */
		FORCEINLINE void* Allocate(const size_t InSize)
		{
			if (Size != InSize)
			{
				Free();
				Size = InSize;

				if (InSize > MaxStackSize)
				{
					HeapPointer = DelegateCore::Alloc(InSize);
					return HeapPointer;
				}
			}

			return (void*)Buffer;
		}

		/** 
		 * @brief Release heap memory if any has been assigned.
		 */
		FORCEINLINE void Free()
		{
			if (Size > MaxStackSize)
			{
				DelegateCore::Free(HeapPointer);
			}
			Size = 0;
		}

		/** 
		 * @brief Return allocated memory.
		 */
		FORCEINLINE void* GetAllocation() const
		{
			if (!HasAllocation())
			{
				return nullptr;
			}

			return (HasHeapAllocation() ? HeapPointer : (void*)Buffer);
		}

		FORCEINLINE size_t GetSize() const { return Size; }
		FORCEINLINE bool HasAllocation() const { return (Size > 0); }
		FORCEINLINE bool HasHeapAllocation() const { return (Size > MaxStackSize); }

	private:
		union
		{
			char Buffer[MaxStackSize] = {};
			void* HeapPointer;
		};

		size_t Size = 0;
	};


	/**
	 * @class CDelegateBase
	 * @brief Base class for all delegate types.
	 */
	class CDelegateBase
	{
	public:
		constexpr CDelegateBase() noexcept : Allocator() {}
		virtual ~CDelegateBase() noexcept { Release(); }

		CDelegateBase(const CDelegateBase& Other)
		{
			if (Other.Allocator.HasAllocation())
			{
				Allocator.Allocate(Other.Allocator.GetSize());
				Other.GetDelegate()->Clone(Allocator.GetAllocation());
			}
		}

		CDelegateBase(CDelegateBase&& Other) noexcept : Allocator(std::move(Other.Allocator)) {}

		/**
		 * @brief Check if the delegate is bound.
		 */
		inline bool IsBound() const { return Allocator.HasAllocation(); }

	protected:
		CDelegateBase& operator=(const CDelegateBase& Other)
		{
			Release();
			if (Other.Allocator.HasAllocation())
			{
				Allocator.Allocate(Other.Allocator.GetSize());
				Other.GetDelegate()->Clone(Allocator.GetAllocation());
			}

			return *this;
		}

		CDelegateBase& operator=(CDelegateBase&& Other) noexcept
		{
			Release();
			Allocator = std::move(Other.Allocator);

			return *this;
		}

		const void* GetOwner() const
		{
			return (Allocator.HasAllocation() ? GetDelegate()->GetOwner() : nullptr);
		}

		inline size_t GetSize() const
		{
			return Allocator.GetSize();
		}

		void ClearIfBoundTo(void* InObject)
		{
			if (InObject && IsBoundTo(InObject))
			{
				Release();
			}
		}

		void Clear()
		{
			Release();
		}

		inline bool IsBoundTo(void* InObject) const
		{
			if (!InObject || !Allocator.HasAllocation())
			{
				return false;
			}

			return (GetDelegate()->GetOwner() == InObject);
		}

	protected:
		inline void Release()
		{
			if (Allocator.HasAllocation())
			{
				GetDelegate()->~IDelegateBase();
				Allocator.Free();
			}
		}

		IDelegateBase* GetDelegate() const
		{
			return static_cast<IDelegateBase*>(Allocator.GetAllocation());
		}

		CInlineAllocator<DelegateCore::INLINE_ALLOCATION_SIZE> Allocator;
	};


	/**
	 * @class CDelegate
	 * @brief Supports binding to all types of functions.
	 */
	template<typename TReturnValue, typename... TArgs>
	class CDelegate : public CDelegateBase
	{
	public:
		CDelegate() = default;
		~CDelegate() = default;

	private:
		template<typename T, typename... Args2>
		using ConstMemberFunction = typename Core::MemberFunction<true, T, TReturnValue, TArgs..., Args2...>::type;

		template<typename T, typename... Args2>
		using NonConstMemberFunction = typename Core::MemberFunction<false, T, TReturnValue, TArgs..., Args2...>::type;

		using TDelegateInterface = IDelegate<TReturnValue, TArgs...>;

	public:
		/**
		 * @brief Direct execution of the delegate.
		 */
		[[nodiscard]] inline TReturnValue Execute(TArgs... Args) const
		{
			LK_ASSERT(Allocator.HasAllocation(), "Delegate '{}' is not bound", typeid(this).name());
			return ((TDelegateInterface*)GetDelegate())->Execute(std::forward<TArgs>(Args)...);
		}

		/**
		 * @brief Attempt to execute the delegate if it bound, otherwise ignore.
		 * A safe way to execute.
		 */
		[[nodiscard]] inline TReturnValue ExecuteIfBound(TArgs... Args) const
		{
			if (IsBound())
			{
				return ((TDelegateInterface*)GetDelegate())->Execute(std::forward<TArgs>(Args)...);
			}

			return TReturnValue();
		}

	private:
		template<typename T, typename... TArgs2>
		[[nodiscard]] static CDelegate CreateRaw(T* InObject, NonConstMemberFunction<T, TArgs2...> InFunction, TArgs2... Args)
		{
			CDelegate Handler;
			Handler.Bind_Internal<CRawDelegate<false, T, TReturnValue(TArgs...), TArgs2...>>(
				InObject, 
				InFunction, 
				std::forward<TArgs2>(Args)...
			);
			
			return Handler;
		}

		template<typename T, typename... TArgs2>
		[[nodiscard]] static CDelegate CreateRaw(T* InObject, ConstMemberFunction<T, TArgs2...> InFunction, TArgs2... Args)
		{
			CDelegate Handler;
			Handler.Bind_Internal<CRawDelegate<true, T, TReturnValue(TArgs...), TArgs2...>>(
				InObject, 
				InFunction, 
				std::forward<TArgs2>(Args)...
			);
			return Handler;
		}

		template<typename... TArgs2>
		[[nodiscard]] static CDelegate CreateStatic(TReturnValue(*InFunction)(TArgs..., TArgs2...), TArgs2... Args)
		{
			CDelegate Handler;
			Handler.Bind_Internal<CStaticDelegate<TReturnValue(TArgs...), TArgs2...>>(
				InFunction, 
				std::forward<TArgs2>(Args)...
			);
			return Handler;
		}

		template<typename T, typename... TArgs2>
		[[nodiscard]] static CDelegate CreateShared(const std::shared_ptr<T>& ObjectRef, 
													 NonConstMemberFunction<T, TArgs2...> InFunction, 
													 TArgs2... Args)
		{
			CDelegate Handler;
			Handler.Bind_Internal<CSharedPtrDelegate<false, T, TReturnValue(TArgs...), TArgs2...>>(
				ObjectRef, 
				InFunction, 
				std::forward<TArgs2>(Args)...
			);

			return Handler;
		}

		template<typename T, typename... TArgs2>
		[[nodiscard]] static CDelegate CreateShared(const std::shared_ptr<T>& ObjectRef, 
													 ConstMemberFunction<T, TArgs2...> InFunction, 
													 TArgs2... Args)
		{
			CDelegate Handler;
			Handler.Bind_Internal<CSharedPtrDelegate<true, T, TReturnValue(TArgs...), TArgs2...>>(
				ObjectRef, 
				InFunction, 
				std::forward<TArgs2>(Args)...
			);

			return Handler;
		}

		template<typename TLambda, typename... TArgs2>
		[[nodiscard]] static CDelegate CreateLambda(TLambda&& InLambda, TArgs2... Args)
		{
			using LambdaType = std::decay_t<TLambda>;
			CDelegate Handler;
			Handler.Bind_Internal<CLambdaDelegate<LambdaType, TReturnValue(TArgs...), TArgs2...>>(
				std::forward<LambdaType>(InLambda), 
				std::forward<TArgs2>(Args)...
			);

			return Handler;
		}

	public:
		/* Bind: Raw, non-const member function. */
		template<typename T, typename... TArgs2>
		void Bind(T* ObjectRef, NonConstMemberFunction<T, TArgs2...> InFunction, TArgs2&&... Args)
		{
			static_assert(!std::is_const_v<T>, "Non-const function cannot be bound on a const object");
			*this = CreateRaw<T, TArgs2...>(ObjectRef, InFunction, std::forward<TArgs2>(Args)...);
		}

		/* Bind: Raw, const member function. */
		template<typename T, typename... TArgs2>
		void Bind(T* ObjectRef, ConstMemberFunction<T, TArgs2...> InFunction, TArgs2&&... Args)
		{
			*this = CreateRaw<T, TArgs2...>(ObjectRef, InFunction, std::forward<TArgs2>(Args)...);
		}

		/* Bind: Static. */
		template<typename... TArgs2>
		void Bind(TReturnValue(*InFunction)(TArgs..., TArgs2...), TArgs2&&... Args)
		{
			*this = CreateStatic<TArgs2...>(InFunction, std::forward<TArgs2>(Args)...);
		}

		/* Bind: Lambda. */
		template<typename LambdaType, typename... Args2>
		void Bind(LambdaType&& InLambda, Args2&&... args)
		{
			*this = CreateLambda<LambdaType, Args2...>(std::forward<LambdaType>(InLambda), std::forward<Args2>(args)...);
		}

		/* Bind: Shared Pointer, non-const member function. */
		template<typename T, typename... Args2>
		void Bind(std::shared_ptr<T> ObjectRef, NonConstMemberFunction<T, Args2...> InFunction, Args2&&... args)
		{
			static_assert(!std::is_const_v<T>, "Attempted to bind a non-const member function on a const object reference");
			*this = CreateShared<T, Args2...>(ObjectRef, InFunction, std::forward<Args2>(args)...);
		}

		/* Bind: Shared Pointer, const member function. */
		template<typename T, typename... Args2>
		void Bind(std::shared_ptr<T> ObjectRef, ConstMemberFunction<T, Args2...> InFunction, Args2&&... args)
		{
			*this = CreateShared<T, Args2...>(ObjectRef, InFunction, std::forward<Args2>(args)...);
		}

	private:
		template<typename T, typename... TBindArgs>
		inline void Bind_Internal(TBindArgs&&... Args)
		{
			Release();
			void* AllocPointer = Allocator.Allocate(sizeof(T));
			new (AllocPointer) T(std::forward<TBindArgs>(Args)...);
		}

	private:
		/** Allow access to the Create<TFunction> functions. */
	#if defined(LK_COMPILER_MSVC)
		template<typename... TArgs>
	#elif defined(LK_COMPILER_GCC) || defined(LK_COMPILER_CLANG)
		template<typename... TArgs2>
	#endif
		friend class CMulticastDelegate;
	};


	/**
	 * @class CMulticastDelegate
	 */
	template<typename... TArgs>
	class CMulticastDelegate : public CDelegateBase
	{
	private:
		template<typename T, typename... TArgs2>
		using ConstMemberFunction = typename Core::MemberFunction<true, T, void, TArgs..., TArgs2...>::type;

		template<typename T, typename... TArgs2>
		using NonConstMemberFunction = typename Core::MemberFunction<false, T, void, TArgs..., TArgs2...>::type;

	public:
		using TDelegate = CDelegate<void, TArgs...>;

		constexpr CMulticastDelegate() : Locks(0) {}
		~CMulticastDelegate() noexcept = default;

		CMulticastDelegate(const CMulticastDelegate& Other) = default;
		CMulticastDelegate(CMulticastDelegate&& Other) noexcept
			: Dispatchers(std::move(Other.Dispatchers))
			, Locks(std::move(Other.Locks))
		{
		}

		CMulticastDelegate& operator=(const CMulticastDelegate& Other) = default;

		CMulticastDelegate& operator=(CMulticastDelegate&& Other) noexcept
		{
			Dispatchers = std::move(Other.Dispatchers);
			Locks = std::move(Other.Locks);
			return *this;
		}

		/**
		 * @brief Invoke all bound callbacks.
		 */
		inline void Broadcast(TArgs... Args)
		{
			Lock();
			for (std::size_t Idx = 0; Idx < Dispatchers.size(); Idx++)
			{
				if (Dispatchers[Idx].Handle.IsValid())
				{
					Dispatchers[Idx].Callback.Execute(Args...);
				}
			}
			Unlock();
		}

		/**
		 * @brief Remove a bound delegate.
		 */
		bool Remove(FDelegateHandle& Handle)
		{
			if (!Handle.IsValid())
			{
				return false;
			}

			for (std::size_t Idx = 0; Idx < Dispatchers.size(); Idx++)
			{
				if (Dispatchers[Idx].Handle != Handle)
				{
					continue;
				}

				if (IsLocked())
				{
					Dispatchers[Idx].Callback.Clear();
				}
				else
				{
					std::swap(Dispatchers[Idx], Dispatchers[Dispatchers.size() - 1]);
					Dispatchers.pop_back();
				}

				Handle.Reset();
				return true;
			}

			return false;
		}

		/** 
		 * @brief Clear all bound callbacks.
		 */
		void RemoveAll()
		{
			if (IsLocked())
			{
				for (FDelegateHandlerPair& Handler : Dispatchers)
				{
					Handler.Callback.Clear();
				}
			}
			else
			{
				Dispatchers.clear();
			}
		}

		/**
		 * @brief Return the count of bound callbacks.
		 */
		inline size_t GetSize() const { return Dispatchers.size(); }
		inline std::string_view GetName() const { return typeid(decltype(this)).name(); }

		/** Raw Pointer, non-const function. */
		template<typename T, typename... TArgs2>
		FDelegateHandle Add(T* ObjectRef, NonConstMemberFunction<T, TArgs2...> InFunction, TArgs2&&... Args)
		{
			return AddHandler(TDelegate::CreateRaw(ObjectRef, InFunction, std::forward<TArgs2>(Args)...));
		}

		/** Raw Pointer, const function. */
		template<typename T, typename... TArgs2>
		FDelegateHandle Add(T* ObjectRef, ConstMemberFunction<T, TArgs2...> InFunction, TArgs2&&... Args)
		{
			return AddHandler(TDelegate::CreateRaw(ObjectRef, InFunction, std::forward<TArgs2>(Args)...));
		}

		/** Lambda. */
		template<typename LambdaType, typename... TArgs2>
		FDelegateHandle Add(LambdaType&& InLambda, TArgs2&&... LambdaArgs)
		{
			return AddHandler(TDelegate::CreateLambda(
				std::forward<LambdaType>(InLambda), 
				std::forward<TArgs2>(LambdaArgs)...)
			);
		}

		/** Shared Pointer, non-const function. */
		template<typename T, typename... TArgs2>
		FDelegateHandle Add(std::shared_ptr<T> ObjectRef, NonConstMemberFunction<T, TArgs2...> InFunction, TArgs2&&... Args)
		{
			return AddHandler(TDelegate::CreateShared(
				ObjectRef, 
				InFunction, 
				std::forward<TArgs2>(Args)...)
			);
		}

		/** Shared Pointer, const function. */
		template<typename T, typename... TArgs2>
		FDelegateHandle Add(std::shared_ptr<T> ObjectRef, ConstMemberFunction<T, TArgs2...> InFunction, TArgs2&&... Args)
		{
			return AddHandler(TDelegate::CreateShared(
				ObjectRef, 
				InFunction, 
				std::forward<TArgs2>(Args)...)
			);
		}

		/** Static function. */
		template<typename... TArgs2>
		FDelegateHandle Add(void(*InFunction)(TArgs..., TArgs2...), TArgs2&&... Args)
		{
			return AddHandler(TDelegate::CreateStatic(InFunction, std::forward<TArgs2>(Args)...));
		}

	private:
		template<typename T>
		FDelegateHandle operator+=(T&& LHS)
		{
			return AddHandler(TDelegate::CreateLambda(std::move(LHS)));
		}

		FDelegateHandle operator+=(TDelegate&& InHandler) noexcept
		{
			return AddHandler(std::forward<TDelegate>(InHandler));
		}

		bool operator-=(FDelegateHandle& InHandle) 
		{ 
			return Remove(InHandle); 
		}

		FDelegateHandle AddHandler(TDelegate&& Handler) noexcept
		{
			for (size_t i = 0; i < Dispatchers.size(); ++i)
			{
				if (Dispatchers[i].Handle.IsValid() == false)
				{
					Dispatchers[i] = FDelegateHandlerPair(FDelegateHandle(true), std::move(Handler));

					return Dispatchers[i].Handle;
				}
			}

			Dispatchers.emplace_back(FDelegateHandle(true), std::move(Handler));
			return Dispatchers.back().Handle;
		}

		/**
		 * @brief Removes all handles that are bound on an object
		 */
		void RemoveObject(void* InObjectRef)
		{
			if (!InObjectRef)
			{
				return;
			}

			for (size_t i = 0; i < Dispatchers.size(); ++i)
			{
				if (Dispatchers[i].Callback.GetOwner() != InObjectRef)
				{
					continue;
				}

				if (IsLocked())
				{
					Dispatchers[i].Callback.Clear();
				}
				else
				{
					std::swap(Dispatchers[i], Dispatchers[Dispatchers.size() - 1]);
					Dispatchers.pop_back();
				}
			}
		}

		inline bool IsBoundTo(const FDelegateHandle& Handle) const
		{
			if (!Handle.IsValid())
			{
				return false;
			}

			for (std::size_t i = 0; i < Dispatchers.size(); i++)
			{
				if (Dispatchers[i].Handle == Handle)
				{
					return true;
				}
			}

			return false;
		}

		inline void Compress(const size_t MaxSpace = 0)
		{
			if (IsLocked())
			{
				return;
			}

			std::size_t ToDelete = 0;
			for (size_t i = 0; i < Dispatchers.size() - ToDelete; i++)
			{
				if (!Dispatchers[i].Handle.IsValid())
				{
					std::swap(Dispatchers[i], Dispatchers[ToDelete]);
					ToDelete++;
				}
			}

			if (ToDelete > MaxSpace)
			{
				Dispatchers.resize(Dispatchers.size() - ToDelete);
			}
		}

	private:
		inline void Lock()
		{
			Locks++;
		}

		inline void Unlock()
		{
			LK_ASSERT(Locks > 0, "Cannot unlock, locks is {}", Locks);
			Locks--;
		}

		inline bool IsLocked() const { return (Locks > 0); }

		/**
		 * @struct FDelegateHandlerPair
		 * Container for storing an UUID to a delegate object.
		 */
		struct FDelegateHandlerPair
		{
			FDelegateHandle Handle;
			TDelegate Callback;

			FDelegateHandlerPair() : Handle(false) {}
			FDelegateHandlerPair(const FDelegateHandle& InHandle, const TDelegate& InCallback) 
				: Handle(InHandle)
				, Callback(InCallback) 
			{
			}

			FDelegateHandlerPair(const FDelegateHandle& InHandle, TDelegate&& InCallback) 
				: Handle(InHandle)
				, Callback(std::move(InCallback)) 
			{
			}
		};

	private:
		std::vector<FDelegateHandlerPair> Dispatchers{};
		uint32_t Locks = 0;
	};
}