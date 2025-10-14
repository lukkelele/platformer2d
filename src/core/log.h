#pragma once

#include <filesystem>
#include <map>
#include <string>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/fmt/fmt.h>

#include "assert.h"
#include "ansi_colors.h"
#include "log_formatters.h"

namespace platformer2d {

    /**
     * @enum ELogLevel
	 * Log verbosity.
     */
    enum class ELogLevel
    {
        Trace,
        Debug,
        Info,
        Warning,
        Error,
        Fatal
    };

    /**
     * @enum ELoggerType
	 * Logger type.
     */
    enum class ELoggerType
    {
        Core = 0,
        Client,
        EditorConsole,
		TestRunner,
    };

    class CLog
    {
    public:
        struct FTagDetails
        {
            bool Enabled = true;
            ELogLevel Filter = ELogLevel::Debug;

            FTagDetails() = default;
            FTagDetails(const ELogLevel InFilter) : Filter(InFilter) {}
        };

        CLog();
        ~CLog();

        static CLog& Get();

        static void Initialize(std::string_view InLogfile = "");

        inline static std::shared_ptr<spdlog::logger>& GetLogger(const ELoggerType LoggerType)
        {
            switch (LoggerType)
            {
				case ELoggerType::Core: return Logger_Core;
            }

			LK_ASSERT(false, "Unsupported logger type: {}", static_cast<int>(LoggerType));
			return Logger_Core;
        }

        /**
         * @brief Print a formatted message.
         */
        template<typename... TArgs>
	#if defined(LK_COMPILER_MSVC)
        static void PrintMessage(const ELoggerType LoggerType, const ELogLevel Level,
                                 std::format_string<TArgs...> Format, TArgs&&... Args);
	#elif defined(LK_COMPILER_GCC) || defined(LK_COMPILER_CLANG)
        static void PrintMessage(const ELoggerType LoggerType, const ELogLevel Level,
                                 fmt::format_string<TArgs...> Format, TArgs&&... Args);
	#endif

        /**
         * @brief Print a formatted message with a tag.
		 * @note Uses std::format_string on MSVC and fmt::format on GCC/Clang.
         */
        template<typename... TArgs>
	#if defined(LK_COMPILER_MSVC)
        static void PrintMessageWithTag(const ELoggerType LoggerType, const ELogLevel Level, std::string_view Tag,
                                        std::format_string<TArgs...> Format, TArgs&&... Args);
	#elif defined(LK_COMPILER_GCC) || defined(LK_COMPILER_CLANG)
        static void PrintMessageWithTag(const ELoggerType LoggerType, const ELogLevel Level, std::string_view Tag,
                                        fmt::format_string<TArgs...> Format, TArgs&&... Args);
	#endif

        static void PrintMessageWithTag(const ELoggerType LoggerType, const ELogLevel Level,
                                        std::string_view Tag, std::string_view Message);

        template<typename... TArgs>
	#if defined(LK_COMPILER_MSVC)
        static void PrintAssertMessage(const ELoggerType LoggerType, std::string_view Prefix,
                                       std::format_string<TArgs...> Message, TArgs&&... Args);
	#elif defined(LK_COMPILER_GCC) || defined(LK_COMPILER_CLANG)
        static void PrintAssertMessage(const ELoggerType LoggerType, std::string_view Prefix,
                                       fmt::format_string<TArgs...> Message, TArgs&&... Args);
	#endif
        static void PrintAssertMessage(const ELoggerType LoggerType, std::string_view Prefix);

		template<typename... TArgs>
	#if defined(LK_COMPILER_MSVC)
		static void Print(std::format_string<TArgs...> Format, TArgs&&... Args)
	#elif defined(LK_COMPILER_GCC) || defined(LK_COMPILER_CLANG)
		static void Print(fmt::format_string<TArgs...> Format, TArgs&&... Args)
	#endif
		{
		#if defined(LK_COMPILER_MSVC)
			const std::string FormattedString = std::format(Format, std::forward<TArgs>(Args)...);
		#elif defined(LK_COMPILER_GCC) || defined(LK_COMPILER_CLANG)
			const std::string FormattedString = fmt::format(Format, std::forward<TArgs>(Args)...);
		#endif
			std::printf("%s", FormattedString.c_str());
			std::fflush(stdout);
		}

		template<typename... TArgs>
	#if defined(LK_COMPILER_MSVC)
		static void PrintLn(std::format_string<TArgs...> Format, TArgs&&... Args)
	#elif defined(LK_COMPILER_GCC) || defined(LK_COMPILER_CLANG)
		static void PrintLn(fmt::format_string<TArgs...> Format, TArgs&&... Args)
	#endif
		{
		#if defined(LK_COMPILER_MSVC)
			const std::string FormattedString = std::format(Format, std::forward<TArgs>(Args)...);
		#elif defined(LK_COMPILER_GCC) || defined(LK_COMPILER_CLANG)
			const std::string FormattedString = fmt::format(Format, std::forward<TArgs>(Args)...);
		#endif
			std::printf("%s\n", FormattedString.c_str());
			std::fflush(stdout);
		}

        static const char* LevelToString(ELogLevel Level);
        static ELogLevel LevelFromString(std::string_view InString);
        static spdlog::level::level_enum ToSpdlogLevel(ELogLevel Level);

        FORCEINLINE static std::string_view GetLoggerName(const ELoggerType LoggerType)
        {
			switch (LoggerType)
			{
				case ELoggerType::Core: return Logger_Core->name();
			}

			LK_ASSERT(false, "Unknown logger type: {}", static_cast<int>(LoggerType));
            return "";
        }


    private:
        inline static std::shared_ptr<spdlog::logger> Logger_Core = nullptr;
        inline static std::map<std::string, FTagDetails> EnabledTags;
    };
}

namespace platformer2d {

	template<typename... TArgs>
	#if defined(LK_COMPILER_MSVC)
	FORCEINLINE void CLog::PrintMessage(const ELoggerType LoggerType, const ELogLevel Level,
										std::format_string<TArgs...> Format, TArgs&&... Args)
	#elif defined(LK_COMPILER_GCC) || defined(LK_COMPILER_CLANG)
	FORCEINLINE void CLog::PrintMessage(const ELoggerType LoggerType, const ELogLevel Level,
										fmt::format_string<TArgs...> Format, TArgs&&... Args)
	#endif
	{
		FTagDetails& TagDetails = EnabledTags[GetLoggerName(LoggerType).data()];
		if (TagDetails.Enabled && TagDetails.Filter <= Level)
		{
			auto& Logger = CLog::GetLogger(LoggerType);
			switch (Level)
			{
				case ELogLevel::Trace:
					Logger->trace(Format, std::forward<TArgs>(Args)...);
					break;
				case ELogLevel::Debug:
					Logger->debug(Format, std::forward<TArgs>(Args)...);
					break;
				case ELogLevel::Info:
					Logger->info(Format, std::forward<TArgs>(Args)...);
					break;
				case ELogLevel::Warning:
					Logger->warn(Format, std::forward<TArgs>(Args)...);
					break;
				case ELogLevel::Error:
					Logger->error(Format, std::forward<TArgs>(Args)...);
					break;
				case ELogLevel::Fatal:
					Logger->critical(Format, std::forward<TArgs>(Args)...);
					break;
			}
		}
	}

	template<typename... TArgs>
	#if defined(LK_COMPILER_MSVC)
	FORCEINLINE void CLog::PrintMessageWithTag(const ELoggerType LoggerType, const ELogLevel Level, std::string_view Tag, 
											   std::format_string<TArgs...> Format, TArgs&&... Args)
	#elif defined(LK_COMPILER_GCC) || defined(LK_COMPILER_CLANG)
	FORCEINLINE void CLog::PrintMessageWithTag(const ELoggerType LoggerType, const ELogLevel Level, std::string_view Tag, 
											   fmt::format_string<TArgs...> Format, TArgs&&... Args)
	#endif
	{
		const FTagDetails& TagDetails = EnabledTags[GetLoggerName(LoggerType).data()];
		if (TagDetails.Enabled && (TagDetails.Filter <= Level))
		{
		#if defined(LK_COMPILER_MSVC)
			const std::string FormattedString = std::format(Format, std::forward<TArgs>(Args)...);
		#elif defined(LK_COMPILER_GCC) || defined(LK_COMPILER_CLANG)
			const std::string FormattedString = fmt::format(Format, std::forward<TArgs>(Args)...);
		#else
			#error "Unsupported"
		#endif
			auto& Logger = CLog::GetLogger(LoggerType);
			switch (Level)
			{
				case ELogLevel::Trace:
					Logger->trace("[{0}] {1}", Tag, FormattedString);
					break;
				case ELogLevel::Debug:
					Logger->debug("[{0}] {1}", Tag, FormattedString);
					break;
				case ELogLevel::Info:
					Logger->info("[{0}] {1}", Tag, FormattedString);
					break;
				case ELogLevel::Warning:
					Logger->warn("[{0}] {1}", Tag, FormattedString);
					break;
				case ELogLevel::Error:
					Logger->error("[{0}] {1}", Tag, FormattedString);
					break;
				case ELogLevel::Fatal:
					Logger->critical("[{0}] {1}", Tag, FormattedString);
					break;
			}
		}
	}

	FORCEINLINE void CLog::PrintMessageWithTag(const ELoggerType LoggerType, const ELogLevel Level, 
											   std::string_view Tag, std::string_view Message)
	{
		const FTagDetails& TagDetails = EnabledTags[GetLoggerName(LoggerType).data()];
		if (TagDetails.Enabled && TagDetails.Filter <= Level)
		{
			auto& Logger = GetLogger(LoggerType);
			switch (Level)
			{
				case ELogLevel::Trace:
					Logger->trace("[{0}] {1}", Tag, Message);
					break;
				case ELogLevel::Debug:
					Logger->debug("[{0}] {1}", Tag, Message);
					break;
				case ELogLevel::Info:
					Logger->info("[{0}] {1}", Tag, Message);
					break;
				case ELogLevel::Warning:
					Logger->warn("[{0}] {1}", Tag, Message);
					break;
				case ELogLevel::Error:
					Logger->error("[{0}] {1}", Tag, Message);
					break;
				case ELogLevel::Fatal:
					Logger->critical("[{0}] {1}", Tag, Message);
					break;
			}
		}
	}

	template<typename... TArgs>
	#if defined(LK_COMPILER_MSVC)
	FORCEINLINE void CLog::PrintAssertMessage(const ELoggerType LoggerType, std::string_view Prefix, 
											  std::format_string<TArgs...> Format, TArgs&&... Args)
	#elif defined(LK_COMPILER_GCC) || defined(LK_COMPILER_CLANG)
	FORCEINLINE void CLog::PrintAssertMessage(const ELoggerType LoggerType, std::string_view Prefix, 
											  fmt::format_string<TArgs...> Format, TArgs&&... Args)
	#endif
	{
		#if defined(LK_COMPILER_MSVC)
			const std::string FormattedString = std::format(Format, std::forward<TArgs>(Args)...);
		#elif defined(LK_COMPILER_GCC) || defined(LK_COMPILER_CLANG)
			const std::string FormattedString = fmt::format(Format, std::forward<TArgs>(Args)...);
		#else
			#error "Unsupported"
		#endif
		if (auto Logger = GetLogger(LoggerType); Logger != nullptr)
		{
			Logger->error("{0}: {1}", Prefix, FormattedString);
		}
		else
		{
			PrintLn("{2}{0}: {1}{3}", Prefix, FormattedString, LK_ANSI_COLOR_BG_BRIGHT_RED, LK_ANSI_COLOR_RESET);
		}

	#if LK_ASSERT_MESSAGE_BOX
		MessageBoxA(nullptr, FormattedString.c_str(), "platformer2d error", (MB_OK | MB_ICONERROR));
	#endif
	}

	FORCEINLINE void CLog::PrintAssertMessage(const ELoggerType LoggerType, std::string_view Message)
	{
		if (auto Logger = GetLogger(LoggerType); Logger != nullptr)
		{
			GetLogger(LoggerType)->error("{0}", Message);
		}
		else
		{
			PrintLn("{1}{0}{2}", Message, LK_ANSI_COLOR_BG_BRIGHT_RED, LK_ANSI_COLOR_RESET);
		}
		
	#if LK_ASSERT_MESSAGE_BOX
		MessageBoxA(nullptr, Message.data(), "platformer2d error", (MB_OK | MB_ICONERROR));
	#endif
	}

}


/** 
 * Standard print functions.
 */
#define LK_PRINT(...)    ::platformer2d::CLog::Print(__VA_ARGS__)
#define LK_PRINTLN(...)  ::platformer2d::CLog::PrintLn(__VA_ARGS__)

/** 
 * Core logging.
 */
#define LK_TRACE(...)   ::platformer2d::CLog::PrintMessage(::platformer2d::ELoggerType::Core, ::platformer2d::ELogLevel::Trace, __VA_ARGS__)
#define LK_DEBUG(...)   ::platformer2d::CLog::PrintMessage(::platformer2d::ELoggerType::Core, ::platformer2d::ELogLevel::Debug, __VA_ARGS__)
#define LK_INFO(...)    ::platformer2d::CLog::PrintMessage(::platformer2d::ELoggerType::Core, ::platformer2d::ELogLevel::Info, __VA_ARGS__)
#define LK_WARN(...)    ::platformer2d::CLog::PrintMessage(::platformer2d::ELoggerType::Core, ::platformer2d::ELogLevel::Warning, __VA_ARGS__)
#define LK_ERROR(...)   ::platformer2d::CLog::PrintMessage(::platformer2d::ELoggerType::Core, ::platformer2d::ELogLevel::Error, __VA_ARGS__)
#define LK_FATAL(...)   ::platformer2d::CLog::PrintMessage(::platformer2d::ELoggerType::Core, ::platformer2d::ELogLevel::Fatal, __VA_ARGS__)

#define LK_TRACE_TAG(Tag, ...)  ::platformer2d::CLog::PrintMessageWithTag(::platformer2d::ELoggerType::Core, ::platformer2d::ELogLevel::Trace, Tag, __VA_ARGS__)
#define LK_DEBUG_TAG(Tag, ...)  ::platformer2d::CLog::PrintMessageWithTag(::platformer2d::ELoggerType::Core, ::platformer2d::ELogLevel::Debug, Tag, __VA_ARGS__)
#define LK_INFO_TAG(Tag, ...)   ::platformer2d::CLog::PrintMessageWithTag(::platformer2d::ELoggerType::Core, ::platformer2d::ELogLevel::Info, Tag, __VA_ARGS__)
#define LK_WARN_TAG(Tag, ...)   ::platformer2d::CLog::PrintMessageWithTag(::platformer2d::ELoggerType::Core, ::platformer2d::ELogLevel::Warning, Tag, __VA_ARGS__)
#define LK_ERROR_TAG(Tag, ...)  ::platformer2d::CLog::PrintMessageWithTag(::platformer2d::ELoggerType::Core, ::platformer2d::ELogLevel::Error, Tag, __VA_ARGS__)
#define LK_FATAL_TAG(Tag, ...)  ::platformer2d::CLog::PrintMessageWithTag(::platformer2d::ELoggerType::Core, ::platformer2d::ELogLevel::Fatal, Tag, __VA_ARGS__)
