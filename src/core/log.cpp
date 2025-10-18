#include "log.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "assert.h"

#ifdef LK_LOG_DEBUG
#define LK_LOG_PRINTLN(...) LK_PRINTLN(__VA_ARGS__)
#else
#define LK_LOG_PRINTLN(...) 
#endif

namespace platformer2d {

	namespace
	{
		std::string Logfile;
		std::filesystem::path LogDirectory = LOGS_DIR;
		constexpr const char* FileName = "platformer2d";
	}

	namespace LogUtility
	{
		bool CompareLogFiles(const std::filesystem::directory_entry& A, const std::filesystem::directory_entry& B);
		int CountLogFilesInDir(const std::filesystem::path& InDirectory);
		void CleanLogDirectory(const std::filesystem::path& InDirectory, int MaxLogFiles);
	}

	static std::string CurrentTimestamp(std::string_view InFormat = "%Y-%m-%d-%H%M%S")
    {
        using namespace std::chrono;
        time_point<system_clock> Now = system_clock::now();
        std::time_t CurrentTime = system_clock::to_time_t(Now);

        std::stringstream StrStream;
        StrStream << std::put_time(std::localtime(&CurrentTime), InFormat.data());

        return StrStream.str();
    }

	CLog::CLog()
	{
		LK_LOG_PRINTLN("Log instance created");
		LK_LOG_PRINTLN("Log directory: {}", LogDirectory);
	}

	CLog::~CLog()
	{
	}

	CLog& CLog::Get()
	{
		static CLog Instance;
		return Instance;
	}

	void CLog::Initialize(std::string_view InLogfile)
	{
		Get(); /* Make sure instance exists. */
		if (!InLogfile.empty())
		{
			Logfile = InLogfile;	
		}
		else
		{
			Logfile = LK_FMT("{}-{}.log", FileName, CurrentTimestamp());
		}
		LK_LOG_PRINTLN("Log file: {}", Logfile);

		std::vector<spdlog::sink_ptr> CoreSinks = {
			std::make_shared<spdlog::sinks::basic_file_sink_mt>(LogDirectory.string() + Logfile, true),
			std::make_shared<spdlog::sinks::stdout_color_sink_mt>()
		};

		LogUtility::CleanLogDirectory(LogDirectory, 2);

		/**
		 * Index 0: file
		 * Index 1: stdout
		 */
		CoreSinks[0]->set_pattern("[%T] [%l] [%n] %v");
		CoreSinks[1]->set_pattern("[%T] [%^%l%$] %v");

		Logger_Core = std::make_shared<spdlog::logger>("CORE", CoreSinks.begin(), CoreSinks.end());
		Logger_Core->set_level(spdlog::level::trace);
		Logger_Core->flush_on(spdlog::level::trace);
	}

	const char* CLog::LevelToString(const ELogLevel Level)
	{
		switch (Level)
		{
			case ELogLevel::Trace:   return "Trace";
			case ELogLevel::Debug:	 return "Debug";
			case ELogLevel::Info:	 return "Info";
			case ELogLevel::Warning: return "Warning";
			case ELogLevel::Error:   return "Error";
			case ELogLevel::Fatal:   return "Fatal";
		}

		LK_ASSERT(false, "Unknown log level: {}", static_cast<int>(Level));
		return "";
	}

	ELogLevel CLog::LevelFromString(std::string_view InString)
	{
		std::basic_string<char> StrLower(InString);
		std::transform(StrLower.begin(), StrLower.end(), StrLower.begin(), [](char Character)
		{
			return static_cast<char>(std::tolower(Character));
		});

		if (StrLower == "trace")        return ELogLevel::Trace;
		else if (StrLower == "debug")   return ELogLevel::Debug;
		else if (StrLower == "info")    return ELogLevel::Info;
		else if (StrLower == "warning") return ELogLevel::Warning;
		else if (StrLower == "error")   return ELogLevel::Error;
		else if (StrLower == "fatal")   return ELogLevel::Fatal;

		LK_ASSERT(false, "LevelFromString failed for '{}' (lower: '{}')", InString, StrLower);
		return ELogLevel::Info;
	}

	spdlog::level::level_enum CLog::ToSpdlogLevel(const ELogLevel Level)
	{
		switch (Level)
		{
			case ELogLevel::Trace:   return spdlog::level::trace;
			case ELogLevel::Debug:   return spdlog::level::debug;
			case ELogLevel::Info:	 return spdlog::level::info;
			case ELogLevel::Warning: return spdlog::level::warn;
			case ELogLevel::Error:	 return spdlog::level::err;
			case ELogLevel::Fatal:	 return spdlog::level::critical;
		}

		LK_ASSERT(false, "ToSpdlogLevel error");
		return spdlog::level::info;
	}


	namespace LogUtility
	{
		/** Assuming the log files are formatted with a timestamp. */
		bool CompareLogFiles(const std::filesystem::directory_entry& A, const std::filesystem::directory_entry& B)
		{
			return (A.path().filename().string() < B.path().filename().string());
		}

		int CountLogFilesInDir(const std::filesystem::path& InDirectory)
		{
			namespace fs = std::filesystem;
			if (!fs::exists(InDirectory) || !fs::is_directory(InDirectory))
			{
				LK_ASSERT(false, "Invalid directory");
				return -1;
			}

			int Files = 0;
			for (const fs::directory_entry& Entry : fs::directory_iterator(InDirectory))
			{
				if (Entry.is_regular_file() && Entry.path().extension() == ".log")
				{
					Files++;
				}
			}

			return Files;
		}

		void CleanLogDirectory(const std::filesystem::path& InDirectory, const int MaxLogFiles)
		{
			namespace fs = std::filesystem;
			if (!fs::exists(InDirectory) || !fs::is_directory(InDirectory))
			{
				LK_ASSERT(false, "Invalid directory");
				return;
			}

			std::vector<fs::directory_entry> LogFiles;
			LogFiles.reserve(MaxLogFiles);
			for (const auto& Entry : fs::directory_iterator(InDirectory))
			{
				if (Entry.is_regular_file() && Entry.path().extension() == ".log")
				{
					LogFiles.push_back(Entry);
				}
			}

			LK_PRINTLN("LogFiles={} MaxLogFiles={}", LogFiles.size(), MaxLogFiles);

			/* Sort and remove oldest logfiles. */
			if (LogFiles.size() > MaxLogFiles)
			{
				/* Sort log files based on their names (timestamps in filenames). */
				std::sort(LogFiles.begin(), LogFiles.end(), CompareLogFiles);

				/* Remove the oldest files, keeping only the most recent ones. */
				for (std::size_t Index = 0; Index < LogFiles.size() - MaxLogFiles; Index++)
				{
					const fs::directory_entry& LogFile = LogFiles[Index];
					if (LogFile.path().extension() == ".log")
					{
						std::filesystem::remove(LogFile.path());
					}
				}
			}
		}
	}

}