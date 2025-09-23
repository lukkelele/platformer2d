#include "test.h"

#include <string>
#include <stdexcept>

#if defined(_WIN32)
#include <Windows.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

namespace platformer2d::test {

	std::filesystem::path GetBinaryDirectory()
	{
#if defined(_WIN32)
		wchar_t Buffer[MAX_PATH];
		const DWORD Length = GetModuleFileNameW(nullptr, Buffer, MAX_PATH);
		if ((Length == 0) || (Length == MAX_PATH))
		{
			throw std::runtime_error("Failed to get executable path");
		}
		return std::filesystem::path(Buffer).parent_path();
#elif defined(__linux__)
		char Buffer[4096];
		ssize_t Count = readlink("/proc/self/exe", Buffer, sizeof(Buffer) - 1);
		if (Count == -1)
		{
			throw std::runtime_error("Failed to read /proc/self/exe");
		}
		Buffer[Count] = '\0';
		return std::filesystem::path(Buffer).parent_path();
#else
#error "Unsupported platform"
#endif
	}

}