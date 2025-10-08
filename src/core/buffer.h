#pragma once

#include "assert.h"
#include "core.h"

namespace platformer2d {

	struct FBuffer
	{
		explicit FBuffer(const void* InData, const uint64_t InSize)
			: Data((void*)InData)
			, Size(InSize)
		{
		}

		FBuffer()
			: Data(nullptr)
			, Size(0)
		{
		}

		~FBuffer()
		{
			Release();
		}

		FBuffer(const FBuffer& Other)
			: Data(nullptr)
			, Size(Other.Size)
		{
			if (Other.Data && (Other.Size > 0))
			{
				/* Perform deep copy. */
				Allocate(Other.Size);
				if (Data)
				{
					std::memcpy(Data, Other.Data, Other.Size);
				}
			}
		}

		FBuffer(FBuffer&& Other) noexcept
			: Data(Other.Data)
			, Size(Other.Size)
		{
			Other.Data = nullptr;
			Other.Size = 0;
		}

		FBuffer& operator=(const FBuffer& Other) noexcept
		{
			if (this != &Other)
			{
				/* Release current buffer if it exists. */
				Release();

				if (Other.Data && Other.Size > 0)
				{
					/* Perform deep copy. */
					Allocate(Other.Size);
					std::memcpy(Data, Other.Data, Other.Size);
					Size = Other.Size;
				}
				else
				{
					Data = nullptr;
					Size = 0;
				}
			}

			return *this;
		}

		FBuffer& operator=(FBuffer&& Other) noexcept
		{
			if (this != &Other)
			{
				delete[](uint8_t*)Data;

				/* Transfer ownership. */
				Allocate(Other.Size);
				std::memcpy(Data, Other.Data, Other.Size);

				Size = Other.Size;

				Other.Data = nullptr;
				Other.Size = 0;
			}

			return *this;
		}

		static FBuffer Copy(const FBuffer& Other)
		{
			FBuffer Buffer;
			Buffer.Allocate(Other.Size);
			std::memcpy(Buffer.Data, Other.Data, Other.Size);

			return Buffer;
		}

		static FBuffer Copy(const void* Data, const uint64_t Size)
		{
			FBuffer Buffer;
			Buffer.Allocate(Size);
			std::memcpy(Buffer.Data, Data, Size);

			return Buffer;
		}

		FORCEINLINE void Allocate(const uint64_t InSize)
		{
			LK_ASSERT(InSize > 0, "Allocate failed, invalid size: {}", InSize);

			delete[](uint8_t*)Data;
			Data = nullptr;

			Data = new uint8_t[InSize];
			Size = InSize;
		}

		FORCEINLINE void Release()
		{
			if (Data)
			{
				delete[](uint8_t*)Data;
				Data = nullptr;
				Size = 0;
			}
		}

		void ZeroInitialize()
		{
			if (Size > 0)
			{
				std::memset(Data, 0, Size);
			}
		}

		template<typename T>
		FORCEINLINE T& Read(const uint64_t Offset = 0)
		{
			return *(T*)(static_cast<uint8_t*>(Data) + Offset);
		}

		template<typename T>
		FORCEINLINE const T& Read(uint64_t Offset = 0) const
		{
			return *(T*)(static_cast<uint8_t*>(Data) + Offset);
		}

		FORCEINLINE uint8_t* ReadBytes(const uint64_t ReadSize, const uint64_t Offset) const
		{
			LK_ASSERT(Offset + ReadSize <= ReadSize, "Buffer overflow");
			uint8_t* Buffer = new uint8_t[ReadSize];
			std::memcpy(Buffer, (uint8_t*)Data + Offset, ReadSize);

			return Buffer;
		}

		FORCEINLINE void Write(const void* InData, const uint64_t WriteSize, uint64_t Offset = 0)
		{
			LK_ASSERT(Offset + WriteSize <= Size, "FBuffer overflow");
			std::memcpy((uint8_t*)Data + Offset, InData, WriteSize);
		}

		operator bool() { return (Data && (Size > 0)); }
		operator bool() const { return (Data && (Size > 0)); }

		uint8_t& operator[](int index)
		{
			return ((uint8_t*)Data)[index];
		}

		uint8_t operator[](int index) const
		{
			return ((uint8_t*)Data)[index];
		}

		template<typename T>
		T* As() const
		{
			return (T*)Data;
		}

		uint64_t GetSize() const { return Size; }

	public:
		void* Data = nullptr;
		uint64_t Size = 0;
	};

}