#include <array>
#include <cstdint>
#include <cstring>
#include <numeric>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "core/buffer.h"
#include "core/log.h"

using namespace platformer2d;

namespace Test::Buffer {
	struct FPodSample
	{
		std::uint32_t Id;
		float X;
		float Y;
	};

	static_assert(std::is_trivially_copyable_v<FPodSample>);
}

using namespace Test::Buffer;

TEST_CASE("FBuffer default construction yields an empty buffer", "[buffer][construction]")
{
	FBuffer Buffer;
	REQUIRE(Buffer.Data == nullptr);
	REQUIRE(Buffer.Size == 0);
	REQUIRE(Buffer.GetSize() == 0);
	REQUIRE_FALSE(static_cast<bool>(Buffer));
}

TEST_CASE("FBuffer takes ownership of preallocated heap memory", "[buffer][construction]")
{
	constexpr std::uint64_t ByteCount = 64;
	std::uint8_t* Raw = new std::uint8_t[ByteCount];
	for (std::size_t Idx = 0; Idx < ByteCount; Idx++) {
		Raw[Idx] = static_cast<std::uint8_t>(Idx);
	}

	FBuffer Buffer(Raw, ByteCount);
	REQUIRE(Buffer.Data == Raw);
	REQUIRE(Buffer.GetSize() == ByteCount);
	REQUIRE(static_cast<bool>(Buffer));
	REQUIRE(Buffer[0] == 0);
	REQUIRE(Buffer[63] == 63);
}

TEST_CASE("FBuffer::Allocate allocates the requested size", "[buffer][memory]")
{
	FBuffer Buffer;
	Buffer.Allocate(128);
	REQUIRE(Buffer.Data != nullptr);
	REQUIRE(Buffer.GetSize() == 128);
	REQUIRE(static_cast<bool>(Buffer));
}

TEST_CASE("FBuffer::Allocate replaces previous allocation", "[buffer][memory]")
{
	FBuffer Buffer;
	Buffer.Allocate(32);
	const void* FirstPtr = Buffer.Data;
	REQUIRE(FirstPtr != nullptr);
	REQUIRE(Buffer.GetSize() == 32);

	Buffer.Allocate(256);
	REQUIRE(Buffer.Data != nullptr);
	REQUIRE(Buffer.GetSize() == 256);
}

TEST_CASE("FBuffer::Release frees memory and resets state", "[buffer][memory]")
{
	FBuffer Buffer;
	Buffer.Allocate(64);
	REQUIRE(static_cast<bool>(Buffer));

	Buffer.Release();
	REQUIRE(Buffer.Data == nullptr);
	REQUIRE(Buffer.GetSize() == 0);
	REQUIRE_FALSE(static_cast<bool>(Buffer));
}

TEST_CASE("FBuffer::Release on an empty buffer is a no-op", "[buffer][memory]")
{
	FBuffer Buffer;
	Buffer.Release();
	REQUIRE(Buffer.Data == nullptr);
	REQUIRE(Buffer.GetSize() == 0);
}

TEST_CASE("FBuffer copy constructor performs a deep copy", "[buffer][copy]")
{
	FBuffer Source;
	Source.Allocate(16);
	for (std::size_t Idx = 0; Idx < Source.GetSize(); Idx++) {
		Source[Idx] = static_cast<std::uint8_t>(Idx + 1);
	}

	FBuffer Copy(Source);
	REQUIRE(Copy.GetSize() == Source.GetSize());
	REQUIRE(Copy.Data != nullptr);
	REQUIRE(Copy.Data != Source.Data);
	REQUIRE(std::memcmp(Copy.Data, Source.Data, Source.GetSize()) == 0);

	Source[0] = 0xAB;
	REQUIRE(Copy[0] == 1);
}

TEST_CASE("FBuffer copy from empty source produces an empty buffer", "[buffer][copy]")
{
	FBuffer Source;
	FBuffer Copy(Source);
	REQUIRE(Copy.Data == nullptr);
	REQUIRE(Copy.GetSize() == 0);
}

TEST_CASE("FBuffer move constructor transfers ownership", "[buffer][move]")
{
	FBuffer Source;
	Source.Allocate(48);
	for (std::size_t Idx = 0; Idx < Source.GetSize(); Idx++) {
		Source[Idx] = static_cast<std::uint8_t>(Idx);
	}

	void* SourcePtr = Source.Data;
	const std::uint64_t SourceSize = Source.GetSize();

	FBuffer Moved(std::move(Source));
	REQUIRE(Moved.Data == SourcePtr);
	REQUIRE(Moved.GetSize() == SourceSize);
	REQUIRE(Source.Data == nullptr);
	REQUIRE(Source.GetSize() == 0);
	REQUIRE_FALSE(static_cast<bool>(Source));
}

TEST_CASE("FBuffer copy assignment performs a deep copy", "[buffer][copy]")
{
	FBuffer Source;
	Source.Allocate(24);
	for (std::size_t Idx = 0; Idx < Source.GetSize(); Idx++) {
		Source[Idx] = static_cast<std::uint8_t>(Idx * 3);
	}

	FBuffer Target;
	Target = Source;
	REQUIRE(Target.GetSize() == Source.GetSize());
	REQUIRE(Target.Data != Source.Data);
	REQUIRE(std::memcmp(Target.Data, Source.Data, Source.GetSize()) == 0);
}

TEST_CASE("FBuffer copy assignment releases previously held memory", "[buffer][copy]")
{
	FBuffer Source;
	Source.Allocate(8);
	std::memset(Source.Data, 0x5A, Source.GetSize());

	FBuffer Target;
	Target.Allocate(64);
	REQUIRE(Target.GetSize() == 64);

	Target = Source;
	REQUIRE(Target.GetSize() == 8);
	for (std::size_t Idx = 0; Idx < Target.GetSize(); Idx++) {
		REQUIRE(Target[Idx] == 0x5A);
	}
}

TEST_CASE("FBuffer copy assignment from empty buffer resets target", "[buffer][copy]")
{
	FBuffer Source;
	FBuffer Target;
	Target.Allocate(16);
	REQUIRE(static_cast<bool>(Target));

	Target = Source;
	REQUIRE(Target.Data == nullptr);
	REQUIRE(Target.GetSize() == 0);
}

TEST_CASE("FBuffer copy assignment is self-assignment safe", "[buffer][copy]")
{
	FBuffer Buffer;
	Buffer.Allocate(16);
	for (std::size_t Idx = 0; Idx < Buffer.GetSize(); Idx++) {
		Buffer[Idx] = static_cast<std::uint8_t>(Idx + 7);
	}

	FBuffer& SelfRef = Buffer;
	Buffer = SelfRef;
	REQUIRE(Buffer.GetSize() == 16);
	for (std::size_t Idx = 0; Idx < Buffer.GetSize(); Idx++) {
		REQUIRE(Buffer[Idx] == static_cast<std::uint8_t>(Idx + 7));
	}
}

TEST_CASE("FBuffer move assignment leaves source empty", "[buffer][move]")
{
	FBuffer Source;
	Source.Allocate(32);
	for (std::size_t Idx = 0; Idx < Source.GetSize(); Idx++) {
		Source[Idx] = static_cast<std::uint8_t>(Idx + 10);
	}

	FBuffer Target;
	Target = std::move(Source);
	REQUIRE(Target.GetSize() == 32);
	for (std::size_t Idx = 0; Idx < Target.GetSize(); Idx++) {
		REQUIRE(Target[Idx] == static_cast<std::uint8_t>(Idx + 10));
	}
	REQUIRE(Source.Data == nullptr);
	REQUIRE(Source.GetSize() == 0);
}

TEST_CASE("FBuffer::Copy(const FBuffer&) duplicates content into a new buffer", "[buffer][static-copy]")
{
	FBuffer Source;
	Source.Allocate(20);
	for (std::size_t Idx = 0; Idx < Source.GetSize(); Idx++) {
		Source[Idx] = static_cast<std::uint8_t>(0xF0 ^ Idx);
	}

	FBuffer Copy = FBuffer::Copy(Source);
	REQUIRE(Copy.GetSize() == Source.GetSize());
	REQUIRE(Copy.Data != Source.Data);
	REQUIRE(std::memcmp(Copy.Data, Source.Data, Source.GetSize()) == 0);
}

TEST_CASE("FBuffer::Copy(const void*, size) duplicates raw memory", "[buffer][static-copy]")
{
	std::array<std::uint8_t, 12> Source{};
	std::iota(Source.begin(), Source.end(), static_cast<std::uint8_t>(0xA0));

	FBuffer Copy = FBuffer::Copy(Source.data(), Source.size());
	REQUIRE(Copy.GetSize() == Source.size());
	REQUIRE(Copy.Data != Source.data());
	REQUIRE(std::memcmp(Copy.Data, Source.data(), Source.size()) == 0);
}

TEST_CASE("FBuffer::Write places data at the requested offset", "[buffer][write]")
{
	FBuffer Buffer;
	Buffer.Allocate(32);
	std::memset(Buffer.Data, 0, Buffer.GetSize());

	const std::array<std::uint8_t, 4> Payload{0xDE, 0xAD, 0xBE, 0xEF};
	Buffer.Write(Payload.data(), Payload.size(), 8);

	for (std::size_t Idx = 0; Idx < 8; Idx++) {
		REQUIRE(Buffer[Idx] == 0);
	}
	REQUIRE(Buffer[8] == 0xDE);
	REQUIRE(Buffer[9] == 0xAD);
	REQUIRE(Buffer[10] == 0xBE);
	REQUIRE(Buffer[11] == 0xEF);
	for (std::size_t Idx = 12; Idx < Buffer.GetSize(); Idx++) {
		REQUIRE(Buffer[Idx] == 0);
	}
}

TEST_CASE("FBuffer::Read reinterprets bytes as a typed value", "[buffer][read]")
{
	FBuffer Buffer;
	Buffer.Allocate(sizeof(FPodSample) * 2);
	std::memset(Buffer.Data, 0, Buffer.GetSize());

	const FPodSample Expected{.Id = 0x12345678, .X = 1.25f, .Y = -3.5f};
	Buffer.Write(&Expected, sizeof(FPodSample), sizeof(FPodSample));

	const FPodSample& ReadBack = Buffer.Read<FPodSample>(sizeof(FPodSample));
	REQUIRE(ReadBack.Id == Expected.Id);
	REQUIRE(ReadBack.X == Expected.X);
	REQUIRE(ReadBack.Y == Expected.Y);

	const FBuffer& ConstView = Buffer;
	const FPodSample& ConstReadBack = ConstView.Read<FPodSample>(sizeof(FPodSample));
	REQUIRE(ConstReadBack.Id == Expected.Id);
}

TEST_CASE("FBuffer::Read returns a mutable reference for non-const buffers", "[buffer][read]")
{
	FBuffer Buffer;
	Buffer.Allocate(sizeof(std::uint32_t));

	std::uint32_t& Slot = Buffer.Read<std::uint32_t>();
	Slot = 0xCAFEBABE;
	REQUIRE(Buffer.Read<std::uint32_t>() == 0xCAFEBABE);
}

TEST_CASE("FBuffer::operator[] reads and writes individual bytes", "[buffer][indexing]")
{
	FBuffer Buffer;
	Buffer.Allocate(4);
	Buffer[0] = 0x01;
	Buffer[1] = 0x02;
	Buffer[2] = 0x03;
	Buffer[3] = 0x04;

	const FBuffer& ConstView = Buffer;
	REQUIRE(ConstView[0] == 0x01);
	REQUIRE(ConstView[3] == 0x04);

	Buffer[2] = 0xFF;
	REQUIRE(Buffer[2] == 0xFF);
}

TEST_CASE("FBuffer::As reinterprets storage as a typed pointer", "[buffer][as]")
{
	FBuffer Buffer;
	Buffer.Allocate(sizeof(std::uint32_t) * 4);

	std::uint32_t* Values = Buffer.As<std::uint32_t>();
	REQUIRE(Values != nullptr);
	for (std::size_t Idx = 0; Idx < 4; Idx++) {
		Values[Idx] = static_cast<std::uint32_t>(Idx) * 0x10101010;
	}
	REQUIRE(Buffer.As<std::uint32_t>()[0] == 0x00000000);
	REQUIRE(Buffer.As<std::uint32_t>()[3] == 0x30303030);
}

TEST_CASE("FBuffer::operator bool reflects ownership and size", "[buffer][bool]")
{
	FBuffer Empty;
	REQUIRE_FALSE(static_cast<bool>(Empty));

	FBuffer Allocated;
	Allocated.Allocate(1);
	REQUIRE(static_cast<bool>(Allocated));

	const FBuffer& ConstView = Allocated;
	REQUIRE(static_cast<bool>(ConstView));

	Allocated.Release();
	REQUIRE_FALSE(static_cast<bool>(Allocated));
}

TEST_CASE("FBuffer destructor releases memory at scope exit", "[buffer][raii]")
{
	{
		FBuffer Buffer;
		Buffer.Allocate(64);
		REQUIRE(static_cast<bool>(Buffer));
	}
	SUCCEED("FBuffer scope exit ran without leaking ownership");
}

TEST_CASE("FBuffer round-trips a structured payload", "[buffer][roundtrip]")
{
	constexpr std::size_t Count = 4;
	FBuffer Buffer;
	Buffer.Allocate(sizeof(FPodSample) * Count);

	for (std::size_t Idx = 0; Idx < Count; Idx++) {
		const FPodSample Sample{
			.Id = static_cast<std::uint32_t>(Idx + 1),
			.X = static_cast<float>(Idx) * 2.0f,
			.Y = -static_cast<float>(Idx),
		};
		Buffer.Write(&Sample, sizeof(FPodSample), Idx * sizeof(FPodSample));
	}

	for (std::size_t Idx = 0; Idx < Count; Idx++) {
		const FPodSample& Sample = Buffer.Read<FPodSample>(Idx * sizeof(FPodSample));
		LK_INFO_TAG("Buffer", "Sample[{}]: Id={} X={} Y={}", Idx, Sample.Id, Sample.X, Sample.Y);
		REQUIRE(Sample.Id == static_cast<std::uint32_t>(Idx + 1));
		REQUIRE(Sample.X == static_cast<float>(Idx) * 2.0f);
		REQUIRE(Sample.Y == -static_cast<float>(Idx));
	}
}
