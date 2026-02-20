#pragma once

#include <cstdio>
#include <filesystem>
#include <map>
#include <string>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/fmt/fmt.h>

#include "ansi_colors.h"
#include "log_formatters.h"
#include "macros.h"

/* Internal assert macro because of cyclic inclusion in assert.h */
#define LK_LOG_ASSERT(Condition, ...) assert(Condition)

#if LK_LOG_FORCE_INLINE
#define LK_INLINE FORCEINLINE
#else
#define LK_INLINE inline
#endif

namespace platformer2d {

#if defined(LK_COMPILER_MSVC)
    template<typename... TArgs>
    using TFormatString = std::format_string<TArgs...>;
#elif defined(LK_COMPILER_GCC) || defined(LK_COMPILER_CLANG)
    template<typename... TArgs>
    using TFormatString = fmt::format_string<TArgs...>;
#endif

    template<typename... TArgs>
    inline std::string Format(TFormatString<TArgs...> Fmt, TArgs&&... Args)
    {
        return LkFmt::format(Fmt, std::forward<TArgs>(Args)...);
    }

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
        Core,
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
            switch (LoggerType) {
				case ELoggerType::Core: return Logger_Core;
            }

			LK_LOG_ASSERT(false, "Unsupported logger type: {}", static_cast<int>(LoggerType));
			return Logger_Core;
        }

        /**
         * @brief Print a formatted message.
         */
        template<typename... TArgs>
        static void PrintMessage(const ELoggerType LoggerType, const ELogLevel Level, TFormatString<TArgs...> Format, TArgs&&... Args);

        /**
         * @brief Print a formatted message with a tag.
		 * @note Uses std::format_string on MSVC and fmt::format on GCC/Clang.
         */
        template<typename... TArgs>
        static void PrintMessageWithTag(const ELoggerType LoggerType, const ELogLevel Level, std::string_view Tag, TFormatString<TArgs...> Format, TArgs&&... Args);
        static void PrintMessageWithTag(const ELoggerType LoggerType, const ELogLevel Level, std::string_view Tag, std::string_view Message);

        template<typename... TArgs>
        static void PrintAssertMessage(const ELoggerType LoggerType, std::string_view Prefix, TFormatString<TArgs...> Message, TArgs&&... Args);
        static void PrintAssertMessage(const ELoggerType LoggerType, std::string_view Prefix);

		template<typename... TArgs>
		static void Print(TFormatString<TArgs...> Fmt, TArgs&&... Args)
		{
			const std::string String = Format(Fmt, std::forward<TArgs>(Args)...);
			std::printf("%s", String.c_str());
			std::fflush(stdout);
		}

		template<typename... TArgs>
		static void PrintLn(TFormatString<TArgs...> Fmt, TArgs&&... Args)
		{
			const std::string String = Format(Fmt, std::forward<TArgs>(Args)...);
			std::printf("%s\n", String.c_str());
			std::fflush(stdout);
		}

		static void SetLogLevel(ELogLevel Level);
		static void SetLogLevel(ELoggerType Logger, ELogLevel Level);
        static const char* LevelToString(ELogLevel Level);
        static ELogLevel LevelFromString(std::string_view InString);
        static spdlog::level::level_enum ToSpdlogLevel(ELogLevel Level);

        FORCEINLINE static std::string_view GetLoggerName(const ELoggerType LoggerType) {
			switch (LoggerType) {
				case ELoggerType::Core: return Logger_Core->name();
			}

			LK_LOG_ASSERT(false, "Unknown logger type: {}", static_cast<int>(LoggerType));
            return "";
        }

    private:
        inline static std::shared_ptr<spdlog::logger> Logger_Core = nullptr;
        inline static std::map<std::string, FTagDetails> EnabledTags;
    };
}

namespace platformer2d {

	template<typename... TArgs>
	FORCEINLINE void CLog::PrintMessage(const ELoggerType LoggerType, const ELogLevel Level, TFormatString<TArgs...> Fmt, TArgs&&... Args)
	{
		FTagDetails& TagDetails = EnabledTags[GetLoggerName(LoggerType).data()];
		if (TagDetails.Enabled && TagDetails.Filter <= Level) {
			auto& Logger = CLog::GetLogger(LoggerType);
			switch (Level) {
				case ELogLevel::Trace:
					Logger->trace(Fmt, std::forward<TArgs>(Args)...);
					break;
				case ELogLevel::Debug:
					Logger->debug(Fmt, std::forward<TArgs>(Args)...);
					break;
				case ELogLevel::Info:
					Logger->info(Fmt, std::forward<TArgs>(Args)...);
					break;
				case ELogLevel::Warning:
					Logger->warn(Fmt, std::forward<TArgs>(Args)...);
					break;
				case ELogLevel::Error:
					Logger->error(Fmt, std::forward<TArgs>(Args)...);
					break;
				case ELogLevel::Fatal:
					Logger->critical(Fmt, std::forward<TArgs>(Args)...);
					break;
			}
		}
	}

	template<typename... TArgs>
	LK_INLINE void CLog::PrintMessageWithTag(const ELoggerType LoggerType, const ELogLevel Level, std::string_view Tag, TFormatString<TArgs...> Fmt, TArgs&&... Args)
	{
		const FTagDetails& TagDetails = EnabledTags[GetLoggerName(LoggerType).data()];
		if (TagDetails.Enabled && (TagDetails.Filter <= Level)) {
			const std::string Message = Format(Fmt, std::forward<TArgs>(Args)...);
			auto& Logger = CLog::GetLogger(LoggerType);
			switch (Level) {
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

	LK_INLINE void CLog::PrintMessageWithTag(const ELoggerType LoggerType, const ELogLevel Level, std::string_view Tag, std::string_view Message)
	{
		const FTagDetails& TagDetails = EnabledTags[GetLoggerName(LoggerType).data()];
		if (TagDetails.Enabled && TagDetails.Filter <= Level) {
			auto& Logger = GetLogger(LoggerType);
			switch (Level) {
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
	LK_INLINE void CLog::PrintAssertMessage(const ELoggerType LoggerType, std::string_view Prefix, TFormatString<TArgs...> Fmt, TArgs&&... Args)
	{
		const std::string Message = Format(Fmt, std::forward<TArgs>(Args)...);
		if (auto Logger = GetLogger(LoggerType); Logger != nullptr) {
			Logger->error("{0}: {1}", Prefix, Message);
		} else {
			PrintLn("{2}{0}: {1}{3}", Prefix, Message, LK_ANSI_COLOR_BG_BRIGHT_RED, LK_ANSI_COLOR_RESET);
		}
	}

	LK_INLINE void CLog::PrintAssertMessage(const ELoggerType LoggerType, std::string_view Message)
	{
		if (auto Logger = GetLogger(LoggerType); Logger != nullptr) {
			GetLogger(LoggerType)->error("{0}", Message);
		} else {
			PrintLn("{1}{0}{2}", Message, LK_ANSI_COLOR_BG_BRIGHT_RED, LK_ANSI_COLOR_RESET);
		}
	}
}

#undef LK_LOG_ASSERT
#undef LK_INLINE

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
